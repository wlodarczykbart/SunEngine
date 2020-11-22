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
			IShaderResource resource;
			IShader* pShader;
		};

		UniformBuffer();
		~UniformBuffer();

		bool Create(const CreateInfo& info);
		bool Destroy() override;
		bool Update(const void* pData);

		uint GetSize() const;

		IObject* GetAPIHandle() const override;
		static IUniformBuffer* Allocate(GraphicsAPI api);
	private:
		IUniformBuffer* _apiBuffer;
		uint _size;
	};

}