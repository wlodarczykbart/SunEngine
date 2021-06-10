#include "Mesh.h"
#include "Material.h"
#include "SceneNode.h"
#include "ShaderMgr.h"
#include "Texture2D.h"
#include "Texture2DArray.h"
#include "ThreadPool.h"
#include "FilePathMgr.h"
#include "ResourceMgr.h"
#include "FastNoise.h"
#include "Terrain.h"

namespace SunEngine
{
    const String Terrain::Strings::SplatMap = "SplatMap";
    const String Terrain::Strings::SplatSampler = "SplatSampler";
    const String Terrain::Strings::PosToUV = "PosToUV";
    const String Terrain::Strings::TextureTiling = "TextureTiling";

    Terrain::Terrain() : RenderObject(RO_TERRAIN)
    {
        _resolution = 2048;
        _slices = 4;
        _mesh = UniquePtr<Mesh>(new Mesh());
        _material = UniquePtr<Material>(new Material());
        _material->SetShader(ShaderMgr::Get().GetShader(DefaultShaders::Terrain));
        _material->RegisterToGPU();

        _diffuseMapArray = UniquePtr<Texture2DArray>(new Texture2DArray());
        _diffuseMapArray->SetWidth(EngineInfo::GetRenderer().TerrainTextureResolution());
        _diffuseMapArray->SetHeight(EngineInfo::GetRenderer().TerrainTextureResolution());

        _normalMapArray = UniquePtr<Texture2DArray>(new Texture2DArray());
        _normalMapArray->SetWidth(EngineInfo::GetRenderer().TerrainTextureResolution());
        _normalMapArray->SetHeight(EngineInfo::GetRenderer().TerrainTextureResolution());

        _splatMapArray = UniquePtr<Texture2DArray>(new Texture2DArray());
        _splatMapArray->SetWidth(_resolution);
        _splatMapArray->SetHeight(_resolution);

        uint numTextures = EngineInfo::GetRenderer().TerrainTextures();
        for (uint i = 0; i < numTextures; i++)
        {
            _diffuseMapArray->AddTexture(ResourceMgr::Get().GetTexture2D(DefaultResource::Texture::White));
            _normalMapArray->AddTexture(ResourceMgr::Get().GetTexture2D(DefaultResource::Texture::Normal));
            _textureTiling[i] = 45 + glm::linearRand(1, 10);
        }

        _splatMapArray->AddTexture(ResourceMgr::Get().GetTexture2D(DefaultResource::Texture::Red));
        for(uint i = 1; i < numTextures / 4; i++)
            _splatMapArray->AddTexture(ResourceMgr::Get().GetTexture2D(DefaultResource::Texture::Black));

        BuildDiffuseMapArray(false);
        BuildNormalMapArray(false);

        uint setCount = 0;
        setCount += _material->SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_REPEAT, SE_AM_8));
        setCount += _material->SetSampler(Strings::SplatSampler, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE));
        setCount += _material->SetMaterialVar(Strings::PosToUV, glm::vec4(1.0f / _resolution, 1.0f / _resolution, 0.5f, 0.5f));
        setCount += _material->SetMaterialVar(Strings::TextureTiling, _textureTiling, sizeof(float) * numTextures);

        BuildMesh();
    }

    Terrain::~Terrain()
    {

    }

    void Terrain::BuildMesh()
    {
        //no change...
        if (_mesh->GetVertexCount() == (_resolution * _resolution) && _sliceData.size() == (_slices * _slices))
            return;

        uint halfRef = _resolution / 2;
        _mesh->AllocVertices(_resolution * _resolution, TerrainVertex::Definition);
        _splatLookup.resize(_resolution * _resolution);
        memset(_splatLookup.data(), 0x0, sizeof(Splat) * _splatLookup.size());

        glm::vec4 offset = -glm::vec4(halfRef, 0, halfRef, 0);
        float scaleFactor = _resolution / (float)(_resolution - 1);
        //scaleFactor=1.0f;

        TerrainVertex* pVerts = _mesh->GetVertices<TerrainVertex>();
        for (uint z = 0; z < _resolution; z++)
        {
            for (uint x = 0; x < _resolution; x++)
            {
                uint vIndex = z * _resolution + x;

                pVerts[vIndex].Position = glm::vec4(x, 0, z, 1) * scaleFactor + offset;
                //printf("%f %f\n", pVerts[vIndex].Position.x, pVerts[vIndex].Position.z);
            }
        }

        //uint testVal = (_resolution - 1) * (_resolution - 1) * 6;

        uint indexCount = 0;
        Map<glm::uvec2, Vector<uint>> sliceTypeIndices;
        BuildSliceIndices(sliceTypeIndices, indexCount);
        _mesh->AllocIndices(indexCount);
        
        Map<glm::uvec2, uint> sliceTypeOffsets;
        uint indexOffset = 0;
        for (auto& sliceIndices : sliceTypeIndices)
        {
            _mesh->SetIndices(sliceIndices.second.data(), indexOffset, sliceIndices.second.size());
            sliceTypeOffsets[sliceIndices.first] = indexOffset;
            indexOffset += sliceIndices.second.size();
        }
        
        uint yOffset = 0;
        uint sliceResolution = _resolution / _slices;
        _sliceData.resize(_slices * _slices);
        for (uint sliceY = 0; sliceY < _slices; sliceY++)
        {
            uint xOffset = 0;
            for (uint sliceX = 0; sliceX < _slices; sliceX++)
            {
                Slice slice = {};
                slice.x = sliceX;
                slice.y = sliceY;

                glm::uvec2 sliceType;
                sliceType.x = slice.x == _slices - 1 ? 1 : 0;
                sliceType.y = slice.y == _slices - 1 ? 1 : 0;

                slice.firstIndex = sliceTypeOffsets[sliceType];
                slice.indexCount = sliceTypeIndices[sliceType].size();
                slice.vertexOffset = yOffset + xOffset;

                slice.aabb.Min.y = slice.aabb.Max.y = 0.0f;

                slice.aabb.Min.x = float(sliceX * sliceResolution) - halfRef;
                slice.aabb.Max.x = slice.aabb.Min.x + sliceResolution;

                slice.aabb.Min.z = float(sliceY * sliceResolution) - halfRef;
                slice.aabb.Max.z = slice.aabb.Min.z + sliceResolution;


                //printf("(%d,%d), min: (%f,%f,%f), max: (%f,%f,%f)\n", slice.x, slice.y, slice.aabb.Min.x, slice.aabb.Min.y, slice.aabb.Min.z, slice.aabb.Max.x, slice.aabb.Max.y, slice.aabb.Max.z);

                _sliceData[sliceY * _slices + sliceX] = slice;
                xOffset += sliceResolution;
            }
            yOffset += (_resolution * sliceResolution);
        }

        _mesh->RegisterToGPU();
        BuildSplatArray();
    }

    void Terrain::BuildSliceIndices(Map<glm::uvec2, Vector<uint>> &sliceTypeIndices, uint& indexCount) const
    {
        uint sliceResolution = _resolution / _slices;

        //generate 4 types of mesh indices for possible location of slices on the vertex grid
        glm::uvec2 sliceTypeResolutions[] =
        {
            glm::uvec2(0, 0), //mesh indices for slice that is not on a exiting boundary
            glm::uvec2(1, 0), //mesh indices for slice that is on the right boundary
            glm::uvec2(0, 1), //mesh indices for slice that is on the bottom boundary
            glm::uvec2(1, 1), //mesh indices for slice that is on the right and bottom boundary
        };

        for (uint t = 0; t < SE_ARR_SIZE(sliceTypeResolutions); t++)
        {
            uint resX = sliceResolution - sliceTypeResolutions[t].x;
            uint resY = sliceResolution - sliceTypeResolutions[t].y;

            Vector<uint>& indices = sliceTypeIndices[sliceTypeResolutions[t]];

            indices.resize(resX * resY * 6);
            uint idxCounter = 0;
            for (uint y = 0; y < resY; y++)
            {
                for (uint x = 0; x < resX; x++)
                {
                    uint topLeft = y * _resolution + x;
                    uint topRight = topLeft + 1;
                    uint bottomLeft = topLeft + _resolution;
                    uint bottomRight = bottomLeft + 1;

                    indices[idxCounter++] = topLeft;
                    indices[idxCounter++] = bottomLeft;
                    indices[idxCounter++] = bottomRight;
                    indices[idxCounter++] = topLeft;
                    indices[idxCounter++] = bottomRight;
                    indices[idxCounter++] = topRight;
                }
            }

            indexCount += indices.size();
        }
    }

    void Terrain::Initialize(SceneNode* pNode, ComponentData* pData)
    {
        TerrainComponentData* pRenderData = pData->As<TerrainComponentData>();
        pRenderData->_sliceNodes.clear();

        for (uint i = 0; i < _sliceData.size(); i++)
        {
            RenderNode* pRenderNode = CreateRenderNode(pRenderData);
            pRenderData->_sliceNodes[pRenderNode] = i;
        }

        RenderObject::Initialize(pNode, pData);
    }

    void Terrain::Update(SceneNode* pNode, ComponentData* pData, float dt, float et)
    {
        RenderObject::Update(pNode, pData, dt, et);
    }

    Terrain::Biome* Terrain::AddBiome(const String& name)
    {
        Biome* pBiome = new Biome(name);
        _biomes[name] = UniquePtr<Biome>(pBiome);
        return pBiome;
    }

    Terrain::Biome* Terrain::GetBiome(const String& name) const
    {
        auto found = _biomes.find(name);
        return found != _biomes.end() ? (*found).second.get() : 0;
    }

    void Terrain::UpdateBiomes()
    {
        TerrainVertex* pVerts = _mesh->GetVertices<TerrainVertex>();
        for (uint i = 0; i < _mesh->GetVertexCount(); i++)
        {
            pVerts[i].Position.y = 0.0f;
        }

        //Can this be threaded?
        for (auto& kv : _biomes)
        {
            auto& biome = kv.second;
            if (biome->_changed && biome->_normalizedTextureHeights.size())
            {
                int resx = int(biome->_texture->GetWidth() * biome->_resolutionScale);
                int resy = int(biome->_texture->GetHeight() * biome->_resolutionScale);

                biome->_heights.resize(resx * resy);

                float invResolution = 1.0f / biome->_resolutionScale;
                for (int y = 0; y < resy; y++)
                {
                    int ty = int(y * invResolution);
                    for (int x = 0; x < resx; x++)
                    {
                        int tx = int(x * invResolution);

                        float height = biome->GetSmoothHeight(tx, ty);
                        if (biome->_invert) height = 1.0f - height;
                        height *= biome->_heightScale;
                        height += biome->_heightOffset;
                        int index = y * resx + x;
                        biome->_heights[index] = height;
                    }
                }
            }
        }

        int halfRes = _resolution / 2;

        for (auto& kv : _biomes)
        {
            auto& biome = kv.second;
            if (biome->_normalizedTextureHeights.size())
            {
                int resx = int(biome->_texture->GetWidth() * biome->_resolutionScale);
                int resy = int(biome->_texture->GetHeight() * biome->_resolutionScale);

                int halfResx = resx / 2;
                int halfResy = resy / 2;

                glm::vec2 uvRange = glm::vec2(resx, resy);

                for (int y = 0; y < resy; y++)
                {
                    int posy = (y - halfResy) + biome->_center.y + halfRes;
                    if (posy >= 0 && posy < _resolution)
                    {
                        for (int x = 0; x < resx; x++)
                        {
                            int posx = (x - halfResx) + biome->_center.x + halfRes;
                            if (posx >= 0 && posx < _resolution)
                            {
                                glm::vec2 uv = glm::vec2(x, y) / uvRange;
                                uv = uv * 2.0f - 1.0f;
                                float t = 1.0f - glm::min(glm::length(uv), 1.0f); //TODO: apply some exponential function?
                                t = t * t;

                                float& height = pVerts[posy * _resolution + posx].Position.y;
                                height = glm::mix(height, biome->_heights[y * resx + x], t);
                            }
                        }
                    }
                }
            }
        }

        RecalcNormals();
        _mesh->UpdateVertices();

        for (uint s = 0; s < _sliceData.size(); s++)
        {
            Slice& slice = _sliceData[s];
            slice.aabb.Min.y = FLT_MAX;
            slice.aabb.Max.y = -FLT_MAX;
        }

        uint sliceResolution = _resolution / _slices;
        for (uint z = 0; z < _resolution; z++)
        {
            uint sliceY = z / sliceResolution;
            for (uint x = 0; x < _resolution; x++)
            {
                uint sliceX = x / sliceResolution;
                uint vIndex = z * _resolution + x;
                uint sIndex = sliceY * _slices + sliceX;
                _sliceData[sIndex].aabb.ExpandY(pVerts[vIndex].Position.y);
            }
        }

        BuildSplatArray();
    }

    void Terrain::GetBiomes(Vector<Biome*>& biomes) const
    {
        biomes.clear();
        for (auto& biome : _biomes)
        {
            biomes.push_back(biome.second.get());
        }
    }

    bool Terrain::SetDiffuseMap(uint index, Texture2D* pTexture)
    {
        if (!_diffuseMapArray->SetTexture(index, pTexture))
            return false;

        return true;
    }

    bool Terrain::BuildDiffuseMapArray(bool generateMips)
    {
        _diffuseMapArray->SetSRGB();

        if (generateMips)
            _diffuseMapArray->GenerateMips(true);

        if (!_diffuseMapArray->RegisterToGPU())
            return false;

        if (!_material->SetTexture2DArray(MaterialStrings::DiffuseMap, _diffuseMapArray.get()))
            return false;

        return true;
    }

    bool Terrain::SetNormalMap(uint index, Texture2D* pTexture)
    {
        if (!_normalMapArray->SetTexture(index, pTexture))
            return false;

        return true;
    }

    bool Terrain::BuildNormalMapArray(bool generateMips)
    {
        if (generateMips)
            _diffuseMapArray->GenerateMips(true);

        if (!_normalMapArray->RegisterToGPU())
            return false;

        if (!_material->SetTexture2DArray(MaterialStrings::NormalMap, _normalMapArray.get()))
            return false;

        return true;
    }

    void Terrain::BuildPipelineSettings(PipelineSettings& settings) const
    {
        settings.inputAssembly.topology = SE_PT_TRIANGLE_LIST;
    }

    bool Terrain::RequestData(RenderNode* pNode, RenderComponentData* pData, Mesh*& pMesh, Material*& pMaterial, const glm::mat4*& worldMtx, const AABB*& aabb, uint& idxCount, uint& instanceCount, uint& firstIdx, uint& vtxOffset) const
    {
        TerrainComponentData* pRenderData = pData->As<TerrainComponentData>();

        auto found = pRenderData->_sliceNodes.find(pNode);
        assert(found != pRenderData->_sliceNodes.end());
        if (found == pRenderData->_sliceNodes.end())
            return false;

        const Slice& slice = _sliceData[(*found).second];

        pMesh = _mesh.get();
        pMaterial = _material.get();
        worldMtx = &pNode->GetNode()->GetWorld();
        aabb = &slice.aabb;
        firstIdx = slice.firstIndex;
        idxCount = slice.indexCount;
        vtxOffset = slice.vertexOffset;
        instanceCount = 1;

        return false;
    }

    void Terrain::RecalcNormals()
    {
#if 1
        TerrainVertex* pVerts = _mesh->GetVertices<TerrainVertex>();

#if 1
        ThreadPool& tp = ThreadPool::Get();
        struct ThreadData
        {
            glm::uvec2 ranges[16];
            uint resolution;
            TerrainVertex* pVerts;
        } threadData;

        threadData.resolution = _resolution;
        threadData.pVerts = pVerts;

        uint vertsPerThread = _mesh->GetVertexCount() / tp.GetThreadCount();
        uint vtxStart = 0;
        for (uint i = 0; i < tp.GetThreadCount(); i++)
        {
            threadData.ranges[i].x = vtxStart;
            threadData.ranges[i].y = glm::min(vtxStart + vertsPerThread, _mesh->GetVertexCount());
            tp.AddTask([](uint threadIdx, void* pData) -> void {
                ThreadData* pThreadData = static_cast<ThreadData*>(pData);
                TerrainVertex* pVerts = pThreadData->pVerts;
                uint resolution = pThreadData->resolution;
                uint resolution2 = resolution * resolution;
                for (uint v = pThreadData->ranges[threadIdx].x; v < pThreadData->ranges[threadIdx].y; v++)
                {
                    uint y = v / resolution;
                    uint x = v % resolution;

                    uint top = y == 0 ? y : y - 1;
                    uint bottom = y == resolution - 1 ? y : y + 1;
                    uint left = x == 0 ? x : x - 1;
                    uint right = x == resolution - 1 ? x : x + 1;

                    float fLeft = pVerts[y * resolution + left].Position.y;
                    float fRight = pVerts[y * resolution + right].Position.y;
                    float fTop = pVerts[top * resolution + x].Position.y;
                    float fBottom = pVerts[bottom * resolution + x].Position.y;

                    //assert(y * resolution + x < resolution2);
                    //assert(y * resolution + left < resolution2);
                    //assert(y * resolution + right < resolution2);
                    //assert(top * resolution + x < resolution2);
                    //assert(bottom * resolution + x < resolution2);

                    glm::vec3 normal;
                    normal.x = (fRight - fLeft);
                    normal.z = (fBottom - fTop);
                    normal.y = 2.0f;
                    pVerts[v].Normal = glm::vec4(glm::normalize(normal), 0.0f);
                }
            }, &threadData);
            vtxStart += vertsPerThread;
        }

        tp.Wait();

#else
        for (uint y = 0; y < _resolution; y++)
        {
            uint top = y == 0 ? y : y - 1;
            uint bottom = y == _resolution - 1 ? y : y + 1;
            for (uint x = 0; x < _resolution; x++)
            {
                uint left = x == 0 ? x : x - 1;
                uint right = x == _resolution - 1 ? x : x + 1;

                const glm::vec4& vLeft = pVerts[y * _resolution + left].Position;
                const glm::vec4& vRight = pVerts[y * _resolution + right].Position;
                const glm::vec4& vTop = pVerts[top * _resolution + x].Position;
                const glm::vec4& vBottom = pVerts[bottom * _resolution + x].Position;

                glm::vec4 normal;
                normal.x = (vRight - vLeft).y;
                normal.z = (vBottom - vTop).y;
                normal.y = 2.0f;
                normal.w = 0.0f;
                pVerts[y * _resolution + x].Normal = glm::normalize(normal);
            }
        }
#endif
#else
        Vector<glm::vec3> normals;
        normals.resize(_resolution * _resolution);
        memset(normals.data(), 0x0, sizeof(glm::vec3)* normals.size());

        TerrainVertex* pVerts = _mesh->GetVertices<TerrainVertex>();

        for (uint y = 0; y < _resolution - 1; y++)
        {
            for (uint x = 0; x < _resolution - 1; x++)
            {
                uint topLeft = y * _resolution + x;
                uint topRight = topLeft + 1;
                uint bottomLeft = topLeft + _resolution;
                uint bottomRight = topRight + 1;

                {
                    glm::vec3 p0 = pVerts[topLeft].Position;
                    glm::vec3 p1 = pVerts[bottomLeft].Position;
                    glm::vec3 p2 = pVerts[bottomRight].Position;
                    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
                    normals[topLeft] += normal;
                    normals[bottomLeft] += normal;
                    normals[bottomRight] += normal;
                }

                {
                    glm::vec3 p0 = pVerts[topLeft].Position;
                    glm::vec3 p1 = pVerts[bottomRight].Position;
                    glm::vec3 p2 = pVerts[topRight].Position;
                    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
                    normals[topLeft] += normal;
                    normals[bottomRight] += normal;
                    normals[topRight] += normal;
                }
            }
        }

        for (uint i = 0; i < normals.size(); i++)
        {
            pVerts->Normal = glm::vec4(glm::normalize(normals[i]), 0.0f);
        }
#endif
    }

    const Terrain::Splat& Terrain::GetSplat(uint x, uint y) const
    {
        return _splatLookup[y * _resolution + x];
    }

    float Terrain::GetSplat(uint x, uint y, uint index) const
    {
        return GetSplat(x, y).weights[index];
    }

    void Terrain::SetSplat(uint x, uint y, uint index, float value)
    {
        auto& splat = _splatLookup[y * _resolution + x];
        splat.weights[index] = glm::max(value, 0.0f);
    }

    void Terrain::IncrementSplat(uint x, uint y, uint index, float value)
    {
        auto& splat = _splatLookup[y * _resolution + x];
        splat.weights[index] += value;
    }

    void Terrain::DecrementSplat(uint x, uint y, uint index, float value)
    {
        auto& splat = _splatLookup[y * _resolution + x];
        splat.weights[index] -= value;
        splat.weights[index] = glm::max(splat.weights[index], 0.0f);
    }

    bool Terrain::BuildSplatArray()
    {
        FastNoise noise;
        noise.SetFrequency(noise.GetFrequency() * 4.0f);

        const float baseGrassValue = 100.0f;
        const float baseRockValue = 400.0f;

        for (uint z = 0; z < _resolution; z++)
        {
            for (uint x = 0; x < _resolution; x++)
            {
                float t = noise.GetPerlinFractal(x, z);
                t = t * 0.5f + 0.5f;
                assert(t <= 1.0f);
                SetSplat(x, z, 0, baseGrassValue * t);
                SetSplat(x, z, 1, baseGrassValue * (1.0f - t));

                uint vIndex = z * _resolution + x;
                float n = 1.0f - glm::max(_mesh->GetVertexVar(vIndex, TerrainVertex::Definition.NormalIndex).y, 0.0f);
               // n = n * n;
                SetSplat(x, z, 2, baseRockValue * n);
            }
        }

        uint texCount = EngineInfo::GetRenderer().TerrainTextures();
        uint splatCount = texCount / 4;
        glm::vec4 splatColors[EngineInfo::Renderer::Limits::MaxTerrainTextures / 4];
        memset(splatColors, 0x0, sizeof(splatColors));

        for (uint z = 0; z < _resolution; z++)
        {
            for (uint x = 0; x < _resolution; x++)
            {
                Splat splat = GetSplat(x, z).GetNormalized(texCount);

                for (uint i = 0; i < texCount; i++)
                {
                    uint r = i / 4;
                    uint c = i % 4;
                    splatColors[r][c] = splat.weights[i];
                }

                for (uint i = 0; i < splatCount; i++)
                {
                    _splatMapArray->SetPixel(x, z, i, splatColors[i]);
                    splatColors[i] = Vec4::Zero;
                }
            }
        }

        _splatMapArray->RegisterToGPU();
        _material->SetTexture2DArray(Strings::SplatMap, _splatMapArray.get());
        return true;
    }

    Terrain::Biome::Biome(const String& name)
    {
        _name = name;
        _texture = 0;
        _resolutionScale = 1.0f;
        _heightScale = 100;
        _heightOffset = -100.0f;
        _smoothKernelSize = 1;
        _center = glm::ivec2(0);
        _invert = false;
    }

    void Terrain::Biome::SetTexture(Texture2D* pTexture)
    {
        if (_texture != pTexture)
        {
            _texture = pTexture;
            if (_texture)
            {
                _normalizedTextureHeights.resize(pTexture->GetWidth() * pTexture->GetHeight());

                bool isFloatTexture = pTexture->GetImageFlags() & ImageData::SAMPLED_TEXTURE_R32F;
                float maxHeight = -FLT_MAX;

                for (uint y = 0; y < pTexture->GetHeight(); y++)
                {
                    for (uint x = 0; x < pTexture->GetWidth(); x++)
                    {
                        uint index = y * pTexture->GetWidth() + x;
                        if (isFloatTexture)
                        {
                            pTexture->GetFloat(x, y, _normalizedTextureHeights[index]);
                        }
                        else
                        {
                            Pixel p;
                            pTexture->GetPixel(x, y, p);
                            _normalizedTextureHeights[index] = (float)p.R;
                        }
                        maxHeight = glm::max(maxHeight, _normalizedTextureHeights[index]);
                    }
                }

                for (uint y = 0; y < pTexture->GetHeight(); y++)
                {
                    for (uint x = 0; x < pTexture->GetWidth(); x++)
                    {
                        uint index = y * pTexture->GetWidth() + x;
                        _normalizedTextureHeights[index] /= maxHeight;
                    }
                }
            }
            else
            {
                _normalizedTextureHeights.clear();
            }
            _changed = true;
        }
    }

    void Terrain::Biome::SetResolutionScale(float value)
    {
        if (_resolutionScale != value)
        {
            _resolutionScale = value;
            _changed = true;
        }
    }

    void Terrain::Biome::SetHeightScale(float value)
    {
        if (_heightScale != value)
        {
            _heightScale = value;
            _changed = true;
        }
    }

    void Terrain::Biome::SetHeightOffset(float value)
    {
        if (_heightOffset != value)
        {
            _heightOffset = value;
            _changed = true;
        }
    }

    void Terrain::Biome::SetSmoothKernelSize(int value)
    {
        if (_smoothKernelSize != value)
        {
            _smoothKernelSize = value;
            _changed = true;
        }
    }

    void Terrain::Biome::SetInvert(bool value)
    {
        if (_invert != value)
        {
            _invert = value;
            _changed = true;
        }
    }

    void Terrain::Biome::SetCenter(const glm::ivec2& value)
    {
        if (_center != value)
        {
            _center = value;
            _changed = true;
        }
    }

    float Terrain::Biome::GetSmoothHeight(int x, int y) const
    {
        if (_smoothKernelSize <= 0)
            return _normalizedTextureHeights[y * _texture->GetWidth() + x];

        float height = 0.0f;
        int samples = 0;

        for (int i = -_smoothKernelSize; i <= _smoothKernelSize; i++)
        {
            int yi = y + i;
            if (yi >= 0 && yi < _texture->GetHeight())
            {
                for (int j = -_smoothKernelSize; j <= _smoothKernelSize; j++)
                {
                    int xj = x + j;
                    if (xj >= 0 && xj < _texture->GetWidth())
                    {
                        uint index = yi * _texture->GetWidth() + xj;
                        height += _normalizedTextureHeights[index];
                        ++samples;
                    }
                }
            }
        }

        return height / samples;
    }

    Terrain::Splat::Splat()
    {
        for (uint i = 0; i < EngineInfo::Renderer::Limits::MaxTerrainTextures; i++)
            weights[i] = 0.0f;
    }

    Terrain::Splat Terrain::Splat::GetNormalized(uint textureCount) const
    {
        float sum = 0.0f;
        for (uint i = 0; i < textureCount; i++)
            sum += weights[i];

        Splat result;
        for (uint i = 0; i < textureCount; i++)
            result.weights[i] = weights[i] / sum;

        return result;
    }
}