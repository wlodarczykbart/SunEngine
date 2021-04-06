#pragma once

#include "Editor.h"
#include "AssetImporter.h"
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

		bool CustomInit(ConfigFile* pConfig, GraphicsWindow* pWindow, GUIRenderer** ppOutGUI) override;
		Asset* ImportAsset(const String& filename, const AssetImporter::Options& options);
	private:

		void CustomUpdate() override;
		bool CreateDefaultScene();

		SceneRenderer _sceneRenderer;
		Asset* _queuedAsset;
	};
}
