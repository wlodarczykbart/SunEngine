#pragma once

#include "IObject.h"

namespace SunEngine
{
	class IDevice
	{
	public:
		IDevice();
		virtual ~IDevice();

		virtual bool Create(const IDeviceCreateInfo& info) = 0;
		virtual bool Destroy() = 0;
		virtual const String &GetErrorMsg() const = 0;
		virtual String QueryAPIError() = 0;
		virtual bool WaitIdle() = 0;

		static IDevice * Allocate(GraphicsAPI api);

		void SetFrameNumber(uint frameNumber);
		uint GetFrameNumber() const;
	private:
		uint _frameNumber;
	};

}