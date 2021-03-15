#pragma once

#include "GUIRenderer.h"

namespace SunEngine
{
	class GameEditor;

	class GameEditorGUI : public GUIRenderer
	{
	public:
		GameEditorGUI();
		GameEditorGUI(const GameEditorGUI&) = delete;
		GameEditorGUI& operator = (const GameEditorGUI&) = delete;
		~GameEditorGUI();

	private:
		enum SecondaryWindowType
		{
			WT_MATERIAL,
			WT_MATERIAL_TEXTURE_PICKER,
			WT_ASSET_IMPORTER,

			SECONDARY_WINDOW_COUNT,
		};

		class UpdateMaterialTextureCommand;

		struct MaterialTexturePickerData
		{
			String MaterialName;
			String TextureName;
			Vector<char> FilterBuffer;
		};

		void CustomRender() override;

		void RenderMenu(GameEditor* pEditor);
		void RenderMaterials(GameEditor* pEditor);
		void RenderMaterialTexturePicker(GameEditor* pEditor);
		void RenderAssetImporter(GameEditor* pEditor);

		bool _visibleWindows[SECONDARY_WINDOW_COUNT];

		MaterialTexturePickerData _mtlTexturePicker;
	};
}
