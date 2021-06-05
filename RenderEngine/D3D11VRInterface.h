#pragma once

#include "IVRInterface.h"
#include "D3D11Object.h"

namespace SunEngine
{
	class D3D11VRInterface : public IVRInterface, public D3D11Object
	{
		const char* GetExtensionName() const override;
		bool Init(IVRInitInfo& info) override;
		bool InitTexture(VRHandle imgArray, uint imgIndex, int64 format, ITexture* pTexture) override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
	};

}