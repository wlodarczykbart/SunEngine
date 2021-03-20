#include "imgui.h"
#include "imgui_internal.h"
#include "StringUtil.h"
#include "View.h"
#include "Asset.h"
#include "GameEditor.h"
#include "ResourceMgr.h"
#include "AssetImporter.h"
#include "GameEditorGUI.h"

//#define TEST_IMGUI_BASIC

namespace SunEngine
{
	GameEditorGUI::GameEditorGUI()
	{
		_mtlTexturePicker = {};
		_mtlTexturePicker.FilterBuffer.resize(2048, '\0');

		memset(_visibleWindows, 0x0, sizeof(_visibleWindows));
	}

	GameEditorGUI::~GameEditorGUI()
	{
	}

	void GameEditorGUI::CustomRender()
	{
		GameEditor* pEditor = static_cast<GameEditor*>(GetEditor());

		RenderMenu(pEditor);
		RenderMaterials(pEditor);
		RenderMaterialTexturePicker(pEditor);
		RenderAssetImporter(pEditor);
	}

	void GameEditorGUI::RenderMenu(GameEditor* pEditor)
	{
		static View* pSelView = 0;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::MenuItem("(demo menu)", NULL, false, false);
				if (ImGui::MenuItem("New")) {}
				if (ImGui::MenuItem("Open", "Ctrl+O")) {}
				if (ImGui::BeginMenu("Open Recent"))
				{
					ImGui::MenuItem("fish_hat.c");
					ImGui::MenuItem("fish_hat.inl");
					ImGui::MenuItem("fish_hat.h");
					if (ImGui::BeginMenu("More.."))
					{
						ImGui::MenuItem("Hello");
						ImGui::MenuItem("Sailor");
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Save", "Ctrl+S")) {}
				if (ImGui::MenuItem("Save As..")) {}

				ImGui::Separator();
				if (ImGui::BeginMenu("Import"))
				{
					if (ImGui::MenuItem("External Asset"))
						_visibleWindows[WT_ASSET_IMPORTER] = true;

					ImGui::EndMenu();
				}


				ImGui::EndMenu();
			}


			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}

				if (ImGui::MenuItem("Materials")) 
					_visibleWindows[WT_MATERIAL] =  true;

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Views"))
			{
				Vector<View*> views;
				for (uint i = 0; i < pEditor->GetViews(views); i++)
				{
					View* pView = views[i];
					if (ImGui::BeginMenu(pView->GetName().c_str()))
					{
						if (ImGui::MenuItem("ClearColor")) pSelView = pView;
						ImGui::EndMenu();
					}
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (pSelView)
		{
			//RenderTargetData& data = (*clearColIter).second;
			//if (ImGui::Begin("ViewClearColor"))
			//{
			//	float cc[4];
			//	data.pTarget->GetClearColor(cc[0], cc[1], cc[2], cc[3]);
			//	ImGui::ColorEdit3("ViewClearColor", cc);
			//	data.pTarget->SetClearColor(cc[0], cc[1], cc[2], cc[3]);
			//	ImGui::End();
			//}
		}
	}

	bool EditMaterialProperty(Material* mtl, const String& name, ShaderDataType dataType)
	{
		Shader* pShader = mtl->GetShader();
		if (!pShader)
			return false;

		const ConfigSection* pConfig = pShader->GetConfigSection("Editor");
		if (!pConfig)
			return false;

		OrderedStrMap<String> options;
		if (!pConfig->GetBlock(name.c_str(), options))
			return false;

		auto type = options.find("ColorEdit");
		if (type != options.end())
		{
			switch (dataType)
			{
			case SunEngine::SDT_FLOAT3:
			{
				glm::vec3 color;
				mtl->GetMaterialVar(name, color);
				if (ImGui::ColorEdit3(name.c_str(), &color.x, ImGuiColorEditFlags_Float))
					mtl->SetMaterialVar(name.c_str(), color);
			}
			break;
			case SunEngine::SDT_FLOAT4:
			{
				glm::vec4 color;
				mtl->GetMaterialVar(name, color);
				if (ImGui::ColorEdit4(name.c_str(), &color.x, ImGuiColorEditFlags_Float))
					mtl->SetMaterialVar(name.c_str(), color);
			}
			break;
			default:
				return false;
			}
			return true;
		}

		return false;
	}

	void GameEditorGUI::RenderMaterials(GameEditor* pEditor)
	{
		float floatDrag = 0.0005f;
		const char* floatFmt = "%.4f";

		bool& state = _visibleWindows[WT_MATERIAL];
		if (state && ImGui::Begin("Materials", &state))
		{
			for(auto iter = ResourceMgr::Get().IterMaterials(); !iter.End(); ++iter)
			{
				Material* mtl = *iter;
				if (ImGui::TreeNode(mtl->GetName().c_str()))
				{
					if (ImGui::TreeNode("Variables"))
					{
						for (auto varIter = mtl->BeginVars(); varIter != mtl->EndVars(); ++varIter)
						{
							auto& var = (*varIter).second;

							if (!EditMaterialProperty(mtl, var.name, var.type))
							{
								switch (var.type)
								{
								case SDT_FLOAT:
								{
									float value;
									mtl->GetMaterialVar(var.name, value);
									if (ImGui::DragFloat(var.name, &value, floatDrag, 0.0f, 0.0f, floatFmt))
										mtl->SetMaterialVar(var.name, value);
								}
								break;
								case SDT_FLOAT2:
								{
									glm::vec2 value;
									mtl->GetMaterialVar(var.name, value);
									if (ImGui::DragFloat2(var.name, &value.x, floatDrag, 0.0f, 0.0f, floatFmt))
										mtl->SetMaterialVar(var.name, value);
								}
								break;
								case SDT_FLOAT3:
								{
									glm::vec3 value;
									mtl->GetMaterialVar(var.name, value);
									if (ImGui::DragFloat3(var.name, &value.x, floatDrag, 0.0f, 0.0f, floatFmt))
										mtl->SetMaterialVar(var.name, value);
								}
								break;
								case SDT_FLOAT4:
								{
									glm::vec4 value;
									mtl->GetMaterialVar(var.name, value);
									if (ImGui::DragFloat4(var.name, &value.x, floatDrag))
										mtl->SetMaterialVar(var.name, value);
								}
								break;
								default:
									break;
								}
							}
						}
						ImGui::TreePop();
					}

					if(ImGui::TreeNode("Textures"))
					{
						for (auto texIter = mtl->BeginTextures2D(); texIter != mtl->EndTextures2D(); ++texIter)
						{
							auto& tex = (*texIter).second;
							if (ImGui::Button(tex.Res.name))
							{
								_visibleWindows[WT_MATERIAL_TEXTURE_PICKER] = true;
								_mtlTexturePicker.MaterialName = mtl->GetName();
								_mtlTexturePicker.TextureName = tex.Res.name;
								_mtlTexturePicker.FilterBuffer[0] = '\0';
							}
							if (tex.pTexture) 
							{
								ImGui::SameLine();
								ImGui::Text(tex.pTexture->GetName().c_str());
							}
						}
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}
			ImGui::End();
		}
	}

	class GameEditorGUI::UpdateMaterialTextureCommand : public UpdateCommand
	{
	public:
		UpdateMaterialTextureCommand(const String& material, const String& textureName, const String& texture)
		{
			Material = material;
			TextureName = textureName;
			Texture = texture;
		}

		bool Execute() override
		{
			auto mtl = ResourceMgr::Get().GetMaterial(Material);
			if (!mtl)
				return false;
			auto tex = ResourceMgr::Get().GetTexture2D(Texture);
			if (!tex)
				return false;
			return mtl->SetTexture2D(TextureName, tex);
		}

	private:
		String Material;
		String TextureName;
		String Texture;
	};

	void GameEditorGUI::RenderMaterialTexturePicker(GameEditor* pEditor)
	{
		bool& state = _visibleWindows[WT_MATERIAL_TEXTURE_PICKER];
		if (state && ImGui::Begin("MaterialTexturePicker", &state))
		{
			auto& resMgr = ResourceMgr::Get();
			auto mtl = resMgr.GetMaterial(_mtlTexturePicker.MaterialName);
			if (mtl)
			{
				ImGui::Text(_mtlTexturePicker.TextureName.c_str());	
				auto currTex = mtl->GetTexture2D(_mtlTexturePicker.TextureName);
				if (currTex)
				{
					ImGui::SameLine();
					ImGui::Text(currTex->GetName().c_str());
				}

				ImGui::InputText("Filter", _mtlTexturePicker.FilterBuffer.data(), _mtlTexturePicker.FilterBuffer.size());
				String strFilter = StrToLower(_mtlTexturePicker.FilterBuffer.data());

				auto texIter = resMgr.IterTextures2D();
				while (!texIter.End())
				{
					auto pTex = *texIter;
					if (strFilter.length() == 0 || StrToLower(pTex->GetName()).find(strFilter) != String::npos)
					{
						if (ImGui::Button(pTex->GetName().c_str()))
						{
							PushUpdateCommand(new UpdateMaterialTextureCommand(mtl->GetName(), _mtlTexturePicker.TextureName, pTex->GetName()));
						}
					}
					++texIter;
				}

			}
			ImGui::End();
		}
	}

	void GameEditorGUI::RenderAssetImporter(GameEditor* pEditor)
	{
		bool& state = _visibleWindows[WT_ASSET_IMPORTER];
		if (state && ImGui::Begin("AssetImporter", &state))
		{
			static AssetImporter::Options opt = AssetImporter::Options::Default;

			ImGui::Checkbox("Combine Materials", &opt.CombineMaterials);

			static char filePath[512] = {};
			ImGui::Text(filePath);
			if (ImGui::Button("Select File"))
			{
				String strFile;
				if (GetEditor()->SelectFile(strFile, "3D File", "*.fbx;*.obj"))
					strncpy_s(filePath, strFile.c_str(), strFile.length());
			}

			if (ImGui::Button("Import"))
			{
				if (pEditor->ImportAsset(filePath, opt))
					state = false;
			}

			ImGui::End();
		}
	}

}