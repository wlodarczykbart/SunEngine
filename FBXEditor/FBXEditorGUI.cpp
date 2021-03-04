#include "imgui.h"
#include "FBXImporter.h"
#include "StringUtil.h"
#include "FBXEditorGUI.h"

namespace SunEngine
{
	void FBXEditorGUI::CustomRender()
	{
		FBXEditor* pEditor = static_cast<FBXEditor*>(GetEditor());

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
				if (ImGui::MenuItem("Import"))
				{
					pEditor->DoImport();
				}
				if (ImGui::MenuItem("Export"))
				{
					pEditor->DoExport();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		FbxScene* pScene = pEditor->GetScene();
		if (pScene)
		{
			if (ImGui::Begin("Scene"))
			{
				if (ImGui::Button("Preview"))
				{
					pEditor->DoPreview();
				}

				if (ImGui::TreeNode("Materials"))
				{
					ShowMaterials(pScene, pEditor);
					ImGui::TreePop();
				}
				ImGui::End();
			}
		}
	}

	void FBXEditorGUI::ShowMaterials(FbxScene* pScene, FBXEditor* pEditor)
	{
		for (int i = 0; i < pScene->GetMaterialCount(); i++)
		{
			FbxSurfaceMaterial* mtl = pScene->GetMaterial(i);
			if (ImGui::TreeNode(StrFormat("%d. %s", i+1, mtl->GetName()).data()))
			{
				ImGui::Text("Shading Model: ");
				ImGui::SameLine();
				ImGui::Text(mtl->ShadingModel.Get().Buffer());
				if (mtl->Is<FbxSurfacePhong>())
				{
					FbxSurfacePhong* pPhong = (FbxSurfacePhong*)mtl;

					glm::vec3 Diffuse = FromFbxDouble(pPhong->Diffuse);
					if (ImGui::ColorEdit3("Diffuse", &Diffuse.x))
					{
						pPhong->Diffuse.Set(ToFbxDouble(Diffuse));
					}

					//FbxDouble3 ambientColor = pPhong->Ambient;
					//FbxDouble3 emissiveColor = pPhong->Emissive;

					//FbxDouble3 transparentColor = pPhong->TransparentColor;
					//FbxDouble transparency = (transparentColor[0] + transparentColor[1] + transparentColor[2]) / 3.0f;
					//pMat->Alpha = (float)(1.0 - transparency);

					glm::vec3 Specular = FromFbxDouble(pPhong->Specular);
					if (ImGui::ColorEdit3("Specular", &Specular.x))
					{
						pPhong->Specular.Set(ToFbxDouble(Specular));
					}

					float Shininess = (float)pPhong->Shininess;
					if (ImGui::DragFloat("Shininess", &Shininess, 0.05f))
					{
						pPhong->Shininess = Shininess;
					}
				}

				int layerIndex = 0;
				FBXSDK_FOR_EACH_TEXTURE(layerIndex)
				{
					String texType = FbxLayerElement::sTextureChannelNames[layerIndex];
					FbxProperty prop = mtl->FindProperty(texType.data());
					FbxTexture* pTexture = prop.GetSrcObject<FbxTexture>();

					//if (
					//	texType == ModelImporter::FBXImporter::FBX_DIFFUSE_MAP ||
					//	texType == ModelImporter::FBXImporter::FBX_NORMAL_MAP ||
					//	texType == ModelImporter::FBXImporter::FBX_SPECULAR_MAP ||
					//	texType == ModelImporter::FBXImporter::FBX_TRANSPARENT_MAP)
					{

						bool changeTex = ImGui::Button(StrFormat("%s", texType.c_str()).c_str());
						ImGui::SameLine();

						if (pTexture && pTexture->Is<FbxFileTexture>())
						{
							FbxFileTexture* pFileTex = (FbxFileTexture*)pTexture;
							String name = pFileTex->GetRelativeFileName();
							String full = pFileTex->GetFileName();
							ImGui::Text("%s", name.c_str());
						}
						//else if (pTexture && pTexture->Is<FbxLayeredTexture>())
						//{
						//	FbxLayeredTexture* pLayerTex = (FbxLayeredTexture*)pTexture;
						//	ImGui::Text("%s", pLayerTex->GetName());
						//}
						else
						{
							ImGui::Text("<- Select...");
						}

						if (changeTex)
						{
							String texPath;
							if (pEditor->SelectFile(texPath, "Image Types", "*.png;*.jpg;*.jpeg;*.tga;*.bmp"))
							{
								String texName = GetFileName(texPath);
								FbxTexture* pNewTexture =  pScene->GetTexture(texName.data());
								if (pNewTexture == 0)
								{
									FbxFileTexture* pFileTex = FbxFileTexture::Create(pScene->GetFbxManager(), texName.data());
									pFileTex->SetFileName(texPath.data());
									pFileTex->SetRelativeFileName(texName.data());
									pScene->AddTexture(pFileTex);
									pNewTexture = pFileTex;
								}

								if (pTexture)
								{
									bool disconnected = prop.DisconnectSrcObject(pTexture);
									assert(disconnected);
								}


								bool connected = prop.ConnectSrcObject(pNewTexture);
								assert(connected);

								//if (pTexture)
								//{
								//	bool disconnected = prop.DisconnectSrcObject(pTexture);
								//	assert(disconnected);
								//}

								//bool set = prop.Set(pNewTexture);
								//bool valid = prop.IsValid();
								//bool connected = prop.ConnectSrcObject(pNewTexture);
								//assert(connected);
							}
						}

					}
				}

				ImGui::TreePop();
			}
		}
	}
}