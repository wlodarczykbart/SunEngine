#include "Mesh.h"
#include "Material.h"
#include "SceneNode.h"
#include "ShaderMgr.h"
#include "Texture2D.h"
#include "Terrain.h"

namespace SunEngine
{
    Terrain::Terrain()
    {
        _resolution = 1025/2;
        _slices = 4;
        _mesh = UniquePtr<Mesh>(new Mesh());
        _material = UniquePtr<Material>(new Material());
        _material->SetShader(ShaderMgr::Get().GetShader(DefaultShaders::Terrain));
        _material->RegisterToGPU();

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

        VertexDef vtxDef = {};
        vtxDef.NumVars = 2;
        vtxDef.NormalIndex = 1;

        _mesh->AllocVertices(_resolution * _resolution, vtxDef);

        glm::vec4 offset = -glm::vec4(halfRef, 0, halfRef, 0);
        float scaleFactor = _resolution / (float)(_resolution - 1);
        //scaleFactor=1.0f;

        for (uint z = 0; z < _resolution; z++)
        {
            for (uint x = 0; x < _resolution; x++)
            {
                uint vIndex = z * _resolution + x;

                glm::vec4 position = glm::vec4(x, 0, z, 1) * scaleFactor + offset;
                _mesh->SetVertexVar(vIndex, position);
                _mesh->SetVertexVar(vIndex, Vec4::Up, vtxDef.NormalIndex);
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

                slice.aabb.Reset();
                for (uint i = 0; i < slice.indexCount; i++)
                {
                    glm::vec4 position = _mesh->GetVertexPos(_mesh->GetIndex(i + slice.firstIndex) + slice.vertexOffset);
                    slice.aabb.Expand(position);
                }

                //printf("(%d,%d), min: (%f,%f,%f), max: (%f,%f,%f)\n", slice.x, slice.y, slice.aabb.Min.x, slice.aabb.Min.y, slice.aabb.Min.z, slice.aabb.Max.x, slice.aabb.Max.y, slice.aabb.Max.z);

                _sliceData.push_back(slice);
                xOffset += sliceResolution;
            }
            yOffset += (_resolution * sliceResolution);
        }

        _mesh->RegisterToGPU();
    }

    void Terrain::SetHeights(Texture2D* pHeightTexture)
    {
        float mapTextureX = pHeightTexture->GetWidth() / (float)_resolution;
        float mapTextureY = pHeightTexture->GetHeight() / (float)_resolution;

        bool isFloatTexture = pHeightTexture->GetImageFlags() & ImageData::SAMPLED_TEXTURE_R32F;

        Vector<float> heights(pHeightTexture->GetWidth() * pHeightTexture->GetHeight());
        float maxHeight = -FLT_MAX;
        float minHeight = FLT_MAX;
        for (uint y = 0; y < pHeightTexture->GetHeight(); y++)
        {
            for (uint x = 0; x < pHeightTexture->GetWidth(); x++)
            {
                uint index = y * pHeightTexture->GetWidth() + x;
                if (isFloatTexture)
                {
                    pHeightTexture->GetFloat(x, y, heights[index]);
                }
                else
                {
                    Pixel p;
                    pHeightTexture->GetPixel(x, y, p);
                    heights[index] = (float)p.R;
                }
                maxHeight = glm::max(maxHeight, heights[index]);
                minHeight = glm::min(minHeight, heights[index]);
            }
        }

        float yStart = minHeight / maxHeight;

        for (uint y = 0; y < _resolution; y++)
        {
            uint ty = uint(y * mapTextureY);
            for (uint x = 0; x < _resolution; x++)
            {
                uint tx = uint(x * mapTextureX);

                uint vIndex = y * _resolution + x;
                uint tIndex = ty * pHeightTexture->GetWidth() + tx;
                glm::vec4 position = _mesh->GetVertexPos(vIndex);

                position.y = (heights[tIndex])/ maxHeight;
               // position.y -= yStart;
                position.y *= 100.0f;

                _mesh->SetVertexVar(vIndex, position);
            }
        }
        RecalcNormals();
        _mesh->UpdateVertices();

        for (uint s = 0; s < _sliceData.size(); s++)
        {
            Slice& slice = _sliceData[s];
            slice.aabb.Reset();
            for (uint i = 0; i < slice.indexCount; i++)
            {
                glm::vec4 position = _mesh->GetVertexPos(_mesh->GetIndex(i + slice.firstIndex) + slice.vertexOffset);
                slice.aabb.Expand(position);
            }
        }
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
#if 0
        for (uint y = 0; y < _resolution; y++)
        {
            uint top = y == 0 ? y : y - 1;
            uint bottom = y == _resolution - 1 ? y : y + 1;
            for (uint x = 0; x < _resolution; x++)
            {
                uint left = x == 0 ? x : x - 1;
                uint right = x == _resolution - 1 ? x : x + 1;

                glm::vec4 vLeft = _mesh->GetVertexPos(y * _resolution + left);
                glm::vec4 vRight = _mesh->GetVertexPos(y * _resolution + right);
                glm::vec4 vTop = _mesh->GetVertexPos(top * _resolution + x);
                glm::vec4 vBottom = _mesh->GetVertexPos(bottom * _resolution + x);

                glm::vec4 normal;
                normal.x = (vRight - vLeft).y;
                normal.z = (vBottom - vTop).y;
                normal.y = 2.0f;
                normal.w = 0.0f;

                normal = glm::normalize(normal);

                _mesh->SetVertexVar(y * _resolution + x, normal, 1);
            }
        }
#else
        Vector<glm::vec3> normals;
        normals.resize(_resolution * _resolution);
        memset(normals.data(), 0x0, sizeof(glm::vec3)* normals.size());

        for (uint y = 0; y < _resolution - 1; y++)
        {
            for (uint x = 0; x < _resolution - 1; x++)
            {
                uint topLeft = y * _resolution + x;
                uint topRight = topLeft + 1;
                uint bottomLeft = topLeft + _resolution;
                uint bottomRight = topRight + 1;

                {
                    glm::vec3 p0 = _mesh->GetVertexPos(topLeft);
                    glm::vec3 p1 = _mesh->GetVertexPos(bottomLeft);
                    glm::vec3 p2 = _mesh->GetVertexPos(bottomRight);
                    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
                    normals[topLeft] += normal;
                    normals[bottomLeft] += normal;
                    normals[bottomRight] += normal;
                }

                {
                    glm::vec3 p0 = _mesh->GetVertexPos(topLeft);
                    glm::vec3 p1 = _mesh->GetVertexPos(bottomRight);
                    glm::vec3 p2 = _mesh->GetVertexPos(topRight);
                    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
                    normals[topLeft] += normal;
                    normals[bottomRight] += normal;
                    normals[topRight] += normal;
                }
            }
        }

        for (uint i = 0; i < normals.size(); i++)
        {
            _mesh->SetVertexVar(i, glm::vec4(glm::normalize(normals[i]), 0.0f), 1);
        }
#endif
    }
}