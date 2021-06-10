#define XR_USE_PLATFORM_WIN32
#include <Windows.h>
#include <openxr/openxr_platform.h>

#include "IVRInterface.h"
#include "GraphicsContext.h"
#include "IDevice.h"
#include "RenderTarget.h"
#include "BaseTexture.h"
#include "ITexture.h"
#include "StringUtil.h"
#include "CommandBuffer.h"

#include "VRInterface.h"

const XrPosef XR_POSE_IDENTITY = { {0,0,0,1}, {0,0,0} };

namespace SunEngine
{
	class VRInterface::Impl
	{
	public:
		struct SwapImage
		{
			BaseTexture texture;
			RenderTarget target;
			CommandBuffer cmdBuffer;
		};

		struct Swapchain
		{
			uint width;
			uint height;
			XrSwapchain handle;
			Vector<UniquePtr<SwapImage>> images;
			XrFovf fov;
			XrPosef pose;
			uint imgIndex;
		};

		Impl(VRInterface* pThis)
		{
			_pThis = pThis;
			_instance = XR_NULL_HANDLE;
			_systemID = {};
			_session = XR_NULL_HANDLE;
			_appSpace = XR_NULL_HANDLE;
			_blendMode = {};
			_sessionState = {};
			_frameState = {};
			_debugUtilsMesenger = XR_NULL_HANDLE;
		}

		bool Create(const CreateInfo info)
		{
			Vector<const char*> extensions;
			if (info.debugEnabled) extensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
			extensions.push_back(_pThis->_iInterface->GetExtensionName());

			uint extCount = 0;
			XR_RETURN_ON_FAIL(xrEnumerateInstanceExtensionProperties(0, 0, &extCount, 0));
			Vector<XrExtensionProperties> extensionProperties(extCount, { XR_TYPE_EXTENSION_PROPERTIES });
			XR_RETURN_ON_FAIL(xrEnumerateInstanceExtensionProperties(0, extCount, &extCount, extensionProperties.data()));

			for (uint i = 0; i < extensions.size(); i++)
			{
				XrExtensionProperties* pFoundExt = Find<XrExtensionProperties>(extensionProperties, extensions[i], [](const XrExtensionProperties& val, const void* pData) -> bool {
					return strcmp(val.extensionName, (const char*)pData) == 0;
				});

				if (!pFoundExt)
					return false;
			}

			XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
			createInfo.enabledExtensionCount = extensions.size();
			createInfo.enabledExtensionNames = extensions.data();
			createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
			strcpy_s(createInfo.applicationInfo.applicationName, "VRApp");
			XR_RETURN_ON_FAIL(xrCreateInstance(&createInfo, &_instance));

			if (info.debugEnabled)
			{
				XrDebugUtilsMessengerCreateInfoEXT debugInfo = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
				debugInfo.messageTypes =
					XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
					XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
				debugInfo.messageSeverities =
					XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
					XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
					XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
					XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
				debugInfo.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT* msg, void* pData) {
					// Print the debug message we got! There's a bunch more info we could
					// add here too, but this is a pretty good start, and you can always
					// add a breakpoint this line!
					static_cast<VRInterface*>(pData)->_errStr = StrFormat("%s: %s\n", msg->functionName, msg->message);
					// Returning XR_TRUE here will force the calling function to fail
					return (XrBool32)XR_FALSE;
				};
				debugInfo.userData = _pThis;

				PFN_xrCreateDebugUtilsMessengerEXT CreateDebugFunc = 0;
				XR_RETURN_ON_FAIL(xrGetInstanceProcAddr(_instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&CreateDebugFunc)));
				XR_RETURN_ON_FAIL(CreateDebugFunc(_instance, &debugInfo, &_debugUtilsMesenger));
			}

			XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
			systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY; //Should this be configurable?
			XR_RETURN_ON_FAIL(xrGetSystem(_instance, &systemInfo, &_systemID));

			XrBaseInStructure* graphicsBinding = 0;
			XrSwapchainImageBaseHeader* pImageHeaders = 0;

			IVRInitInfo initInfo = {};
			initInfo.inInstance = VRHandle(_instance);
			initInfo.inSystemID = VRHandle(_systemID);
			initInfo.inGetInstanceProcAddr = xrGetInstanceProcAddr;
			initInfo.inDebugEnabled = info.debugEnabled;
			if (!_pThis->_iInterface->Init(initInfo))
				return false;

