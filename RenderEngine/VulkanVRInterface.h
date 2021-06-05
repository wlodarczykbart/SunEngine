#pragma once

#include "IVRInterface.h"
#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanVRInterface : public IVRInterface, public VulkanObject
	{
		const char* GetExtensionName() const override;
		bool Init(IVRInitInfo& info) override;
		bool InitTexture(VRHandle imgArray, uint imgIndex, int64 format, ITexture* pTexture) override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
	};
}