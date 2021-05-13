
#include "StringUtil.h"
#include "FileBase.h"
#include "ShaderCompiler.h"
#include "CommandBuffer.h"
#include "IDevice.h"

#include "glm/gtc/random.hpp"

#include "spdlog/spdlog.h"
#include "Editor.h"

#define DEFAULT_EDITOR_WIDTH 1200
#define DEFAULT_EDITOR_HEIGHT 800

namespace SunEngine
{
	const char* TexCopyVertexText =
		"struct PS_In\n"
		"{\n"
		"	float4 clipPos : SV_POSITION;\n"
		"	float2 texCoord : TEXCOORD;\n"
		"};\n"
		"\n"
		"static const float2 VERTS[] =\n"
		"{\n"
		"	float2(-1, -1),\n"
		"	float2(+1, -1),\n"
		"	float2(+1, +1),\n"
		"\n"
		"	float2(-1, -1),\n"
		"	float2(+1, +1),\n"
		"	float2(-1, +1)\n"
		"};\n"
		"\n"
		"PS_In main(uint vIndex : SV_VERTEXID)\n"
		"{\n"
		"	PS_In pIn;\n"
		"	pIn.clipPos = float4(VERTS[vIndex], 0.0, 1.0);\n"
		"	pIn.texCoord = VERTS[vIndex] * 0.5 + 0.5;\n"
		"	pIn.texCoord.y = 1.0 - pIn.texCoord.y;\n"
		"	return pIn;\n"
		"}\n"
		;

	const char* TexCopyPixelText =
		"struct PS_In\n"
		"{\n"
		"	float4 clipPos : SV_POSITION;\n"
		"	float2 texCoord : TEXCOORD;\n"
		"};\n"
		"\n"
		"Texture2D Texture;\n"
		"SamplerState Sampler;\n"
		"\n"
		"float4 main(PS_In pIn) : SV_TARGET\n"
		"{\n"
		"	return Texture.Sample(Sampler, pIn.texCoord);\n"
		"}\n"
		;

	Editor::Editor()
	{
		_bInitialized = false;
		_bHasFocus = false;
	}

	Editor::~Editor()
	{

	}

	bool Editor::Init(int argc, const char** argv)
	{
		if (argc < 2)
			return false;

		_appDir = argv[0];
		_appDir = GetDirectory(_appDir) + "/";

		String configPath;

		for (int i = 0; i < argc; i++)
		{
			String arg = argv[i];
			if (StrStartsWith(arg, "-config"))
			{
				configPath = arg.substr(sizeof("-config") - 1);
			}
		}

		if (!_config.Load(configPath))
		{
			spdlog::error("Failed to load editor config file: {}", configPath);
			return false;
		}

		if (!CustomParseConfig(&_config))
		{
			spdlog::error("Failed to parse config file correct: {}", configPath);
			return false;
		}

		ConfigSection* editorSection = _config.GetSection("Editor");
		if (editorSection == NULL)
		{
			spdlog::error("Failed to find [Editor] section within config file: {}", configPath);
			return false;
		}

		GraphicsWindow::CreateInfo windowInfo = {};
		windowInfo.title = editorSection->GetString("Title", "SunEngine Editor");
		windowInfo.width = editorSection->GetInt("Width", DEFAULT_EDITOR_WIDTH);
		windowInfo.height = editorSection->GetInt("Height", DEFAULT_EDITOR_HEIGHT);
		windowInfo.windowStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;
		if (!_graphicsWindow.Create(windowInfo))
		{
			spdlog::error("Failed to create GraphicsWindow based on config file: {}", configPath);
			return false;
		}

		GraphicsContext::CreateInfo contextInfo = {};
		contextInfo.debugEnabled = editorSection->GetString("Debug", "false") == "true";
		if (!_graphicsContext.Create(contextInfo))
		{
			spdlog::error("Failed to create GraphicsContext: {}", _graphicsContext.GetErrStr());
			return false;
		}

		if (!_graphicsSurface.Create(&_graphicsWindow))
		{
			spdlog::error("Failed to create GraphicsWindow");
			return false;
		}

		ShaderCompiler::SetAuxiliaryDir(GetPathFromConfig("Shaders"));


		GUIRenderer* pGUI = 0;
		if (!CustomLoad(&_graphicsWindow, &pGUI))
			return false;

		if (pGUI == 0)
		{
			spdlog::error("CustomInit failed to provide a valid GUI");
			return false;
		}

		_guiRenderer = UniquePtr<GUIRenderer>(pGUI);
		if (!_guiRenderer->Init(this, &_graphicsWindow))
		{
			spdlog::error("Failed to init GUIRenderer");
			return false;
		}

		View* pGraphicsWindowView = 0;
		for (auto iter = _views.begin(); iter != _views.end(); ++iter)
		{
			View* pView = (*iter).second.get();
			_guiRenderer->RegisterView(pView);

			if (pView->GetRenderToGraphicsWindow())
				pGraphicsWindowView = pView;
		}

		if (pGraphicsWindowView)
		{
			if (!CreateTextureCopyData(pGraphicsWindowView))
			{
				spdlog::error("Failed to create Texture Copy Data");
				return false;
			}
		}

		spdlog::info("Editor initialization succesfull");
		_bInitialized = true;
		return true;
	}

	bool Editor::Run()
	{
		if (!_bInitialized)
			return false;

		_graphicsWindow.Open();

		while (_graphicsWindow.IsAlive())
		{
			Update();
			Render();
		}

		GraphicsContext::GetDevice()->WaitIdle();
		_graphicsWindow.Destroy();
		return true;
	}

	void Editor::AddView(View* pView)
	{
		_views[pView->GetName()] = UniquePtr<View>(pView);
	}

