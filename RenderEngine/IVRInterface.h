#pragma once

#include "IObject.h"

#define XR_RETURN_ON_FAIL(exp) { XrResult result = exp; if (result != XR_SUCCESS) return false; }

namespace SunEngine
{
	class IDevice;

	class IVRInterface : public IObject
	{
	public:
		IVRInterface();
		virtual ~IVRInterface();

		virtual bool Init(IVRInitInfo& info) = 0;
		virtual bool InitTexture(VRHandle imgArray, uint imgIndex, int64 format, ITexture* pTexture) = 0;

		virtual const char* GetExtensionName() const = 0;

		static IVRInterface* Allocate(GraphicsAPI api);
	};
}