			graphicsBinding = (XrBaseInStructure*)initInfo.outBinding;
			pImageHeaders = (XrSwapchainImageBaseHeader*)initInfo.outImageHeaders;

			XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
			sessionInfo.systemId = _systemID;
			sessionInfo.next = graphicsBinding;
			XR_RETURN_ON_FAIL(xrCreateSession(_instance, &sessionInfo, &_session));

			XrReferenceSpaceCreateInfo refSpace = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
			refSpace.poseInReferenceSpace = XR_POSE_IDENTITY;
			refSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
			XR_RETURN_ON_FAIL(xrCreateReferenceSpace(_session, &refSpace, &_appSpace));

			//left/right eye based headset
			XrViewConfigurationType viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

			uint blendCount;
			XR_RETURN_ON_FAIL(xrEnumerateEnvironmentBlendModes(_instance, _systemID, viewConfigType, 1, &blendCount, &_blendMode));

			uint viewCount;
			XR_RETURN_ON_FAIL(xrEnumerateViewConfigurationViews(_instance, _systemID, viewConfigType, 0, &viewCount, 0));
			Vector<XrViewConfigurationView> configViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
			XR_RETURN_ON_FAIL(xrEnumerateViewConfigurationViews(_instance, _systemID, viewConfigType, viewCount, &viewCount, configViews.data()));

			if (viewCount == 0)
				return false;

			uint formatCount;
			XR_RETURN_ON_FAIL(xrEnumerateSwapchainFormats(_session, 0, &formatCount, 0));
			Vector<int64> swapchainFormats(formatCount);
			XR_RETURN_ON_FAIL(xrEnumerateSwapchainFormats(_session, formatCount, &formatCount, swapchainFormats.data()));

			int64 swapchainFormat = 0;
			for (uint i = 0; i < initInfo.outSupportedSwapchainFormats.size(); i++)
			{
				if (std::find(swapchainFormats.begin(), swapchainFormats.end(), initInfo.outSupportedSwapchainFormats[i]) != swapchainFormats.end())
				{
					swapchainFormat = initInfo.outSupportedSwapchainFormats[i];
					break;
				}
			}

			_swapchains.resize(viewCount);

			float ClearColors[4][4] =
			{
				{ 1.0f, 0.0f, 0.0f, 1.0f },
				{ 0.0f, 1.0f, 0.0f, 1.0f },
				{ 0.0f, 0.0f, 1.0f, 1.0f },
				{ 1.0f, 1.0f, 0.0f, 1.0f },
			};

			for (uint i = 0; i < viewCount; i++)
			{
				XrViewConfigurationView& config = configViews[i];

				Swapchain& swapchain = _swapchains[i];
				swapchain.width = config.recommendedImageRectWidth;
				swapchain.height = config.recommendedImageRectHeight;

				XrSwapchainCreateInfo swapchainInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
				swapchainInfo.arraySize = 1;
				swapchainInfo.faceCount = 1;
				swapchainInfo.mipCount = 1;
				swapchainInfo.width = config.recommendedImageRectWidth;
				swapchainInfo.height = config.recommendedImageRectHeight;
				swapchainInfo.sampleCount = config.recommendedSwapchainSampleCount;
				swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
				swapchainInfo.format = swapchainFormat;
				XR_RETURN_ON_FAIL(xrCreateSwapchain(_session, &swapchainInfo, &swapchain.handle));

				uint surfaceCount = 0;
				XR_RETURN_ON_FAIL(xrEnumerateSwapchainImages(swapchain.handle, 0, &surfaceCount, 0));
				XR_RETURN_ON_FAIL(xrEnumerateSwapchainImages(swapchain.handle, surfaceCount, &surfaceCount, pImageHeaders));

				swapchain.images.resize(surfaceCount);
				for (uint j = 0; j < surfaceCount; j++)
				{
					SwapImage* pSwapImage = new SwapImage();

					BaseTexture::CreateInfo texInfo = {};
					texInfo.image.Width = swapchainInfo.width;
					texInfo.image.Height = swapchainInfo.height;
					texInfo.isExternal = true;
					if (!pSwapImage->texture.Create(texInfo))
						return false;

					if (!_pThis->_iInterface->InitTexture(pImageHeaders, j, swapchainFormat, static_cast<ITexture*>(pSwapImage->texture.GetAPIHandle())))
						return false;

					RenderTarget::CreateInfo targetInfo = {};
					targetInfo.width = swapchainInfo.width;
					targetInfo.height = swapchainInfo.height;
					targetInfo.numTargets = 1;
					targetInfo.pSharedColorBuffers[0] = &pSwapImage->texture;
					targetInfo.hasDepthBuffer = true;
					
					if (!pSwapImage->target.Create(targetInfo))
						return false;

					pSwapImage->target.SetClearColor(ClearColors[i][0], ClearColors[i][1], ClearColors[i][2], ClearColors[i][3]);

					pSwapImage->cmdBuffer.Create();
					swapchain.images[j] = UniquePtr<SwapImage>(pSwapImage);
				}
			}