	bool Editor::CreateTextureCopyData(View* pGraphicsWindowView)
	{
		ShaderCompiler shaderCompiler;
		shaderCompiler.SetVertexShaderSource(TexCopyVertexText);
		shaderCompiler.SetPixelShaderSource(TexCopyPixelText);

		if (!shaderCompiler.Compile())
			return false;

		if (!_shader.Create(shaderCompiler.GetCreateInfo()))
			return false;

		GraphicsPipeline::CreateInfo pipelineInfo = {};
		pipelineInfo.pShader = &_shader;
		pipelineInfo.settings.EnableAlphaBlend();
		pipelineInfo.settings.rasterizer.enableScissor = true;
		pipelineInfo.settings.rasterizer.cullMode = SE_CM_NONE;
		pipelineInfo.settings.rasterizer.frontFace = SE_FF_COUNTER_CLOCKWISE;
		//pipelineInfo.settings.depthClipEnable = true;
		//pipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_ALWAYS;
		if (!_pipeline.Create(pipelineInfo))
			return false;

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.pShader = &_shader;
		bindingInfo.type = SBT_MATERIAL;

		if (!_bindings.Create(bindingInfo)) return false;
		if (!_bindings.SetTexture("Texture", pGraphicsWindowView->GetRenderTarget()->GetColorTexture())) return false;
		//if (!_fontBinding.SetTexture("Texture", GraphicsContext::GetDefaultTexture(GraphicsContext::DT_WHITE))) return false;
		if (!_bindings.SetSampler("Sampler", GraphicsContext::GetDefaultSampler(GraphicsContext::DS_NEAR_CLAMP))) return false;

		return true;
	}

	uint Editor::GetViews(Vector<View*>& views) const
	{
		views.clear();
		for (auto iter = _views.begin(); iter != _views.end(); ++iter)
		{
			views.push_back((*iter).second.get());
		}
		return views.size();
	}

	String Editor::GetPathFromConfig(const String& key) const
	{
		const ConfigSection* paths = _config.GetSection("Paths");
		if (paths == NULL)
			return "";

		String path = paths->GetString(key);
		if (path.length() == 0)
			return "";

		return _appDir + path;
	}

	bool Editor::SelectFile(String& file, const String& fileTypeDescription, const String& fileTypeFilter) const
	{
		char filename[MAX_PATH] = {};

		char filter[512];
		uint buffIdx = 0;
		for (uint i = 0; i < fileTypeDescription.length(); i++) filter[buffIdx++] = fileTypeDescription[i];
		filter[buffIdx++] = '\0';
		for(uint i = 0; i < fileTypeFilter.length(); i++) filter[buffIdx++] = fileTypeFilter[i];
		filter[buffIdx++] = '\0';
		
		OPENFILENAME ofn;
		ZeroMemory(&filename, sizeof(filename));
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = filename;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = "Select a File, yo!";
		ofn.Flags = OFN_DONTADDTORECENT;// | OFN_FILEMUSTEXIST;

		if (GetOpenFileNameA(&ofn))
		{
			file = filename;
			return true;
		}
		else
		{
			return false;
		}
	}

	void Editor::Update()
	{
		GWEventData* pEvents;
		uint nEvents;

		_graphicsWindow.Update(&pEvents, nEvents);

		for (uint i = 0; i < nEvents; i++)
		{
			if (pEvents[i].type == GWE_MOUSE_DOWN && pEvents[i].keyCode == KEY_ESCAPE)
				_graphicsWindow.Close();
			if (pEvents[i].type == GWE_FOCUS_SET)
				_bHasFocus = true;
			if (pEvents[i].type == GWE_FOCUS_LOST)
				_bHasFocus = false;
		}


		//start here since this is when gpu is blocked for completion of previous frame, could think of better way to do this
		_graphicsSurface.StartFrame();

		//if (_bHasFocus)
		{
			CustomUpdate();

			for (auto iter = _views.begin(); iter != _views.end(); ++iter)
			{
				View* pView = (*iter).second.get();

				if (pView->GetRenderToGraphicsWindow())
					pView->UpdateViewState(glm::vec2((float)_graphicsWindow.Width(), (float)_graphicsWindow.Height()), glm::vec2(0.0f), true, true);

				bool resizing = pView->NeedsResize();
				pView->Update(&_graphicsWindow, pEvents, nEvents, 1 / 60.0f, 0.0f);
				if (resizing)
				{
					if (pView->GetRenderToGraphicsWindow())
						_bindings.SetTexture("Texture", pView->GetRenderTarget()->GetColorTexture());
					else
						_guiRenderer->UpdateView(pView);
				}
			}
			_guiRenderer->Update(pEvents, nEvents);
		}
	}

	void Editor::Render()
	{
		CommandBuffer* cmdBuffer = _graphicsSurface.GetCommandBuffer();

		View* pGraphicsWindowView = 0;
		for (auto iter = _views.begin(); iter != _views.end(); ++iter)
		{
			View* pView = (*iter).second.get();
			bool result = pView->Render(cmdBuffer);
			assert(result);

			if (pView->GetRenderToGraphicsWindow())
				pGraphicsWindowView = pView;
		}

		_graphicsSurface.Bind(cmdBuffer);
		if (pGraphicsWindowView)
		{
			_shader.Bind(cmdBuffer);
			_pipeline.Bind(cmdBuffer);
			_bindings.Bind(cmdBuffer);
			cmdBuffer->Draw(6, 1, 0, 0);
			_bindings.Unbind(cmdBuffer);
			_pipeline.Unbind(cmdBuffer);
			_shader.Unbind(cmdBuffer);
		}
		_guiRenderer->Render(cmdBuffer);
		_graphicsSurface.Unbind(cmdBuffer);

		_graphicsSurface.EndFrame();
	}
}

