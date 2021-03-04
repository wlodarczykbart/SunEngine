#pragma once

#include "GUIRenderer.h"

namespace SunEngine
{
	class GameEditorGUI : public GUIRenderer
	{
	public:
		GameEditorGUI();
		GameEditorGUI(const GameEditorGUI&) = delete;
		GameEditorGUI& operator = (const GameEditorGUI&) = delete;
		~GameEditorGUI();

	private:
		class UpdateMaterialTextureCommand;

		struct MaterialTexturePickerData
		{
			bool RenderMaterialTexturePicker;
			String MaterialName;
			String TextureName;
			Vector<char> FilterBuffer;
		};

		void CustomRender() override;

		void RenderMenu();
		void RenderMaterials();
		void RenderMaterialTexturePicker();

		bool _renderMaterials;

		MaterialTexturePickerData _mtlTexturePicker;
	};
}
