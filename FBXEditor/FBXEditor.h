#pragma once

#include "Editor.h"
#include "fbxsdk.h"

namespace SunEngine
{
	glm::vec3 FromFbxDouble(const FbxDouble3& value);
	glm::vec2 FromFbxDouble(const FbxDouble2& value);

	FbxDouble3 ToFbxDouble(const glm::vec3& value);
	FbxDouble2 ToFbxDouble(const glm::vec2& value);

	class Asset;

	class FBXEditor : public Editor
	{
	public:
		FBXEditor();
		void DoImport() { _actionFlags |= AF_IMPORT; }
		void DoExport() { _actionFlags |= AF_EXPORT; }
		void DoPreview() { _actionFlags |= AF_PREVIEW; }

		FbxScene* GetScene() const { return _fbxScene; }

	private:
		enum ActionFlags
		{
			AF_IMPORT = 1 << 0,
			AF_EXPORT = 1 << 1,
			AF_PREVIEW = 1 << 2,
		};

		bool CustomInit(ConfigFile* pConfig, GraphicsWindow* pWindow, GUIRenderer** ppOutGUI) override;
		void CustomUpdate() override;

		bool Import(const String& path);
		bool Export(const String& path);

		class FBXView* _view;
		uint _actionFlags;

		FbxManager* _fbxManager;
		FbxImporter* _fbxImporter;
		FbxExporter* _fbxExporter;
		FbxScene* _fbxScene;

		Asset* _currentAsset;
		String _currentExportFile;

	};
}
