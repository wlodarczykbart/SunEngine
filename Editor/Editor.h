#pragma once

#include "GraphicsWindow.h"
#include "GraphicsContext.h"
#include "Surface.h"
#include "ConfigFile.h"
#include "RenderTarget.h"
#include "GUIRenderer.h"
#include "View.h"

namespace SunEngine
{
	class Editor
	{
	public:
		struct CompiledShaderInfo
		{
			BaseShader::CreateInfo CreateInfo;
			String ConfigPath;
		};

		Editor();
		Editor(const Editor&) = delete;
		Editor& operator = (const Editor&) = delete;
		virtual ~Editor();

		bool Init(int argc, const char** argv);
		bool Run();

		bool CompileShader(CompiledShaderInfo& info, const String& name, const String& vertexText, const String& pixelText) const;
		bool CompileShader(CompiledShaderInfo& info, const String& name) const;
		uint GetViews(Vector<View*>& views) const;
		String GetPathFromConfig(const String& key) const;
		bool SelectFile(String& file, const String& fileTypeDescription, const String& fileTypeFilter) const;
	protected:

		virtual bool CustomInit(ConfigSection* pEditorConfig, GraphicsWindow* pWindow, GUIRenderer** ppOutGUI) = 0;
		virtual void CustomUpdate() = 0;

		void AddView(View* pView);
	private:

		void Update();
		void Render();

		bool _bInitialized;
		bool _bHasFocus;
		String _appDir;
		ConfigFile _config;
		GraphicsWindow _graphicsWindow;
		GraphicsContext _graphicsContext;
		Surface _graphicsSurface;
		UniquePtr<GUIRenderer> _guiRenderer;

		StrMap<UniquePtr<View>> _views;
	};
}
