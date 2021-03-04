#pragma once

#include "ITextureArray.h"
#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanTextureArray : public VulkanObject, public ITextureArray
	{
	public:
		VulkanTextureArray();
		~VulkanTextureArray();

		bool Create(const ITextureArrayCreateInfo& info) override;
		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		friend class VulkanShaderBindings;
	};

}