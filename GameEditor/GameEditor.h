#pragma once

#include "Editor.h"
#include "SceneRenderer.h"

namespace SunEngine
{
	class GameEditor : public Editor
	{
	public:
		GameEditor();
		GameEditor(const GameEditor&) = delete;
		GameEditor& operator = (const GameEditor&) = delete;
		~GameEditor();

		bool CustomInit(ConfigSection* pEditorConfig, GraphicsWindow* pWindow, GUIRenderer** ppOutGUI) override;
		bool ImportFromFileType(const String& fileType, Asset*& pAsset);
	private:
		Asset* ImportAsset(const String& filename);

		void CustomUpdate() override;
		bool CompileShaders();
		bool CreateDefaultScene();

		SceneRenderer _sceneRenderer;
		StrMap<CompiledShaderInfo> _compiledShaders;
		Asset* _queuedAsset;
	};
}
