#include "imgui.h"
#include "imgui_internal.h"
#include "StringUtil.h"
#include "View.h"
#include "Asset.h"
#include "GameEditor.h"
#include "ResourceMgr.h"
#include "GameEditorGUI.h"

//#define TEST_IMGUI_BASIC

namespace SunEngine
{
	GameEditorGUI::GameEditorGUI()
	{
		_renderMaterials = false;
		_mtlTexturePicker = {};


		_mtlTexturePicker.FilterBuffer.resize(2048, '\0');
	}

	GameEditorGUI::~GameEditorGUI()
	{
	}

	void GameEditorGUI::CustomRender()
	{
		RenderMenu();
		RenderMaterials();
		RenderMaterialTexturePicker();
	}

	void GameEditorGUI::RenderMenu()
	{
		static View* pSelView = 0;
		GameEditor* pEditor = static_cast<GameEditor*>(GetEditor());

		String importFileType = "";

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
					if (ImGui::MenuItem("obj")) importFileType = "obj";
					if (ImGui::MenuItem("fbx")) importFileType = "fbx";
					//if (ImGui::MenuItem("obj")) importFileType = ".obj";
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
					_renderMaterials = true;

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

		if (importFileType.length())
		{
			Asset* pAsset = 0;
			if (pEditor->ImportFromFileType(importFileType, pAsset))
			{

			}
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

	void GameEditorGUI::RenderMaterials()
	{
		float floatDrag = 0.005f;

		if (_renderMaterials && ImGui::Begin("Materials", &_renderMaterials))
		{
			auto iter = ResourceMgr::Get().IterMaterials();
			while (!iter.End())
			{
				Material* mtl = *iter;
				if (ImGui::TreeNode(mtl->GetName().c_str()))
				{
					if (ImGui::TreeNode("Variables"))
					{
						for (auto varIter = mtl->BeginVars(); varIter != mtl->EndVars(); ++varIter)
						{
							auto& var = (*varIter).second;
							switch (var.Type)
							{
							case SDT_FLOAT:
							{
								float value;
								mtl->GetMaterialVar(var.Name, value);
								if (ImGui::DragFloat(var.Name.c_str(), &value, floatDrag))
									mtl->SetMaterialVar(var.Name, value);
							}
							break;
							case SDT_FLOAT2:
							{
								glm::vec2 value;
								mtl->GetMaterialVar(var.Name, value);
								if (ImGui::DragFloat2(var.Name.c_str(), &value.x, floatDrag))
									mtl->SetMaterialVar(var.Name, value);
							}
							break;
							case SDT_FLOAT3:
							{
								glm::vec3 value;
								mtl->GetMaterialVar(var.Name, value);
								if (ImGui::DragFloat3(var.Name.c_str(), &value.x, floatDrag))
									mtl->SetMaterialVar(var.Name, value);
							}
							break;
							case SDT_FLOAT4:
							{
								glm::vec4 value;
								mtl->GetMaterialVar(var.Name, value);
								if (ImGui::DragFloat4(var.Name.c_str(), &value.x, floatDrag))
									mtl->SetMaterialVar(var.Name, value);
							}
							break;
							default:
								break;
							}
						}
						ImGui::TreePop();
					}

					if(ImGui::TreeNode("Textures"))
					{
						for (auto texIter = mtl->BeginTextures2D(); texIter != mtl->EndTextures2D(); ++texIter)
						{
							auto& tex = (*texIter).second;
							if (ImGui::Button(tex.Res.name.c_str()))
							{
								_mtlTexturePicker.RenderMaterialTexturePicker = true;
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
				++iter;
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

	void GameEditorGUI::RenderMaterialTexturePicker()
	{
		if (_mtlTexturePicker.RenderMaterialTexturePicker && ImGui::Begin("MaterialTexturePicker", &_mtlTexturePicker.RenderMaterialTexturePicker))
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

}