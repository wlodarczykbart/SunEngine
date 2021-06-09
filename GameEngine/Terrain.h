#pragma once

#include "RenderObject.h"
#include "FilePathMgr.h"

namespace SunEngine
{
	class Texture2D;
	class Texture2DArray;

	class TerrainComponentData : public RenderComponentData
	{
	public:
		TerrainComponentData(Component* pComponent, SceneNode* pNode) : RenderComponentData(pComponent, pNode) { }

	private:
		friend class Terrain;
		Map<RenderNode*, uint> _sliceNodes;
	};

	class Terrain : public RenderObject
	{
	public:
		class Strings
		{
		public:
			static const String SplatMap;
			static const String SplatSampler;
			static const String PosToUV;
			static const String TextureTiling;
		};

		class Biome
		{
		public:
			Biome(const String& name);

			const String& GetName() const { return _name; }

			void SetTexture(Texture2D* pTexture);
			Texture2D* GetTexture() const { return _texture; }

			void SetResolutionScale(float value);
			float GetResolutionScale() const { return _resolutionScale; }

			void SetHeightScale(float value);
			float GetHeightScale() const { return _heightScale; }

			void SetHeightOffset(float value);
			float GetHeightOffset() const { return _heightOffset; }

			void SetSmoothKernelSize(int value);
			int GetSmoothKernelSize() const { return _smoothKernelSize; }

			void SetInvert(bool value);
			int GetInvert() const { return _invert; }

			void SetCenter(const glm::ivec2& value);
			const glm::ivec2& GetCenter() const { return _center; }

		private:
			float GetSmoothHeight(int x, int y) const;

			friend class Terrain;

			String _name;
			Texture2D* _texture;
			float _resolutionScale;
			float _heightScale;
			float _heightOffset;
			int _smoothKernelSize;
			bool _invert;
			glm::ivec2 _center;
			bool _changed;
			Vector<float> _normalizedTextureHeights;
			Vector<float> _heights;
		};

		Terrain();
		~Terrain();

		void SetResolution(uint resolution) { _resolution = resolution; }
		uint GetResolution() const { return _resolution; }

		void SetSlices(uint slices) { _slices = slices; }
		uint GetSlices() const { return _slices; }

		void BuildMesh();

		void Initialize(SceneNode* pNode, ComponentData* pData) override;
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

		Material* GetMaterial() const { return _material.get(); }
		Mesh* GetMesh() const { return _mesh.get(); }

		Biome* AddBiome(const String& name);
		Biome* GetBiome(const String& name) const;
		void UpdateBiomes();
		void GetBiomes(Vector<Biome*>& biomes) const;

		bool SetDiffuseMap(uint index, Texture2D* pTexture);
		bool BuildDiffuseMapArray(bool generateMips);

		bool SetNormalMap(uint index, Texture2D* pTexture);
		bool BuildNormalMapArray(bool generateMips);

	private:
		struct Slice
		{
			uint x;
			uint y;
			uint firstIndex;
			uint indexCount;
			uint vertexOffset;
			AABB aabb;
		};

		struct Splat
		{
			Splat();

			Splat GetNormalized(uint textureCount) const;

			float weights[EngineInfo::Renderer::Limits::MaxTerrainTextures];
		};

		RenderComponentData* AllocRenderData(SceneNode* pNode) { return new TerrainComponentData(this, pNode); }
		bool RequestData(RenderNode* pNode, RenderComponentData* pData, Mesh*& pMesh, Material*& pMaterial, const glm::mat4*& worldMtx, const AABB*& aabb, uint& idxCount, uint& instanceCount, uint& firstIdx, uint& vtxOffset) const override;
		void BuildSliceIndices(Map<glm::uvec2, Vector<uint>>& sliceTypeIndices, uint& indexCount) const;
		void BuildPipelineSettings(PipelineSettings& settings) const override;
		void RecalcNormals();

		const Splat& GetSplat(uint x, uint y) const;
		float GetSplat(uint x, uint y, uint index) const;
		void SetSplat(uint x, uint y, uint index, float value);
		void IncrementSplat(uint x, uint y, uint index, float value);
		void DecrementSplat(uint x, uint y, uint index, float value);

		uint _resolution;
		uint _slices;
		Vector<Slice> _sliceData;

		UniquePtr<Mesh> _mesh;
		UniquePtr<Material> _material;
		StrMap<UniquePtr<Biome>> _biomes;
		UniquePtr<Texture2DArray> _diffuseMapArray;
		UniquePtr<Texture2DArray> _normalMapArray;
		UniquePtr<Texture2DArray> _splatMapArray;

		Vector<Splat> _splatLookup;
		float _textureTiling[EngineInfo::Renderer::Limits::MaxTerrainTextures];
	};
}