			return true;
		}

		bool Update()
		{
			XrEventDataBuffer eventBuffer = { XR_TYPE_EVENT_DATA_BUFFER };	
			while (xrPollEvent(_instance, &eventBuffer) == XR_SUCCESS)
			{
				switch (eventBuffer.type)
				{
				case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
				{
					XrEventDataSessionStateChanged* changed = (XrEventDataSessionStateChanged*)&eventBuffer;
					_sessionState = changed->state;
					switch (_sessionState)
					{
					case XR_SESSION_STATE_UNKNOWN:
						break;
					case XR_SESSION_STATE_IDLE:
						break;
					case XR_SESSION_STATE_READY:
					{
						XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
						beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
						XR_RETURN_ON_FAIL(xrBeginSession(_session, &beginInfo));
					}
						break;
					case XR_SESSION_STATE_SYNCHRONIZED:
						break;
					case XR_SESSION_STATE_VISIBLE:
						break;
					case XR_SESSION_STATE_FOCUSED:
						break;
					case XR_SESSION_STATE_STOPPING:
						XR_RETURN_ON_FAIL(xrEndSession(_session));
						break;
					case XR_SESSION_STATE_LOSS_PENDING:
						break;
					case XR_SESSION_STATE_EXITING:
						break;
					case XR_SESSION_STATE_MAX_ENUM:
						break;
					default:
						break;
					}
				}
					break;
				default:
					break;
				}
				eventBuffer = { XR_TYPE_EVENT_DATA_BUFFER };
			}

			return true;
		}

		bool StartFrame()
		{
			if (!ShouldDoFrame())
				return true;

			XR_RETURN_ON_FAIL(xrWaitFrame(_session, 0, &_frameState));

			XR_RETURN_ON_FAIL(xrBeginFrame(_session, 0));

			if (ShouldRender())
			{
				XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
				locateInfo.displayTime = _frameState.predictedDisplayTime;
				locateInfo.space = _appSpace;
				locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

				XrViewState viewState = { XR_TYPE_VIEW_STATE };
				uint viewCount;
				XrView views[16];
				XR_RETURN_ON_FAIL(xrLocateViews(_session, &locateInfo, &viewState, _swapchains.size(), &viewCount, views));

				for (uint i = 0; i < _swapchains.size(); i++)
				{
					XrView& view = views[i];
					Swapchain& swapchain = _swapchains[i];
					swapchain.pose = view.pose;
					swapchain.fov = view.fov;
				}
			}
			return true;
		}

		bool StartSwapchain(uint index, SwapchainRenderInfo& renderInfo)
		{
			renderInfo.shouldRender = false;
			if (index >= _swapchains.size())
				return false;

			if (!ShouldRender())
				return true;

			Swapchain& swapchain = _swapchains[index];

			renderInfo.fov.left = swapchain.fov.angleLeft;
			renderInfo.fov.right = swapchain.fov.angleRight;
			renderInfo.fov.up = swapchain.fov.angleUp;
			renderInfo.fov.down = swapchain.fov.angleDown;

			renderInfo.orientation.w = swapchain.pose.orientation.w;
			renderInfo.orientation.x = swapchain.pose.orientation.x;
			renderInfo.orientation.y = swapchain.pose.orientation.y;
			renderInfo.orientation.z = swapchain.pose.orientation.z;

			renderInfo.position.x = swapchain.pose.position.x;
			renderInfo.position.y = swapchain.pose.position.y;
			renderInfo.position.z = swapchain.pose.position.z;

			XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
			XR_RETURN_ON_FAIL(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &swapchain.imgIndex));

			_pThis->Bind(&swapchain.images[swapchain.imgIndex]->cmdBuffer);

			XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
			waitInfo.timeout = XR_INFINITE_DURATION;
			XR_RETURN_ON_FAIL(xrWaitSwapchainImage(swapchain.handle, &waitInfo));

			renderInfo.pRenderTarget = &swapchain.images[swapchain.imgIndex]->target;
			renderInfo.pCmdBuffer = &swapchain.images[swapchain.imgIndex]->cmdBuffer;
			renderInfo.shouldRender = true;
			return true;
		}

		bool EndSwapchain(uint index)
		{
			if (index >= _swapchains.size())
				return false;

			if (!ShouldRender())
				return true;

			Swapchain& swapchain = _swapchains[index];

			_pThis->Unbind(&swapchain.images[swapchain.imgIndex]->cmdBuffer);

			XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
			XR_RETURN_ON_FAIL(xrReleaseSwapchainImage(swapchain.handle, &releaseInfo));		
			return true;
		}

		bool EndFrame()
		{
			if (!ShouldDoFrame())
				return true;

			XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
			endInfo.displayTime = _frameState.predictedDisplayTime;
			endInfo.environmentBlendMode = _blendMode;
			XrCompositionLayerBaseHeader* pLayerPtr = 0;
			
			if (ShouldRender())
			{
				static XrCompositionLayerProjectionView views[16];
				static XrCompositionLayerProjection layerProj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
				layerProj.space = _appSpace;
				layerProj.viewCount = _swapchains.size();
				layerProj.views = views;

				for (uint i = 0; i < layerProj.viewCount; i++)
				{
					views[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
					views[i].pose = _swapchains[i].pose;
					views[i].fov = _swapchains[i].fov;
					views[i].subImage.swapchain = _swapchains[i].handle;
					views[i].subImage.imageRect.offset = { 0, 0 };
					views[i].subImage.imageRect.extent = { (int)_swapchains[i].width, (int)_swapchains[i].height };
				}

				pLayerPtr = (XrCompositionLayerBaseHeader*)&layerProj;;
				endInfo.layerCount = 1;
			}

			endInfo.layers = &pLayerPtr;
			XR_RETURN_ON_FAIL(xrEndFrame(_session, &endInfo));
			return true;
		}

		inline bool ShouldDoFrame() const
		{
			return _sessionState == XR_SESSION_STATE_READY || _sessionState == XR_SESSION_STATE_VISIBLE || _sessionState == XR_SESSION_STATE_FOCUSED;
		}

		inline bool ShouldRender() const
		{
			return _frameState.shouldRender && (_sessionState == XR_SESSION_STATE_VISIBLE || _sessionState == XR_SESSION_STATE_FOCUSED);
		}

		VRInterface* _pThis;
		XrInstance _instance;
		XrSystemId _systemID;
		XrSession _session;
		XrSpace _appSpace;
		XrEnvironmentBlendMode _blendMode;
		XrSessionState _sessionState;
		XrFrameState _frameState;
		XrDebugUtilsMessengerEXT _debugUtilsMesenger;

		Vector<Swapchain> _swapchains;
	};

	VRInterface::VRInterface() : GraphicsObject(GraphicsObject::VR_INTERFACE)
	{
		_debugEnabled = false;
		_impl = UniquePtr<Impl>(new Impl(this));
		_iInterface = 0;
	}

	VRInterface::~VRInterface()
	{

	}

	bool VRInterface::Create(const CreateInfo& info)
	{
		if (!Destroy())
			return false;

		_iInterface = AllocateGraphics<IVRInterface>();

		_debugEnabled = info.debugEnabled;

		if (!_impl->Create(info))
			return false;

		return true;
	}

	IObject* VRInterface::GetAPIHandle() const 
	{
		return _iInterface;
	}

	bool VRInterface::Update()
	{
		return _impl->Update();
	}

	bool VRInterface::StartFrame()
	{
		return _impl->StartFrame();
	}

	bool VRInterface::StartSwapchain(uint swapchainIndex, SwapchainRenderInfo& renderInfo)
	{
		return _impl->StartSwapchain(swapchainIndex, renderInfo);
	}

	bool VRInterface::EndSwapchain(uint swapchainIndex)
	{
		return _impl->EndSwapchain(swapchainIndex);
	}

	bool VRInterface::EndFrame()
	{
		return _impl->EndFrame();
	}

	uint VRInterface::GetSwapchainCount() const
	{
		return _impl->_swapchains.size();
	}
}