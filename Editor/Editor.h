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
		Editor();
		Editor(const Editor&) = delete;
		Editor& operator = (const Editor&) = delete;
		virtual ~Editor();

		bool Init(int argc, const char** argv);
		bool Run();

		uint GetViews(Vector<View*>& views) const;
		String GetPathFromConfig(const String& key) const;
		bool SelectFile(String& file, const String& fileTypeDescription, const String& fileTypeFilter) const;

		double GetUpdateTick() const { return _updateTick; }
		double GetRenderTick() const { return _renderTick; }
		double GetFrameTick() const { return _updateTick + _renderTick; }

		const Surface* GetSurface() const { return &_graphicsSurface; }
		const GraphicsWindow* GetGraphicsWindow() const { return &_graphicsWindow; }
	protected:

		virtual bool CustomParseConfig(ConfigFile* pConfig) = 0;
		virtual bool CustomLoad(GraphicsWindow* pWindow, GUIRenderer** ppOutGUI) = 0;
		virtual void CustomUpdate() = 0;

		const ConfigFile& GetConfig() const { return _config; }

		void AddView(View* pView);
	private:
		bool CreateTextureCopyData(View* pGraphicsWindowView);

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

		BaseShader _shader;
		GraphicsPipeline _pipeline;
		ShaderBindings _bindings;

		double _updateTick;
		double _renderTick;
	};
}
