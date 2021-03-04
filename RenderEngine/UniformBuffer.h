#pragma once

#include "GraphicsObject.h"

namespace SunEngine
{
	class IShader;

	class UniformBuffer : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			uint size;
			bool isShared;
		};

		UniformBuffer();
		~UniformBuffer();

		bool Create(const CreateInfo& info);
		bool Destroy() override;
		bool Update(const void* pData);
		bool Update(const void* pData, uint offest, uint size);
		bool UpdateShared(const void* pData, uint numElements);
		uint GetMaxSharedUpdates() const;

		uint GetSize() const;

		IObject* GetAPIHandle() const override;
		static IUniformBuffer* Allocate(GraphicsAPI api);
	private:
		IUniformBuffer* _apiBuffer;
		uint _size;
	};

}