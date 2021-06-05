#pragma once
#include "GraphicsObject.h"
#include "BaseShader.h"

namespace SunEngine
{
	class IVRInterface;

	class VRInterface : public GraphicsObject
	{
	public:
		struct SwapchainRenderInfo
		{
			struct
			{
				float w;
				float x;
				float y;
				float z;
			} orientation;
			
			struct
			{
				float x;
				float y;
				float z;
			} position;

			struct
			{
				float left;
				float right;
				float up;
				float down;
			} fov;

			RenderTarget* pRenderTarget;
			CommandBuffer* pCmdBuffer;
			bool shouldRender;
		};

		struct CreateInfo
		{
			bool debugEnabled;
			bool createDepthBuffer;
		};

		VRInterface();
		~VRInterface();

		bool Create(const CreateInfo& info);
		bool GetDebugEnabled() const { return _debugEnabled; }

		IObject* GetAPIHandle() const override;

		bool Update();
		bool StartFrame();
		bool StartSwapchain(uint swapchainIndex, SwapchainRenderInfo& renderInfo);
		bool EndSwapchain(uint swapchainIndex);
		bool EndFrame();

		uint GetSwapchainCount() const;

	private:
		class Impl;
		UniquePtr<Impl> _impl;

		IVRInterface* _iInterface;
		bool _debugEnabled;


	};

}