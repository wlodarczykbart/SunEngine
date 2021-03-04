#pragma once

#include "FBXEditor.h"

namespace SunEngine
{
	class FBXEditor;

	class FBXEditorGUI : public GUIRenderer
	{
	public:

	private:
		void CustomRender() override;

		void ShowMaterials(FbxScene* pScene, FBXEditor* pEditor);
	};
}
