#include <fbxsdk.h>
using namespace fbxsdk;

#include "FBXImporter.h"

#include <unordered_map>
#include <assert.h>

namespace ModelImporter
{
	const char* FBXImporter::FBX_DIFFUSE_MAP = "DiffuseColor";
	const char* FBXImporter::FBX_NORMAL_MAP = "NormalMap";
	const char* FBXImporter::FBX_SPECULAR_MAP = "SpecularColor";
	const char* FBXImporter::FBX_TRANSPARENT_MAP = "TransparentColor";

	const char* FBXImporter::FBX_AMBIENT_MAP = "AmbientColor";
	const char* FBXImporter::FBX_SPECUALR_FACTOR_MAP = "SpecularFactor";
	const char* FBXImporter::FBX_SHININESS_EXPONENT_MAP = "ShininessExponent";
	const char* FBXImporter::FBX_BUMP_MAP = "BumpMap";

	template<typename T>
	Mat4 ToImporterMtx(const T& otherMtx)
	{
		Mat4 mtx;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				mtx[i][j] = (float)otherMtx[i][j];
			}
		}

		return mtx;
	}

	class FBXImporter::FbxNodeHandle
	{
	public:
		FBXImporter::FbxNodeHandle(FbxNode* pNode)
		{
			_node = pNode;
		}

		FbxNode* _node;
	};

	class FBXImporter::FbxData
	{
	public:
		FbxData(FbxManager* manager) : _geomConv(manager)
		{
			_manager = manager;
		}

		FbxManager* _manager;
		FbxImporter* _importer;
		FbxScene* _scene;
		FbxGeometryConverter _geomConv;
	};

	FBXImporter::FBXImporter()
	{
	}


	FBXImporter::~FBXImporter()
	{

	}

	bool FBXImporter::DerivedImport()
	{
		_fbxData = new FbxData(FbxManager::Create());
		_fbxData->_importer = FbxImporter::Create(_fbxData->_manager, "Importer");

		if (!_fbxData->_importer->Initialize(_fileName.data()))
			return false;

		_fbxData->_scene = FbxScene::Create(_fbxData->_manager, "Scene");
		if (!_fbxData->_importer->Import(_fbxData->_scene))
			return false;

		// Convert Axis System to what is used in this example, if needed
		FbxAxisSystem SceneAxisSystem = _fbxData->_scene->GetGlobalSettings().GetAxisSystem();
		FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
		if (SceneAxisSystem != OurAxisSystem)
		{
			OurAxisSystem.ConvertScene(_fbxData->_scene);
		}

		// Convert Unit System to what is used in this example, if needed
		FbxSystemUnit SceneSystemUnit = _fbxData->_scene->GetGlobalSettings().GetSystemUnit();
		if (SceneSystemUnit.GetScaleFactor() != 1.0)
		{
			//The unit in this example is centimeter.
			FbxSystemUnit::cm.ConvertScene(_fbxData->_scene);
		}

		//tODO: move to function?
		int numMeshes = _fbxData->_scene->GetSrcObjectCount<FbxMesh>();
		for (int i = 0; i < numMeshes; i++)
		{
			FbxMesh* pFbxMesh = _fbxData->_scene->GetSrcObject<FbxMesh>(i);
			int numPolygons = pFbxMesh->GetPolygonCount();
			for (int p = 0; p < numPolygons; p++)
			{
				if (pFbxMesh->GetPolygonSize(p) > 4)
				{
					_fbxData->_geomConv.Triangulate(pFbxMesh, true);
					break;
				}
			}
		}

		CollectMaterials();
		InitAnimationData();
		Traverse(&FBXImporter::InitNodes);
		CollectMeshes();
		Traverse(&FBXImporter::CollectNodes);
		UpdateSkinningData();
		UpdateAnimations();
		//Traverse(&FBXImporter::CollectMeshes);

		_fbxData->_scene->Destroy();
		_fbxData->_importer->Destroy();
		_fbxData->_manager->Destroy();
		delete _fbxData;

		return true;
	}

	void FBXImporter::Traverse(FbxTraverseFunc func)
	{
		FbxNodeHandle handle(_fbxData->_scene->GetRootNode());
		TraverseStack data = {};
		Traverse(func, &handle, data);

	}

	void FBXImporter::Traverse(FbxTraverseFunc func, FbxNodeHandle* pHandle, TraverseStack data)
	{
		FbxNode* pNode = pHandle->_node;

		FbxAMatrix globalMtx = pNode->EvaluateGlobalTransform();
		FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
		FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
		FbxAMatrix geomMtx(lT, lR, lS);

		data.localMtx = ToImporterMtx(pNode->EvaluateLocalTransform());
		data.worldMtx = ToImporterMtx(globalMtx);
		data.geomOffset = ToImporterMtx(geomMtx);

		(this->*func)(pHandle, data);

		data.pParent = pHandle;
		int numChild = pNode->GetChildCount();
		for (int i = 0; i < numChild; i++)
		{
			FbxNodeHandle childHandle(pNode->GetChild(i));
			Traverse(func, &childHandle, data);
		}
	}

	void FBXImporter::PrintNames(FbxNodeHandle* pHandle, TraverseStack &)
	{
		printf("%s\n", pHandle->_node->GetName());
	}

	void FBXImporter::CollectMeshes()
	{
		int numMeshes = _fbxData->_scene->GetSrcObjectCount<FbxMesh>();
		for (int m = 0; m < numMeshes; m++)
		{
			FbxMesh* pFbxMesh = _fbxData->_scene->GetSrcObject<FbxMesh>(m);
		
			int nDeformers = pFbxMesh->GetDeformerCount();
			(void)nDeformers;

			FbxSkin* pSkin = 0;
			for (int d = 0; d < pFbxMesh->GetDeformerCount(); d++)
			{
				if (pFbxMesh->GetDeformer(d)->GetDeformerType() == FbxDeformer::EDeformerType::eSkin)
				{
					pSkin = (FbxSkin*)pFbxMesh->GetDeformer(d);
					break;
				}
			}

			//init entry
			//_existingMeshMap[pFbxMesh];
			MeshInternalData* pMeshData = &_existingMeshMap[pFbxMesh].VertexData;
			std::vector<std::pair<uint32_t, uint32_t>>& polyIndexDataLookup = _existingMeshMap[pFbxMesh].PolyIndexDataLookup;
			_existingMeshMap[pFbxMesh].Skin = pSkin;

			//Mat4 meshWorldMtx;
			//Mat4 meshNormalMtx;

			//if (pSkin)
			//{
			//	int nPoses = _fbxData->_scene->GetPoseCount();
			//	for (int i = 0; i < nPoses; i++)
			//	{
			//		FbxPose* pPose = _fbxData->_scene->GetPose(i);
			//		bool bBindPose = pPose->IsBindPose();
			//		if ( bBindPose|| nPoses == 1)
			//		{
			//			for (int j = 0; j < pPose->GetCount(); j++)
			//			{
			//				if (pPose->GetNode(j) == pNode)
			//				{
			//					FbxMatrix mtx = pPose->GetMatrix(j);
			//					meshWorldMtx = /*stack.worldMtx * */ToImporterMtx(mtx);

			//					Vec3 translation;
			//					translation.x = meshWorldMtx[3][0];
			//					translation.y = meshWorldMtx[3][1];
			//					translation.z = meshWorldMtx[3][2];

			//					//meshWorldMtx = meshWorldMtx.GetInverse().GetTranspose();
			//					//meshWorldMtx.SetTranslation(translation.x, translation.y, translation.z);

			//					meshNormalMtx = meshWorldMtx.GetInverse().GetTranspose();
			//					break;
			//				}
			//			}
			//		}
			//	}
			//}
			//else
			//{
			//	meshWorldMtx = stack.worldMtx;
			//	meshNormalMtx = stack.normalMtx;
			//}

			pMeshData->_positions.resize(pFbxMesh->GetControlPointsCount());
			for (int i = 0; i < pFbxMesh->GetControlPointsCount(); i++)
			{
				FbxVector4 cp = pFbxMesh->GetControlPointAt(i);
				Vec3 pos = Vec3((float)cp[0], (float)cp[1], (float)cp[2]);
				pMeshData->_positions[i].Set(pos.x, pos.y, pos.z);
			}

			FbxLayerElementNormal* pNormElem = pFbxMesh->GetElementNormalCount() ? pFbxMesh->GetElementNormal(0) : 0;
			if (pNormElem)
			{
				pMeshData->_normals.resize(pNormElem->GetDirectArray().GetCount());
				for (int i = 0; i < pNormElem->GetDirectArray().GetCount(); i++)
				{
					FbxVector4 fbxNormal = pNormElem->GetDirectArray().GetAt(i);
					Vec3 normal = Vec3((float)fbxNormal[0], (float)fbxNormal[1], (float)fbxNormal[2]);
					normal.Norm();
					pMeshData->_normals[i].Set(normal.x, normal.y, normal.z);
				}
			}


			uint32_t nUVElements = pFbxMesh->GetElementUVCount();
			FbxLayerElementUV* pUVElem = nUVElements ? pFbxMesh->GetElementUV(0) : 0;
			if (pUVElem)
			{
				pMeshData->_texCoords.resize(pUVElem->GetDirectArray().GetCount());
				for (int i = 0; i < pUVElem->GetDirectArray().GetCount(); i++)
				{
					FbxVector2 uv = pUVElem->GetDirectArray().GetAt(i);
					pMeshData->_texCoords[i].Set((float)uv[0], 1.0f - (float)uv[1]);
				}
			}

			if (pSkin)
			{
				pMeshData->_vertexBones.resize(pMeshData->_positions.size());
				memset(pMeshData->_vertexBones.data(), 0, sizeof(VertexBoneInfo) * pMeshData->_positions.size());
				int nClusters = pSkin->GetClusterCount();
				for (int i = 0; i < nClusters; i++)
				{
					FbxCluster* pCluster = pSkin->GetCluster(i);
					if (pCluster->GetLink())
					{
						uint32_t boneNumber = UINT32_MAX;
						for (uint32_t b = 0; b < _vertexBoneTable.size() && boneNumber == UINT32_MAX; b++)
						{
							if (_vertexBoneTable[b] == pCluster->GetLink())
								boneNumber = b;
						}

						int nIndices = pCluster->GetControlPointIndicesCount();
						for (int j = 0; j < nIndices; j++)
						{
							int vtx = pCluster->GetControlPointIndices()[j];
							float weight = (float)pCluster->GetControlPointWeights()[j];

							bool bFoundSlot = false;

							float minWeight = FLT_MAX;
							uint32_t minIndex = (uint32_t)-1;

							for (uint32_t b = 0; b < VertexBoneInfo::MAX_VERTEX_BONES && !bFoundSlot; b++)
							{
								if (pMeshData->_vertexBones[vtx].weights[b] == 0.0f)
								{
									pMeshData->_vertexBones[vtx].bones[b] = (float)boneNumber;
									pMeshData->_vertexBones[vtx].weights[b] = weight;
									bFoundSlot = true;
								}
								else
								{
									if (pMeshData->_vertexBones[vtx].weights[b] < minWeight)
									{
										minWeight = pMeshData->_vertexBones[vtx].weights[b];
										minIndex = b;
									}
								}
							}

							if (!bFoundSlot)
							{
								if (weight > minWeight)
								{
									pMeshData->_vertexBones[vtx].bones[minIndex] = (float)boneNumber;
									pMeshData->_vertexBones[vtx].weights[minIndex] = weight;
								}
							}
						}

						for (uint32_t j = 0; j < pMeshData->_vertexBones.size(); j++)
						{
							Vec2 sortedBones[VertexBoneInfo::MAX_VERTEX_BONES] = {};
							for (uint32_t b = 0; b < VertexBoneInfo::MAX_VERTEX_BONES; b++)
							{
								sortedBones[b].x = pMeshData->_vertexBones[j].bones[b];
								sortedBones[b].y = pMeshData->_vertexBones[j].weights[b];
							}

							qsort(sortedBones, VertexBoneInfo::MAX_VERTEX_BONES, sizeof(Vec2),
								[](const void* lhs, const void* rhs) -> int {
								Vec2* pLeft = (Vec2*)lhs;
								Vec2* pRight = (Vec2*)rhs;

								if (pLeft->y < pRight->y)
									return 1;
								else
									return -1;
							}
							);

							for (uint32_t b = 0; b < VertexBoneInfo::MAX_VERTEX_BONES; b++)
							{
								pMeshData->_vertexBones[j].bones[b] = sortedBones[b].x;
								pMeshData->_vertexBones[j].weights[b] = sortedBones[b].y;
							}
						}
					}
				}

				for (uint32_t i = 0; i < pMeshData->_vertexBones.size(); i++)
				{
					float sum = 0.0f;
					for (uint32_t j = 0; j < VertexBoneInfo::MAX_VERTEX_BONES; j++)
					{
						sum += pMeshData->_vertexBones[i].weights[j];
					}

					if (sum > 0.0f)
					{
						for (uint32_t j = 0; j < VertexBoneInfo::MAX_VERTEX_BONES; j++)
						{
							pMeshData->_vertexBones[i].weights[j] /= sum;
						}
					}
				}
			}

			int numPolygons = pFbxMesh->GetPolygonCount();
			int polyVertIndex = 0;
			int numNegTexCoord = 0;
			polyIndexDataLookup.resize(numPolygons);
			for (int p = 0; p < numPolygons; p++)
			{
				int nSize = pFbxMesh->GetPolygonSize(p);
				static PolygonVertex localVerts[128];

				if (nSize <= 4)
				{
					for (int i = 0; i < nSize; i++)
					{
						int index = pFbxMesh->GetPolygonVertex(p, i);

						PolygonVertex vert;
						vert.positionIndex = index;

						if (pNormElem)
						{
							int nNorms = pNormElem->GetDirectArray().GetCount();
							(void)nNorms;

							int normIndex = -1;
							switch (pNormElem->GetMappingMode())
							{
							case FbxLayerElement::EMappingMode::eByControlPoint:
								normIndex = index;
								break;
							case FbxLayerElement::EMappingMode::eByPolygon:
								normIndex = p;
								break;
							case FbxLayerElement::EMappingMode::eByPolygonVertex:
								normIndex = polyVertIndex;
								break;
							default:
								printf("Unsupported Normal mapping mode: %d\n", pNormElem->GetMappingMode());
								break;
							}

							switch (pNormElem->GetReferenceMode())
							{
							case FbxLayerElement::EReferenceMode::eDirect:
								vert.normalIndex = normIndex;
								break;
							case FbxLayerElement::EReferenceMode::eIndexToDirect:
							case FbxLayerElement::EReferenceMode::eIndex:
								vert.normalIndex = pNormElem->GetIndexArray().GetAt(normIndex);
								break;
							default:
								break;
							}
						}

						if (pFbxMesh->GetElementUVCount())
						{
							int uvIndex = -1;
							switch (pUVElem->GetMappingMode())
							{
							case FbxLayerElement::EMappingMode::eByControlPoint:
								uvIndex = index;
								break;
							case FbxLayerElement::EMappingMode::eByPolygonVertex:
								uvIndex = polyVertIndex;
								break;
							default:
								printf("Unsupported UV mapping mode: %d\n", pUVElem->GetMappingMode());
								break;
							}

							switch (pUVElem->GetReferenceMode())
							{
							case FbxLayerElement::EReferenceMode::eDirect:
								vert.texCoordIndex = uvIndex;
								break;
							case FbxLayerElement::EReferenceMode::eIndexToDirect:
							case FbxLayerElement::EReferenceMode::eIndex:
								vert.texCoordIndex = pUVElem->GetIndexArray().GetAt(uvIndex);
								break;
							default:
								break;
							}

						}

						if (vert.texCoordIndex == -1)
						{
							numNegTexCoord++;
						}

						localVerts[i] = vert;
						polyVertIndex++;
					}

					uint32_t polyStart = pMeshData->_polyVerts.size();

					if (nSize == 3)
					{
						pMeshData->_polyVerts.push_back(localVerts[0]);
						pMeshData->_polyVerts.push_back(localVerts[1]);
						pMeshData->_polyVerts.push_back(localVerts[2]);
						polyIndexDataLookup[p] = { polyStart,  3 };
					}
					else if (nSize == 4)
					{
						pMeshData->_polyVerts.push_back(localVerts[0]);
						pMeshData->_polyVerts.push_back(localVerts[1]);
						pMeshData->_polyVerts.push_back(localVerts[2]);

						pMeshData->_polyVerts.push_back(localVerts[0]);
						pMeshData->_polyVerts.push_back(localVerts[2]);
						pMeshData->_polyVerts.push_back(localVerts[3]);
						polyIndexDataLookup[p] = { polyStart, 6 };
					}
					else
					{
						assert(false);
					}
				}
			}
		}
	}

	void FBXImporter::CollectMaterials()
	{
		int numMaterials = _fbxData->_scene->GetMaterialCount();
		for (int i = 0; i < numMaterials; i++)
		{
			FbxSurfaceMaterial* pFbxMtl = _fbxData->_scene->GetMaterial(i);

			Material* pMat = AddMaterial();
			pMat->Name = pFbxMtl->GetName();
			pMat->pUserData = pFbxMtl;

			if (pFbxMtl->Is<FbxSurfacePhong>())
			{
				FbxSurfacePhong* pPhong = (FbxSurfacePhong*)pFbxMtl;

				FbxDouble3 diffuseColor = pPhong->Diffuse;
				pMat->DiffuseColor.Set((float)diffuseColor[0], (float)diffuseColor[1], (float)diffuseColor[2]);

				FbxDouble3 ambientColor = pPhong->Ambient;
				FbxDouble3 emissiveColor = pPhong->Emissive;

				FbxDouble3 transparentColor = pPhong->TransparentColor;
				FbxDouble transparency = (transparentColor[0] + transparentColor[1] + transparentColor[2]) / 3.0f;
				pMat->Alpha = (float)(1.0 - transparency);

				FbxDouble3 specularColor = pPhong->Specular;
				pMat->SpecularColor.Set((float)specularColor[0], (float)specularColor[1], (float)specularColor[2]);
				pMat->SpecularColor *= (float)pPhong->SpecularFactor;
				pMat->SpecularExponent = (float)pPhong->Shininess * 4.0f; //safe to hardcode here? using blinn-phong in shaders...

	
			}
			else if (pFbxMtl->Is<FbxSurfaceLambert>())
			{
				FbxSurfaceLambert* pLambert = (FbxSurfaceLambert*)pFbxMtl;

				FbxDouble3 diffuseColor = pLambert->Diffuse;
				FbxDouble3 ambientColor = pLambert->Ambient;
				FbxDouble3 emissiveColor = pLambert->Emissive;

				FbxDouble3 transparentColor = pLambert->TransparentColor;
				FbxDouble transparency = (transparentColor[0] + transparentColor[1] + transparentColor[2]) / 3.0f;
				pMat->Alpha = 1.0f - transparency;

				pMat->DiffuseColor.Set((float)diffuseColor[0], (float)diffuseColor[1], (float)diffuseColor[2]);
			}
			else
			{

			}

			int layerIndex = 0;
			FBXSDK_FOR_EACH_TEXTURE(layerIndex)
			{
				std::string texType = FbxLayerElement::sTextureChannelNames[layerIndex];
				FbxProperty prop = pFbxMtl->FindProperty(texType.data());
				FbxTexture* pTexture = prop.GetSrcObject<FbxTexture>();

				if (
					texType == FBX_DIFFUSE_MAP || 
					texType == FBX_NORMAL_MAP || 
					texType == FBX_SPECULAR_MAP ||
					texType == FBX_TRANSPARENT_MAP ||
					texType == FBX_AMBIENT_MAP ||
					texType == FBX_SPECUALR_FACTOR_MAP ||
					texType == FBX_SHININESS_EXPONENT_MAP ||
					texType == FBX_BUMP_MAP
					
					)

				{
					if (pTexture)
					{
						if (pTexture->Is<FbxFileTexture>())
						{
							FbxFileTexture* pFileTex = (FbxFileTexture*)pTexture;
							std::string path = pFileTex->GetRelativeFileName();
							std::string full = pFileTex->GetFileName();

							std::string fullPath;
							if (this->GetFullFilePath(path, fullPath))
							{
								if(texType == FBX_DIFFUSE_MAP)
									pMat->DiffuseMap = GetTexture(fullPath);
								else if (texType == FBX_NORMAL_MAP)
									pMat->NormalMap = GetTexture(fullPath);
								else if (texType == FBX_SPECULAR_MAP)
									pMat->SpecularMap = GetTexture(fullPath);
								else if (texType == FBX_TRANSPARENT_MAP)
									pMat->TransparentMap = GetTexture(fullPath);
								if (texType == FBX_AMBIENT_MAP)
									pMat->AmbientMap = GetTexture(fullPath);
								else if (texType == FBX_SPECUALR_FACTOR_MAP)
									pMat->SpecularFactorMap = GetTexture(fullPath);
								else if (texType == FBX_SHININESS_EXPONENT_MAP)
									pMat->ShininessExponentMap = GetTexture(fullPath);
								else if (texType == FBX_BUMP_MAP)
									pMat->BumpMap = GetTexture(fullPath);
							}							
						}

					}

				}
				else
				{
					if (pTexture)
					{
						int ww = 5;
						ww++;
					}
				}
			}
		}
	}

	void ModelImporter::FBXImporter::UpdateAnimations()
	{
		for (uint32_t i = 0; i < GetAnimationCount(); i++)
		{
			FbxAnimStack* pAnimStack = (FbxAnimStack*)GetAnimation(i)->pUserData;
			_fbxData->_scene->SetCurrentAnimationStack(pAnimStack);
			FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>();
			
			std::vector<FbxAnimCurve*> curves;

			for (uint32_t j = 0; j < GetBoneCount(); j++)
			{
				Bone* pBone = GetBone(j);
				FbxNode* pBoneNode = (FbxNode*)pBone->pUserData;

				FbxAnimCurve* pTransCurve = pBoneNode->LclTranslation.GetCurve(pAnimLayer);
				FbxAnimCurve* pRotCurve = pBoneNode->LclRotation.GetCurve(pAnimLayer);
				FbxAnimCurve* pScaleCurve = pBoneNode->LclScaling.GetCurve(pAnimLayer);

				if(pTransCurve) curves.push_back(pTransCurve);
				if(pRotCurve) curves.push_back(pRotCurve);
				if(pScaleCurve) curves.push_back(pScaleCurve);
			}

			Animation* pAnim = GetAnimation(i);
			if (curves.size())
			{
				std::qsort(curves.data(), curves.size(), sizeof(FbxAnimCurve*), 
					[] (const void* pLeft, const void* pRight) -> int {
					FbxAnimCurve** ppLeft = (FbxAnimCurve**)pLeft;
					FbxAnimCurve** ppRight = (FbxAnimCurve**)pRight;

					if ((*ppLeft)->KeyGetCount() > (*ppRight)->KeyGetCount())
						return -1;
					else
						return 1;
				});

				FbxAnimCurve* pBestCurve = curves[0];
				
				for (int j = 0; j < pBestCurve->KeyGetCount(); j++)
				{
					FbxTime keyTime = pBestCurve->KeyGetTime(j);

					//NOTE: we are not using negative keys based on one model(MilitaryMechanic.fbx) 
					//having issues with its first keyframe being negative time
					if (keyTime.GetSecondDouble() >= 0.0)
					{
						pAnim->KeyFrames.push_back(KeyFrame());
						KeyFrame& kf = pAnim->KeyFrames[pAnim->KeyFrames.size() - 1];

						kf.Time = keyTime.GetSecondDouble();
						kf.Transforms.resize(GetBoneCount());

						for (uint32_t b = 0; b < GetBoneCount(); b++)
						{
							Bone* pBone = GetBone(b);
							FbxNode* pBoneNode = (FbxNode*)pBone->pUserData;

							FbxAMatrix boneMtx = pBoneNode->EvaluateLocalTransform(keyTime);
							FbxVector4 fbxTranslation = boneMtx.GetT();
							FbxVector4 fbxRot = boneMtx.GetR();
							FbxVector4 fbxScale = boneMtx.GetS();

							BoneTransform& bt = kf.Transforms[b];
							bt.Translation.Set((float)fbxTranslation[0], (float)fbxTranslation[1], (float)fbxTranslation[2]);
							bt.Rotation.Set((float)fbxRot[0], (float)fbxRot[1], (float)fbxRot[2]);
							bt.Rotation *= (PI / 180.0f);
							bt.Scale.Set((float)fbxScale[0], (float)fbxScale[1], (float)fbxScale[2]);
#if 0

							Mat4 mtxTranslation, mtxRotation, mtxScale;
							mtxTranslation.SetTranslation(bt.Translation.x, bt.Translation.y, bt.Translation.z);
							mtxRotation.RotateXYZ(bt.Rotation.x, bt.Rotation.y, bt.Rotation.z);
							mtxScale.SetScale(bt.Scale.x, bt.Scale.y, bt.Scale.z);

							Mat4 myMtx = mtxScale * mtxRotation * mtxTranslation;

							Mat4 parentMtx;
							Bone* pParent = pBone->Parent;
							while (pParent)
							{
								BoneTransform& pt = kf.Transforms[pParent->Index];
								mtxTranslation.SetTranslation(pt.Translation.x, pt.Translation.y, pt.Translation.z);
								mtxRotation.RotateXYZ(pt.Rotation.x, pt.Rotation.y, pt.Rotation.z);
								mtxScale.SetScale(pt.Scale.x, pt.Scale.y, pt.Scale.z);

								Mat4 currParentMtx = mtxScale * mtxRotation * mtxTranslation;
								parentMtx = parentMtx * currParentMtx;
								pParent = pParent->Parent;
							}

							Mat4 globalMtx = myMtx * parentMtx;
							Mat4 omg = ToImporterMtx(pBoneNode->EvaluateGlobalTransform(keyTime));

							if (globalMtx != omg)
							{
								int ww = 5;
								ww++;
							}
#endif
						}
					}
				}
				pAnim->Length = pAnim->KeyFrames[pAnim->KeyFrames.size() - 1].Time - pAnim->KeyFrames[0].Time;
			}
		}

		//Add animation clearup in base class
	}

	void ModelImporter::FBXImporter::InitAnimationData()
	{
		int nAnimations = _fbxData->_scene->GetSrcObjectCount<FbxAnimStack>();
		for (int i = 0; i < nAnimations; i++)
		{
			FbxAnimStack* pAnimStack = _fbxData->_scene->GetSrcObject<FbxAnimStack>(i);
			int nLayers = pAnimStack->GetMemberCount<FbxAnimLayer>();
			(void)nLayers;
			_fbxData->_scene->SetCurrentAnimationStack(pAnimStack);
			FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>();

			Animation* pAnim = AddAnimation();
			pAnim->Name = pAnimStack->GetName();
			pAnim->pUserData = pAnimStack;
		}
	}

	void ModelImporter::FBXImporter::InitNodes(FbxNodeHandle* pHandle, TraverseStack& data)
	{
		FbxNode* pFbxNode = pHandle->_node;
		FbxMesh* pFbxMesh = pFbxNode->GetMesh();
		FbxSkin* pFbxSkin = 0;
		if (pFbxMesh)
		{
			for (int d = 0; d < pFbxMesh->GetDeformerCount(); d++)
			{
				if (pFbxMesh->GetDeformer(d)->GetDeformerType() == FbxDeformer::EDeformerType::eSkin)
				{
					pFbxSkin = (FbxSkin*)pFbxMesh->GetDeformer(d);
					if (_skinIndexMap.find(pFbxSkin) == _skinIndexMap.end())
					{
						uint32_t skinIndex = _skinIndexMap.size();
						_skinIndexMap[pFbxSkin] = skinIndex;

						int nClusters = pFbxSkin->GetClusterCount();
						for (int c = 0; c < nClusters; c++)
						{
							FbxCluster* pCluster = pFbxSkin->GetCluster(c);
							if (pCluster->GetLink())
							{
								_typeMap[pCluster->GetLink()] = BONE;
								if(std::find(_vertexBoneTable.begin(), _vertexBoneTable.end(), pCluster->GetLink()) == _vertexBoneTable.end())
								if (pCluster->GetUserDataPtr() == 0)
								{
									_vertexBoneTable.push_back(pCluster->GetLink());
								}
							}
						}
					}
					break;
				}
			}
		}

		if (pFbxSkin == 0)
		{
			for (uint32_t i = 0; i < GetAnimationCount(); i++)
			{
				FbxAnimStack* pAnimStack = (FbxAnimStack*)GetAnimation(i)->pUserData;
				_fbxData->_scene->SetCurrentAnimationStack(pAnimStack);
				FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>();

				FbxAnimCurve* pCurveR = pFbxNode->LclRotation.GetCurve(pAnimLayer);
				FbxAnimCurve* pCurveT = pFbxNode->LclTranslation.GetCurve(pAnimLayer);
				FbxAnimCurve* pCurveS = pFbxNode->LclScaling.GetCurve(pAnimLayer);

				int nKeysT = pCurveT ? pCurveT->KeyGetCount() : 0;
				int nKeysS = pCurveS ? pCurveS->KeyGetCount() : 0;
				int nKeysR = pCurveR ? pCurveR->KeyGetCount() : 0;

				if ((nKeysT > 1 || nKeysS > 1 || nKeysR > 1))
				{
					if (_typeMap.find(pFbxNode) == _typeMap.end())
					{
						_typeMap[pFbxNode] = BONE;
					}
					break;
				}
			}
		}

		if (_typeMap.find(pFbxNode) == _typeMap.end())
		{
			if (pFbxMesh)
			{
				_typeMap[pFbxNode] = MESH;
			}
			else
			{
				_typeMap[pFbxNode] = TRANSFORM;
			}
		}
	}

	void ModelImporter::FBXImporter::UpdateSkinningData()
	{	
		for (uint32_t i = 0; i < GetBoneCount(); i++)
		{
			GetBone(i)->SkinMatrices.resize(_skinIndexMap.size());
		}

		for(auto iter = _skinIndexMap.begin(); iter != _skinIndexMap.end(); ++iter)
		{
			uint32_t skinIndex = (*iter).second;
			FbxSkin* pSkin = (FbxSkin*)(*iter).first;

			int nClusters = pSkin->GetClusterCount();
			for (int i = 0; i < nClusters; i++)
			{
				FbxCluster* pCluster = pSkin->GetCluster(i);
				Bone* pBone = FindBone(pCluster->GetLink());
				if (pBone)
				{
					FbxAMatrix clusterGlobalInitPosition, referenceGlobalInitPosition;
					pCluster->GetTransformMatrix(referenceGlobalInitPosition);
					pCluster->GetTransformLinkMatrix(clusterGlobalInitPosition);

					Mat4 cluster = ToImporterMtx(clusterGlobalInitPosition);
					Mat4 reference = ToImporterMtx(referenceGlobalInitPosition);
					Mat4 test = cluster.GetInverse() * reference;
					(void)test;

					FbxAMatrix clusterRelativeInitPosition = clusterGlobalInitPosition.Inverse() * referenceGlobalInitPosition;
					pBone->SkinMatrices[skinIndex] = ToImporterMtx(clusterRelativeInitPosition);
				}
			}
		}

		std::unordered_map<void*, FbxMaterialMeshData>::iterator meshIter = _existingMeshMap.begin();
		while (meshIter != _existingMeshMap.end())
		{
			uint32_t skinIndex = (*meshIter).second.Skin ? _skinIndexMap[(*meshIter).second.Skin] : 0;

			for (uint32_t i = 0; i < (*meshIter).second.Sets.size(); i++)
			{
				MeshInternalData* pData = (*meshIter).second.Sets[i].MeshData;
				pData->_skinIndex = skinIndex;

				if (pData->_vertexBones.size())
				{
					for (uint32_t j = 0; j < pData->_vertexBones.size(); j++)
					{
						VertexBoneInfo& info = pData->_vertexBones[j];
						uint32_t b = 0;
						while (b < VertexBoneInfo::MAX_VERTEX_BONES && info.weights[b] > 0.0f)
						{
							uint32_t id = (uint32_t)info.bones[b];
							void* pNodePtr = _vertexBoneTable[id];
							Bone* pBone = FindBone(pNodePtr);
							info.bones[b] = (float)pBone->GetID();
							b++;
						}
					}
				}
			}
			++meshIter;
		}
	}

	void ModelImporter::FBXImporter::CollectNodes(FbxNodeHandle* pHandle, TraverseStack& data)
	{
		FbxNode* pFbxNode = pHandle->_node;
		FbxMesh* pFbxMesh = pFbxNode->GetMesh();

		std::vector<Node*> nodeList;

		//bool exists = _typeMap.find(pFbxNode) != _typeMap.end();
		
		NodeType type = _typeMap.at(pFbxNode);

		if (type == MESH)
		{
			CreateMesh(pHandle, nodeList);
		}
		else if(type == BONE)
		{
			Bone* pBone = AddBone();
			pBone->Name = GetUniqueNodeName(pFbxNode->GetName());
			pBone->Color = Vec3((float)rand(), (float)rand(), (float)rand()) * (1.0f / (float)RAND_MAX);
			pBone->pUserData = pHandle->_node;
			
			if (pFbxMesh)
			{
				CreateMesh(pHandle, nodeList);
				for (uint32_t i = 0; i < nodeList.size(); i++)
				{
					nodeList[i]->GeometryOffset = data.geomOffset;
					nodeList[i]->LocalMatrix = Mat4();
					nodeList[i]->WorldMatrix = data.worldMtx;
					pBone->Attach(nodeList[i]);
				}
				data.geomOffset = Mat4();
				nodeList.clear();
			}

			nodeList.push_back(pBone);
		}

		if(nodeList.size() == 0)
		{
			Node* pNode = AddNode();
			pNode->Name = GetUniqueNodeName(pFbxNode->GetName());
			nodeList.push_back(pNode);
		}

		Node* pParentNode = (Node*)data.pUserData;
		for (uint32_t i = 0; i < nodeList.size(); i++)
		{
			Node* pNode = nodeList[i];
			pNode->LocalMatrix = data.localMtx;
			pNode->WorldMatrix = data.worldMtx;
			pNode->GeometryOffset = data.geomOffset;

			if(pParentNode)
				pParentNode->Attach(pNode);
		}

		data.pUserData = nodeList[0];
	}

	void ModelImporter::FBXImporter::CreateMesh(FbxNodeHandle* pHandle, std::vector<Node*>& output)
	{
		FbxNode* pNode = pHandle->_node;
		FbxMesh* pFbxMesh = pHandle->_node->GetMesh();

		FbxLayerElementMaterial* pGeomMtl = pFbxMesh->GetElementMaterial(0);

		pGeomMtl = pFbxMesh->GetElementMaterial(0);
		std::unordered_map<FbxSurfaceMaterial*, std::vector<std::pair<void*, uint32_t>>> meshMtlMap;
		bool bOneMaterial = pGeomMtl ? pGeomMtl->GetMappingMode() != FbxGeometryElement::eByPolygon : true;

		int numPolygons = pFbxMesh->GetPolygonCount();
		for (int p = 0; p < numPolygons; p++)
		{
			if (pFbxMesh->GetPolygonSize(p) <= 4)
			{
				if (bOneMaterial)
				{
					FbxSurfaceMaterial* pMtl = pNode->GetMaterial(0);
					meshMtlMap[pMtl].push_back({ pMtl, p });
				}
				else
				{
					auto& arr = pGeomMtl->GetIndexArray();
					FbxSurfaceMaterial* pMtl = pNode->GetMaterial(arr.GetAt(p));
					meshMtlMap[pMtl].push_back({ pMtl, p });
				}
			}
		}

		FbxMaterialMeshData* data = &_existingMeshMap.at(pFbxMesh);

		std::vector<FbxSurfaceMaterial*> removeList;

		std::unordered_map<FbxSurfaceMaterial*, std::vector<std::pair<void*, uint32_t>>>::iterator meshIter = meshMtlMap.begin();
		while (meshIter != meshMtlMap.end())
		{
			std::vector<std::pair<void*, uint32_t>>& mtlKeys = (*meshIter).second;
			for (uint32_t i = 0; i < data->Sets.size(); i++)
			{
				if (mtlKeys == data->Sets[i].MaterialPolyIndices)
				{
					Mesh* pMesh = AddMesh();
					pMesh->Material = data->Sets[i].Material;
					pMesh->Name = GetUniqueNodeName(pNode->GetName());
					pMesh->pUserData = data->Sets[i].MeshData;
					output.push_back(pMesh);

					removeList.push_back((*meshIter).first);
					break;
				}
			}
			++meshIter;
		}

		for (uint32_t i = 0; i < removeList.size(); i++)
		{
			meshMtlMap.erase(removeList[i]);
		}

		MeshInternalData* pMeshData = &_existingMeshMap.at(pFbxMesh).VertexData;
		std::vector<std::pair<uint32_t, uint32_t>>& polyIndexDataLookup =_existingMeshMap.at(pFbxMesh).PolyIndexDataLookup;

		meshIter = meshMtlMap.begin();
		while (meshIter != meshMtlMap.end())
		{
			MeshInternalData* pNewData = new MeshInternalData();

			FbxSurfaceMaterial* pMtl = (*meshIter).first;
			std::vector<PolygonVertex> verts;

			for (uint32_t i = 0; i < (*meshIter).second.size(); i++)
			{
				std::pair<uint32_t, uint32_t> polyData = polyIndexDataLookup[(*meshIter).second[i].second];

				for (uint32_t j = 0; j < polyData.second; j++)
				{
					verts.push_back(pMeshData->_polyVerts[polyData.first + j]);
				}
			}

			//assert(memcmp(verts.data(), pMeshData->_polyVerts.data(), sizeof(PolygonVertex) * verts.size()) == 0);

			std::vector<uint32_t> usedPositions;
			usedPositions.resize(pMeshData->_positions.size());
			memset(usedPositions.data(), 0, sizeof(uint32_t) * usedPositions.size());

			std::vector<uint32_t> usedNormals;
			usedNormals.resize(pMeshData->_normals.size());
			memset(usedNormals.data(), 0, sizeof(uint32_t) * usedNormals.size());

			std::vector<uint32_t> usedTexCoords;
			usedTexCoords.resize(pMeshData->_texCoords.size());
			memset(usedTexCoords.data(), 0, sizeof(uint32_t) * usedTexCoords.size());

			std::vector<uint32_t> remappedPositions;
			remappedPositions.resize(usedPositions.size());

			std::vector<uint32_t> remappedNormals;
			remappedNormals.resize(usedNormals.size());

			std::vector<uint32_t> remappedTexCoords;
			remappedTexCoords.resize(usedTexCoords.size());

			for (uint32_t v = 0; v < verts.size(); v++)
			{
				PolygonVertex& vtx = verts[v];
				if (vtx.normalIndex != -1)
					usedNormals[vtx.normalIndex]++;
				if (vtx.texCoordIndex != -1)
					usedTexCoords[vtx.texCoordIndex]++;

				usedPositions[vtx.positionIndex]++;
			}

			uint32_t nUsedPositions = 0;
			for (uint32_t i = 0; i < usedPositions.size(); i++)
			{
				if (usedPositions[i] > 0)
				{
					remappedPositions[i] = nUsedPositions++;
					pNewData->_positions.push_back(pMeshData->_positions[i]);

					if (pMeshData->_vertexBones.size())
					{
						pNewData->_vertexBones.push_back(pMeshData->_vertexBones[i]);
					}
				}
			}

			uint32_t nUsedNormals = 0;
			for (uint32_t i = 0; i < usedNormals.size(); i++)
			{
				if (usedNormals[i] > 0)
				{
					remappedNormals[i] = nUsedNormals++;
					pNewData->_normals.push_back(pMeshData->_normals[i]);
				}
			}

			uint32_t nUsedTexCoords = 0;
			for (uint32_t i = 0; i < usedTexCoords.size(); i++)
			{
				if (usedTexCoords[i] > 0)
				{
					remappedTexCoords[i] = nUsedTexCoords++;
					pNewData->_texCoords.push_back(pMeshData->_texCoords[i]);
				}
			}

			pNewData->_polyVerts.resize(verts.size());
			for (uint32_t i = 0; i < verts.size(); i++)
			{
				PolygonVertex& vtxOld = verts[i];
				PolygonVertex& vtxNew = pNewData->_polyVerts[i];

				vtxNew.positionIndex = remappedPositions[vtxOld.positionIndex];
				if (vtxOld.normalIndex != -1)
					vtxNew.normalIndex = remappedNormals[vtxOld.normalIndex];
				else
					vtxNew.normalIndex = -1;

				if (vtxOld.texCoordIndex != -1)
					vtxNew.texCoordIndex = remappedTexCoords[vtxOld.texCoordIndex];
				else
					vtxNew.texCoordIndex = -1;
			}

			assert(pNewData->_positions.size());

			Mesh* pNewMesh = AddMesh();
			pNewMesh->Material = GetMaterial(pMtl);
			pNewMesh->Name = GetUniqueNodeName(pNode->GetName());
			pNewData->_skinIndex = pMeshData->_skinIndex;
			pNewMesh->pUserData = pNewData;
			output.push_back(pNewMesh);

			FbxMaterialMeshData::UniqueSet set;
			set.Material = pNewMesh->Material;
			set.MeshData = pNewData;
			set.MaterialPolyIndices = (*meshIter).second;
			_existingMeshMap[pFbxMesh].Sets.push_back(set);
			++meshIter;
		}
	}
}