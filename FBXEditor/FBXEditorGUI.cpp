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

	void EditColor(FbxPropertyT<FbxDouble3>& prop)
	{
		glm::vec3 color = FromFbxDouble(prop);
		if (ImGui::ColorEdit3(prop.GetName(), &color.x))
		{
			prop.Set(ToFbxDouble(color));
		}
	}

	void EditProperty(FbxPropertyT<FbxDouble>& prop)
	{
		auto fValue = float(prop.Get());
		if (ImGui::DragFloat(prop.GetName(), (float*)&fValue, 0.005f))
		{
			prop.Set(FbxDouble(fValue));
		}
	}

	void EditProperty(FbxPropertyT<FbxDouble3>& prop)
	{
		auto fValue = FromFbxDouble(prop.Get());
		if (ImGui::DragFloat3(prop.GetName(), (float*)&fValue, 0.005f))
		{
			prop.Set(ToFbxDouble(fValue));
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

				if (ImGui::TreeNode("Properties"))
				{
					if (mtl->Is<FbxSurfacePhong>())
					{
						FbxSurfacePhong* pPhong = (FbxSurfacePhong*)mtl;

						//! Diffuse color property.
						EditColor(pPhong->Diffuse);

						/** Diffuse factor property. This factor is used to
						 * attenuate the diffuse color.
						 */
						EditProperty(pPhong->DiffuseFactor);

						//! Specular color property.
						EditColor(pPhong->Specular);

						/** Specular factor property. This factor is used to
						 *  attenuate the specular color.
						 */
						EditProperty(pPhong->SpecularFactor);

						/** Shininess property. This property controls the aspect
						 *  of the shiny spot. It is the specular exponent in the Phong
						 *  illumination model.
						 */
						EditProperty(pPhong->Shininess);

						//! Transparent color property.
						EditColor(pPhong->TransparentColor);

						/** Transparency factor property.  This property is used to make a
						 * surface more or less opaque (0 = opaque, 1 = transparent).
						 */
						EditProperty(pPhong->TransparencyFactor);

						/** Reflection color property. This property is used to
						 * implement reflection mapping.
						 */
						EditColor(pPhong->Reflection);

						/** Reflection factor property. This property is used to
						 * attenuate the reflection color.
						 */
						EditProperty(pPhong->ReflectionFactor);

						EditColor(pPhong->Emissive);

						/** Emissive factor property. This factor is used to
						 *  attenuate the emissive color.
						 */
						EditProperty(pPhong->EmissiveFactor);

						//! Ambient color property.
						EditColor(pPhong->Ambient);

						/** Ambient factor property. This factor is used to
						 * attenuate the ambient color.
						 */
						EditProperty(pPhong->AmbientFactor);

						/** NormalMap property. This property can be used to specify the distortion of the surface
						 * normals and create the illusion of a bumpy surface.
						 */
						EditProperty(pPhong->NormalMap);

						/** Bump property. This property is used to distort the
						 * surface normal and create the illusion of a bumpy surface.
						 */
						EditProperty(pPhong->Bump);

						/** Bump factor property. This factor is used to
						 * make a surface more or less bumpy.
						 */
						EditProperty(pPhong->BumpFactor);

						//! Displacement color property.
						EditColor(pPhong->DisplacementColor);

						//! Displacement factor property.
						EditProperty(pPhong->DisplacementFactor);

						//! Vector displacement color property.
						EditColor(pPhong->VectorDisplacementColor);

						//! Vector displacement factor property.
						EditProperty(pPhong->VectorDisplacementFactor);


					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Textures"))
				{
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
									FbxTexture* pNewTexture = pScene->GetTexture(texName.data());
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
				ImGui::TreePop();
			}
		}
	}
}