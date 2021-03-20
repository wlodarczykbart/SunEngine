
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

		if (!_config.Load(configPath.data()))
		{
			spdlog::error("Failed to load editor config file: {}", configPath);
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

		String strGraphicsAPI = StrToLower(editorSection->GetString("Graphics", "Vulkan"));
		if (strGraphicsAPI == "vulkan")
		{
			SetGraphicsAPI(SE_GFX_VULKAN);
		}
		if (strGraphicsAPI == "d3d11")
		{
			SetGraphicsAPI(SE_GFX_D3D11);
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
		if (!CustomInit(editorSection, &_graphicsWindow, &pGUI))
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

		for (auto iter = _views.begin(); iter != _views.end(); ++iter)
		{
			_guiRenderer->RegisterView((*iter).second.get());
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

	bool Editor::CompileShader(CompiledShaderInfo& info, const String& name, const String& vertexText, const String& pixelText) const
	{
		ShaderCompiler compiler;

		String shaderOutputDir = _appDir + name + "/";
		CreateDirectory(shaderOutputDir.data(), NULL);

		if (vertexText.length())
		{
			String vertexPath = shaderOutputDir + name + ".vs";

			FileStream fw;
			if (!fw.OpenForWrite(vertexPath.data()))
				return false;

			if (!fw.Write(vertexText.data()))
				return false;

			if (!fw.Close())
				return false;

			compiler.SetVertexShaderPath(vertexPath);
		}

		if (pixelText.length())
		{
			String pixelPath = shaderOutputDir + name + ".ps";

			FileStream fw;
			if (!fw.OpenForWrite(pixelPath.data()))
				return false;

			if (!fw.Write(pixelText.data()))
				return false;

			if (!fw.Close())
				return false;

			compiler.SetPixelShaderPath(pixelPath);
		}

		if (!compiler.Compile())
		{
			spdlog::error("Shader Error: {}", compiler.GetLastError().data());
			return false;
		}

		info.CreateInfo = compiler.GetCreateInfo();
		info.ConfigPath.clear();
		return true;
	}

	bool Editor::CompileShader(CompiledShaderInfo& info, const String& name) const
	{
		String path = GetPathFromConfig("ShaderConfig");
		if (!path.length())
			return false;

		String shaderDir = GetPathFromConfig("Shaders");

		ConfigFile config;
		if (!config.Load(path.data()))
			return false;

		const ConfigSection* pSection = config.GetSection(name.data());
		if (pSection)
		{

			String vertexText, pixelText;

			FileStream fr;
			String shaderFile;

			shaderFile = shaderDir + pSection->GetString("vs");
			if (fr.OpenForRead(shaderFile.data()))
			{
				fr.ReadAllText(vertexText);
			}
			else
			{
				spdlog::error("Failed to read required vertex shader file: {}", shaderFile.data());
				return false;
			}

			shaderFile = shaderDir + pSection->GetString("ps");
			if (fr.OpenForRead(shaderFile.data()))
			{
				fr.ReadAllText(pixelText);
			}
			else
			{
				spdlog::error("Failed to read required pixel shader file: {}", shaderFile.data());
				return false;
			}

			info = {};
			if (!CompileShader(info, name, vertexText, pixelText))
			{
				return false;
			}

			String configPath = pSection->GetString("config");
			if (configPath.length())
			{
				info.ConfigPath = shaderDir + configPath;
			}

			return true;
		}
		else
		{
			return false;
		}
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

		String path = paths->GetString(key.data());
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
				(*iter).second->Update(&_graphicsWindow, pEvents, nEvents, 1 / 60.0f, 0.0f);
			}
			_guiRenderer->Update(pEvents, nEvents);
		}
	}

	void Editor::Render()
	{
		CommandBuffer* cmdBuffer = _graphicsSurface.GetCommandBuffer();

		for (auto iter = _views.begin(); iter != _views.end(); ++iter)
		{
			bool result = (*iter).second->Render(cmdBuffer);
			assert(result);
		}

		_graphicsSurface.Bind(cmdBuffer);
		_guiRenderer->Render(cmdBuffer);
		_graphicsSurface.Unbind(cmdBuffer);

		_graphicsSurface.EndFrame();
	}
}

