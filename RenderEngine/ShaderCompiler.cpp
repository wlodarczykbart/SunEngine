#include <d3dcompiler.h>
#include <direct.h>
#include <mutex>
#include <assert.h>
#include "GraphicsAPIDef.h"
#include "FileBase.h"
#include "StringUtil.h"
#include "BaseShader.h"

#include "MemBuffer.h"
#include "BufferBase.h"
#include "ShaderCompiler.h"


namespace SunEngine
{
	//Make sure this is updated when something related to the compiler changes
	const uint SHADER_COMPILER_VERSION = 3;

	const uint HLSL_MASK_XYZW = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z | D3D_COMPONENT_MASK_W);
	const uint HLSL_MASK_XYZ = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z);
	const uint HLSL_MASK_XY = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y);

	const uint ENGINE_RESOURCE_COUNT = 3;

	static const Vector<uint> SHADER_STAGE_ARRAY
	{
		SS_VERTEX,
		SS_PIXEL,
		SS_GEOMETRY,
		SS_COMPUTE
	};

	void ShaderCompiler::InitShaderBindingNames(ShaderBindingType type)
	{
		Vector<String> names;
		switch (type)
		{
		case SunEngine::SBT_CAMERA:
			names.push_back(ShaderStrings::CameraBufferName);
			break;
		case SunEngine::SBT_OBJECT:
			names.push_back(ShaderStrings::ObjectBufferName);
			break;
		case SunEngine::SBT_LIGHT:
			names.push_back(ShaderStrings::PointlightBufferName);
			names.push_back(ShaderStrings::SpotlightBufferName);
			break;
		case SunEngine::SBT_ENVIRONMENT:
			names.push_back(ShaderStrings::EnvBufferName);
			names.push_back(ShaderStrings::EnvTextureName);
			names.push_back(ShaderStrings::EnvProbesTextureName);
			names.push_back(ShaderStrings::EnvSamplerName);
			break;
		case SunEngine::SBT_SHADOW:
			names.push_back(ShaderStrings::ShadowBufferName);
			names.push_back(ShaderStrings::ShadowTextureName);
			names.push_back(ShaderStrings::ShadowSamplerName);
			break;
		case SunEngine::SBT_SCENE:
			names.push_back(ShaderStrings::SceneTextureName);
			names.push_back(ShaderStrings::SceneSamplerName);
			names.push_back(ShaderStrings::DepthTextureName);
			names.push_back(ShaderStrings::DepthSamplerName);
			break;
		case SunEngine::SBT_BONES:
			names.push_back(ShaderStrings::SkinnedBoneBufferName);
			break;
		case SunEngine::SBT_MATERIAL:
			names.push_back(ShaderStrings::MaterialBufferName);
			names.push_back(ShaderStrings::TextureTransformBufferName);
			break;
		default:
			break;
		}

		for (uint i = 0; i < names.size(); i++)
			_namedBindingLookup[names[i]].bindType = (ShaderBindingType)type;
	}
   
	String g_ShaderAuxDir = "";
	String g_ShaderCacheDir = "";
	String g_ShaderVulkanDir = "";

	ShaderCompiler::ShaderCompiler()
	{
		_numUserSamplers = 0;
		_numUserTextures = 0;
	}

	ShaderCompiler::~ShaderCompiler()
	{
	}

	void ShaderCompiler::SetAuxiliaryDir(const String& path)
	{
		g_ShaderAuxDir = path + "/";
		g_ShaderCacheDir = g_ShaderAuxDir + "Cached/";
		g_ShaderVulkanDir = g_ShaderCacheDir + "Vulkan/";
		_mkdir(g_ShaderCacheDir.c_str());
		_mkdir(g_ShaderVulkanDir.c_str());
	}

	void ShaderCompiler::SetDefines(const Vector<String>& defines)
	{
		_defines = defines;
	}

	void ShaderCompiler::SetShaderSource(ShaderStage stage, const String& source)
	{
		if(!source.empty())
			_shaderSource[stage] = source;
	}

	bool ShaderCompiler::Compile(const String& uniqueName)
	{
		_shaderInfo.resources.clear();
		_namedBindingLookup.clear();
		_numUserTextures = 0;
		_numUserSamplers = 0;
		_uniqueName = uniqueName;

		for (uint i = 0; i <= SBT_MATERIAL; i++)
			InitShaderBindingNames((ShaderBindingType)i);

		uint shaderFlags = 0;

		for (auto iter = _shaderSource.begin(); iter != _shaderSource.end(); ++iter)
		{
			ShaderStage stage = (*iter).first;
			PreProcessText((*iter).second, _hlslShaderText[stage], _glslShaderText[stage]);
			shaderFlags |= stage;
		}

		String cachedFile = g_ShaderCacheDir + _uniqueName + ".scached";
		if (_uniqueName.length())
		{
			if (MatchesCachedFile(cachedFile, shaderFlags))
				return true;
		}

		for (auto iter = _shaderSource.begin(); iter != _shaderSource.end(); ++iter)
		{
			if (!CompileShader((*iter).first))
				return false;
		}

		Vector<String> unusedResources;
		for (auto iter = _shaderInfo.resources.begin(); iter != _shaderInfo.resources.end(); ++iter)
		{
			if (strlen((*iter).second.name) == 0)
				unusedResources.push_back((*iter).first);
		}
		for (uint i = 0; i < unusedResources.size(); i++)
		{
			_shaderInfo.resources.erase(unusedResources[i]);
		}

		if (_uniqueName.length())
		{
			if (WriteCachedFile(cachedFile, shaderFlags))
				return true;
		}

		return true;
	}

	void ShaderCompiler::PreProcessText(const String& inText, String& outHLSL, String& outGLSL)
	{
		String parsedText;
		HashSet<String> includedFiles;
		ParseShaderFile(parsedText, inText, includedFiles);

		Vector<String> lines;
		StrSplit(StrRemove(parsedText, '\r'), lines, '\n');

		outGLSL.clear();
		outHLSL.clear();

		//This must be in order of any names that might overlap, ie, Texture2DArray should be before Texture2D because Texture2D is contained in Texture2DArray, likely a better way of doing this...
		Vector<Pair<String, String>> ResourceDefintions = 
		{
			{"b", "cbuffer"},
			{"u", "RWTexture2D<float4>"},
			{"t", "Texture2DArray"},
			{"t", "Texture2DMS<float4>"},
			{"t", "Texture2D"},
			{"t", "TextureCubeArray"},
			{"t", "TextureCube"},
			{"s", "SamplerState"},
		};

		const String STR_REGISTER_PLACEHOLDER = "SHADER_COMPILER_REGISTER=";

		Map<uint, String> registerUpdateLines;

		for (uint i = 0; i < lines.size(); i++)
		{
			const String& line = lines[i];

			for (uint j = 0; j < ResourceDefintions.size(); j++)
			{
				usize resPos = line.find(ResourceDefintions[j].second);
				if (resPos != String::npos)
				{
					bool valid = true;
					for (uint k = i; k < lines.size() && valid; k++)
					{
						//Add other things as needed to invalidate any resources which are not declerations(ie function arguments)
						if (lines[k].find('(') != String::npos || lines[k].find(')') != String::npos || lines[k].find("struct") != String::npos)
							valid = false;

						if (lines[k].find(';') != String::npos)
							break;
					}

					if (valid)
					{
						String resName = line.substr(resPos + ResourceDefintions[j].second.length() + 1);
						for (uint k = 0; k < resName.length(); k++)
						{
							if (isalpha(resName[k]) == 0)
							{
								resName = resName.substr(0, k);
								break;
							}
						}

						String strResType = ResourceDefintions[j].first;
						if (_namedBindingLookup.count(resName) == 0)
							_namedBindingLookup[resName].bindType = SBT_MATERIAL;
						_namedBindingLookup[resName].hlslRegister = strResType;
						registerUpdateLines[i] = resName;
					}
					break;
				}
			}
		}

		Map<ShaderBindingType, uint> vulkanCounters;
		Map<String, uint> d3d11Counters;

		//supported registers
		d3d11Counters["b"] = 0;
		d3d11Counters["t"] = 0;
		d3d11Counters["s"] = 0;
		d3d11Counters["u"] = 0;

		//supported sets
		for (uint i = 0; i <= SBT_MATERIAL; i++)
			vulkanCounters[(ShaderBindingType)i] = 0;

		//First pass we compute the max bindings to start incrementing based on already set bindings
		for (auto iter = _namedBindingLookup.begin(); iter != _namedBindingLookup.end(); ++iter)
		{
			auto& info = (*iter).second;
			if (!info.needsBindingsSet)
			{
				d3d11Counters.at(info.hlslRegister)++; //d3d11 binding are incremented based on resource type based on register char
				vulkanCounters.at(info.bindType)++; //vulkan bindings are incremented based on what set they are in, which is defined by the bind type
			}
		}

		//Second pass we generate correct bindings
		for (auto iter = registerUpdateLines.begin(); iter != registerUpdateLines.end(); ++iter)
		{
			auto& info = _namedBindingLookup[(*iter).second];
			if (info.needsBindingsSet)
			{
			 	uint& d3dBinding = d3d11Counters.at(info.hlslRegister); //d3d11 binding are incremented based on resource type based on register char
				uint& vkBinding = vulkanCounters.at(info.bindType); //vulkan bindings are incremented based on what set they are in, which is defined by the bind type

				info.bindings[SE_GFX_D3D11] = d3dBinding;
				info.bindings[SE_GFX_VULKAN] = vkBinding;

				++d3dBinding;
				++vkBinding;

				info.needsBindingsSet = false;
			}
		}

		for (uint i = 0; i < lines.size(); i++)
		{
			auto found = registerUpdateLines.find(i);
			if (found != registerUpdateLines.end())
			{
				auto& namedBinding = _namedBindingLookup.at((*found).second);
				const String& line = lines[i];

				if(StrContains(line, ";"))
					outHLSL += StrFormat("%s: register(%s%d);\n", StrRemove(line, ';').c_str(), namedBinding.hlslRegister.c_str(), namedBinding.bindings[SE_GFX_D3D11]);
				else
					outHLSL += StrFormat("%s: register(%s%d)\n", line.c_str(), namedBinding.hlslRegister.c_str(), namedBinding.bindings[SE_GFX_D3D11]);
				outGLSL += StrFormat("[[vk::binding(%d, %d)]]\n%s\n", namedBinding.bindings[SE_GFX_VULKAN], (uint)namedBinding.bindType, line.c_str());
			}
			else
			{
				outHLSL += lines[i] + "\n";
				outGLSL += lines[i] + "\n";
			}
		}

		String shaderHeader = "#pragma pack_matrix( row_major )\n";
		for (uint i = 0; i < _defines.size(); i++)
		{
			shaderHeader += "#define " + _defines[i] + "\n";
		}

		outHLSL = shaderHeader + outHLSL;
		outGLSL = shaderHeader + outGLSL;
	}

	bool ShaderCompiler::CompileShader(ShaderStage type)
	{
		String targetHLSL, targetGLSL;
		BaseShader::CreateInfo& shader = _shaderInfo;
		MemBuffer* pBinBuffer = 0;

		if (type == SS_VERTEX)
		{
			targetHLSL = "vs_5_0";
			targetGLSL = "vert";
			pBinBuffer = shader.vertexBinaries;
		}
		else if (type == SS_PIXEL)
		{
			targetHLSL = "ps_5_0";
			targetGLSL = "frag";
			pBinBuffer = shader.pixelBinaries;
		}
		else if (type == SS_COMPUTE)
		{
			targetHLSL = "cs_5_0";
			targetGLSL = "comp";
			pBinBuffer = shader.computeBinaries;
		}
		else
		{
			return false;
		}


		FileStream fr;

		ID3DBlob* pShaderBlod = NULL;
		ID3DBlob* pErrorBlod = NULL;

		String& hlsl = _hlslShaderText.at(type);
		if (D3DCompile(hlsl.c_str(), hlsl.size(), NULL, NULL, NULL, "main", targetHLSL.data(), 0, 0, &pShaderBlod, &pErrorBlod) == S_OK)
		{
			pBinBuffer[SE_GFX_D3D11].SetSize((uint)pShaderBlod->GetBufferSize());
			pBinBuffer[SE_GFX_D3D11].SetData(pShaderBlod->GetBufferPointer(), (uint)pShaderBlod->GetBufferSize());

			String glslInPath = g_ShaderVulkanDir + _uniqueName + "ShaderCompilerTempShader.glsl";
			String glslOutPath = g_ShaderVulkanDir + _uniqueName + "ShaderCompilerTempShader.spv";
			FileStream fw;
			fw.OpenForWrite(glslInPath.data());
			fw.WriteText(_glslShaderText.at(type));
			fw.Close();

			//Don't really want to wrap this in mutex as I feel it should be thread safe, but have gotten file crashes here from the glslangValidator
			//Could look into that more to remove the mutex
			static std::mutex mtx;
			{
				std::lock_guard<std::mutex> lock(mtx);
				//-s = silent console
				//-t = threaded
				//-D = HLSL is input
				//-S = shader stage 
				//-e entry point
				//-o output file
				//V generate spirv binary
				String compileSpvCmd = StrFormat("glslangValidator -t -D -S %s -e main -o %s -V %s", targetGLSL.data(), glslOutPath.data(), glslInPath.data());
				int compiled = system(compileSpvCmd.data());
				if (compiled == 0)
				{
					fr.OpenForRead(glslOutPath.data());
					fr.ReadBuffer(pBinBuffer[SE_GFX_VULKAN]);;
					fr.Close();
				}
				else
				{
					fr.OpenForRead(glslInPath.data());
					String invalidSpvText;
					fr.ReadText(invalidSpvText);
					fr.Close();

					Vector<String> shaderLines;
					StrSplit(invalidSpvText, shaderLines, '\n');
					for (uint l = 0; l < shaderLines.size(); l++)
					{
						_lastErr += StrFormat("%d\t%s\n", l + 1, shaderLines[l].data());
					}
					return false;
				}
			}

			ID3D11ShaderReflection* pReflector = NULL;
			if (D3DReflect(pShaderBlod->GetBufferPointer(), pShaderBlod->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void**)&pReflector) == S_OK)
			{
				D3D11_SHADER_DESC shaderDesc;
				pReflector->GetDesc(&shaderDesc);

				if (type == SS_VERTEX)
				{
					uint vertexOffset = 0;
					for (uint i = 0; i < shaderDesc.InputParameters; i++)
					{
						D3D11_SIGNATURE_PARAMETER_DESC inputDesc;
						pReflector->GetInputParameterDesc(i, &inputDesc);

						if (StrContains(StrToLower(inputDesc.SemanticName), "sv_"))
							continue;

						uint mask = inputDesc.Mask;
						IVertexElement elem = {};

						if ((mask & HLSL_MASK_XYZW) == HLSL_MASK_XYZW)
						{
							elem.format = VIF_FLOAT4;
							elem.size = sizeof(float) * 4;
						}
						else if ((mask & HLSL_MASK_XYZ) == HLSL_MASK_XYZ)
						{
							elem.format = VIF_FLOAT3;
							elem.size = sizeof(float) * 3;
						}
						else if ((mask & HLSL_MASK_XY) == HLSL_MASK_XY)
						{
							elem.format = VIF_FLOAT2;
							elem.size = sizeof(float) * 2;
						}

						elem.offset = vertexOffset;
						strncpy_s(elem.semantic, inputDesc.SemanticName, strlen(inputDesc.SemanticName));
						vertexOffset += elem.size;

						shader.vertexElements.push_back(elem);
					}
				}
				else if (type == SS_COMPUTE)
				{
					pReflector->GetThreadGroupSize(&shader.computeThreadGroupSize.x, &shader.computeThreadGroupSize.y, &shader.computeThreadGroupSize.z);
				}

				for (uint i = 0; i < shaderDesc.BoundResources; i++)
				{
					D3D11_SHADER_INPUT_BIND_DESC resDesc = {};
					pReflector->GetResourceBindingDesc(i, &resDesc);

					auto found = shader.resources.find(resDesc.Name);
					if (found == shader.resources.end())
					{
						IShaderResource& resource = shader.resources[resDesc.Name];
						strncpy_s(resource.name, resDesc.Name, strlen(resDesc.Name));
						resource.stages |= type;
						resource.texture.bindingCount = resDesc.BindCount;

						auto& bindInfo = _namedBindingLookup.at(resource.name);
						assert(!bindInfo.needsBindingsSet);
						resource.binding[SE_GFX_VULKAN] = bindInfo.bindings[SE_GFX_VULKAN];
						resource.binding[SE_GFX_D3D11] = bindInfo.bindings[SE_GFX_D3D11];
						resource.bindType = bindInfo.bindType;

						switch (resDesc.Type)
						{
						case D3D_SIT_TEXTURE:
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SIT_SAMPLER:
							resource.type = SRT_SAMPLER;
							break;
						case D3D_SIT_CBUFFER:
						{
							resource.type = SRT_BUFFER;

							ID3D11ShaderReflectionConstantBuffer* pBuffer = pReflector->GetConstantBufferByName(resDesc.Name);

							D3D11_SHADER_BUFFER_DESC bufferDesc;
							pBuffer->GetDesc(&bufferDesc);

							auto& buffer = resource.buffer;
							buffer.size = bufferDesc.Size;

							for (uint j = 0; j < bufferDesc.Variables; j++)
							{
								ID3D11ShaderReflectionVariable* pVar = pBuffer->GetVariableByIndex(j);

								D3D11_SHADER_TYPE_DESC typeDesc;
								pVar->GetType()->GetDesc(&typeDesc);

								D3D11_SHADER_VARIABLE_DESC varDesc;
								pVar->GetDesc(&varDesc);

								ShaderBufferVariable var;
								strncpy_s(var.name, varDesc.Name, strlen(varDesc.Name));
								var.numElements = typeDesc.Elements;
								var.offset = varDesc.StartOffset;
								var.size = varDesc.Size;

								String strType = StrToLower(typeDesc.Name);
								if (strType == "float4x4" || strType == "matrix")
								{
									var.type = SDT_MAT4;
								}
								else if (strType == "float3x3")
								{
									var.type = SDT_MAT3;
								}
								else if (strType == "float2x2")
								{
									var.type = SDT_MAT2;
								}
								if (strType == "float4")
								{
									var.type = SDT_FLOAT4;
								}
								else if (strType == "float3")
								{
									var.type = SDT_FLOAT3;
								}
								else if (strType == "float2")
								{
									var.type = SDT_FLOAT2;
								}
								else if (strType == "float")
								{
									var.type = SDT_FLOAT;
								}
								else if (typeDesc.Class == D3D_SHADER_VARIABLE_CLASS::D3D_SVC_STRUCT)
								{
									var.type = SDT_STRUCT;
								}

								buffer.variables[buffer.numVariables++] = var;
							}
						}
						break;
						case D3D_SIT_UAV_RWTYPED:
							resource.type = SRT_UNDEFINED;
							resource.texture.readOnly = false;
							break;
						default:
							resource.type = SRT_UNDEFINED;
							break;
						}

						switch (resDesc.Dimension)
						{
						case D3D_SRV_DIMENSION_TEXTURE1D:		  
							resource.texture.dimension = SRD_TEXTURE1D;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_TEXTURE1DARRAY:	  
							resource.texture.dimension = SRD_TEXTURE1DARRAY;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2D:		  
							resource.texture.dimension = SRD_TEXTURE2D;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2DARRAY:	  
							resource.texture.dimension = SRD_TEXTURE2DARRAY;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2DMS:		  
							resource.texture.dimension = SRD_TEXTURE2DMS;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:  
							resource.texture.dimension = SRD_TEXTURE2DMSARRAY;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_TEXTURE3D:		  
							resource.texture.dimension = SRD_TEXTURE3D;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_TEXTURECUBE:		  
							resource.texture.dimension = SRD_TEXTURECUBE;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:  
							resource.texture.dimension = SRD_TEXTURECUBEARRAY;
							resource.type = SRT_TEXTURE;
							break;
						case D3D_SRV_DIMENSION_UNKNOWN:
						case D3D_SRV_DIMENSION_BUFFER:
						case D3D_SRV_DIMENSION_BUFFEREX:		 
							break;
						default:
							break;
						}
					}
					else
					{
						(*found).second.stages |= type;
					}
				}
			}

			return true;
		}
		else
		{
			_lastErr = (char*)pErrorBlod->GetBufferPointer();
			Vector<String> shaderLines;
			StrSplit(hlsl, shaderLines, '\n');
			for (uint l = 0; l < shaderLines.size(); l++)
			{
				_lastErr += StrFormat("%d\t%s\n", l + 1, shaderLines[l].data());
			}
			return false;
		}

		return true;
	}

	bool ShaderCompiler::ParseShaderFile(String& output, const String& input, HashSet<String>& includeFiles)
	{
		Vector<String> lines;
		ConvertToLines(input, lines);

		for (uint i = 0; i < lines.size(); i++)
		{
			usize includePos = lines[i].find("#include");
			if (includePos != String::npos)
			{
				const String& line = lines[i];

				usize includeStart = line.find('\"');
				usize includeEnd = line.rfind('\"');
				if (includeStart != includeEnd && includeStart != String::npos)
				{
					String includeName = StrTrim(line.substr(includeStart + 1, includeEnd - includeStart - 1));
					String fileCheck = StrToLower(GetFileName(includeName));
					if (includeFiles.count(fileCheck))
						continue;

					includeFiles.insert(fileCheck);

					FileStream includeReader;
					if (!includeReader.OpenForRead((g_ShaderAuxDir + includeName).data()))
						return false;

					String includeText;
					if (!includeReader.ReadText(includeText))
						return false;

					if (!includeReader.Close())
						return false;

					String includeTextParsed;
					ParseShaderFile(includeTextParsed, includeText, includeFiles);
					output += includeTextParsed;
				}
			}
			else
			{
				output += lines[i];
			}
			output += "\n";
		}

		return true;
	}

	void ShaderCompiler::ConvertToLines(const String& input, Vector<String>& lines) const
	{
		String inText = StrRemove(input, '\r');

		Vector<String> tmpLines;
		StrSplit(inText, tmpLines, '\n');

		for (uint i = 0; i < tmpLines.size(); i++)
		{
			String line = tmpLines[i];
			usize commentPos = line.find("//");
			if (commentPos != String::npos)
				line = line.substr(0, commentPos);

			if (line.length())
				lines.push_back(line);
		}
	}

	bool CloseStreamFunc(FileStream& stream)
	{
		stream.Close();
		return false;
	}
#define CLOSE_AND_RETURN return CloseStreamFunc(stream)

	bool ShaderCompiler::MatchesCachedFile(const String& path, uint stageFlags)
	{
		FileStream stream;
		if (!stream.OpenForRead(path.c_str()))
		{
			stream.Close();
			return false;
		}

		uint version;
		if (!stream.Read(version))
			CLOSE_AND_RETURN;
		if (version != SHADER_COMPILER_VERSION)
			CLOSE_AND_RETURN;

		uint cachedFlags;
		if (!stream.Read(cachedFlags))
			CLOSE_AND_RETURN;

		if (cachedFlags != stageFlags)
			CLOSE_AND_RETURN;

		Vector<uint> stages;
		for (uint i = 0; i < SHADER_STAGE_ARRAY.size(); i++)
		{
			if (stageFlags & SHADER_STAGE_ARRAY[i])
				stages.push_back(SHADER_STAGE_ARRAY[i]);
		}

		for (uint i = 0; i < stages.size(); i++)
		{
			uint currStage;
			if (!stream.Read(currStage))
				CLOSE_AND_RETURN;

			String currText;
			if (!stream.Read(currText))
				CLOSE_AND_RETURN;

			auto found = _hlslShaderText.find(ShaderStage(currStage));
			if (found == _hlslShaderText.end())
				CLOSE_AND_RETURN;

			if (currText != (*found).second)
				CLOSE_AND_RETURN;
		}

		BaseShader::CreateInfo newInfo;

		if (!stream.ReadSimple(newInfo.resources))
			CLOSE_AND_RETURN;
		if (!stream.ReadSimple(newInfo.vertexElements))
			CLOSE_AND_RETURN;
		if (!stream.Read(&newInfo.computeThreadGroupSize, sizeof(_shaderInfo.computeThreadGroupSize)))
			CLOSE_AND_RETURN;

		for (uint i = 0; i < MAX_GRAPHICS_API_TYPES; i++)
		{
			if (!newInfo.vertexBinaries[i].Read(stream))
				CLOSE_AND_RETURN;
			if (!newInfo.pixelBinaries[i].Read(stream))
				CLOSE_AND_RETURN;
			if (!newInfo.geometryBinaries[i].Read(stream))
				CLOSE_AND_RETURN;
			if (!newInfo.computeBinaries[i].Read(stream))
				CLOSE_AND_RETURN;
		}

		_shaderInfo = newInfo;

		stream.Close();
		return true;
	}

	bool ShaderCompiler::WriteCachedFile(const String& path, uint stageFlags)
	{
		FileStream stream;
		if (!stream.OpenForWrite(path.c_str()))
		{
			stream.Close();
			return false;
		}

		if (!stream.Write(SHADER_COMPILER_VERSION))
			CLOSE_AND_RETURN;

		if (!stream.Write(stageFlags))
			CLOSE_AND_RETURN;

		Vector<uint> stages;
		for (uint i = 0; i < SHADER_STAGE_ARRAY.size(); i++)
		{
			if (stageFlags & SHADER_STAGE_ARRAY[i])
				stages.push_back(SHADER_STAGE_ARRAY[i]);
		}

		for (uint i = 0; i < stages.size(); i++)
		{
			if (!stream.Write(stages[i]))
				CLOSE_AND_RETURN;

			if (!stream.Write(_hlslShaderText.at(ShaderStage(stages[i]))))
				CLOSE_AND_RETURN;
		}

		if (!stream.WriteSimple(_shaderInfo.resources))
			CLOSE_AND_RETURN;
		if (!stream.WriteSimple(_shaderInfo.vertexElements))
			CLOSE_AND_RETURN;
		if (!stream.Write(&_shaderInfo.computeThreadGroupSize, sizeof(_shaderInfo.computeThreadGroupSize)))
			CLOSE_AND_RETURN;

		for (uint i = 0; i < MAX_GRAPHICS_API_TYPES; i++)
		{
			if (!_shaderInfo.vertexBinaries[i].Write(stream))
				CLOSE_AND_RETURN;
			if (!_shaderInfo.pixelBinaries[i].Write(stream))
				CLOSE_AND_RETURN;
			if (!_shaderInfo.geometryBinaries[i].Write(stream))
				CLOSE_AND_RETURN;
			if (!_shaderInfo.computeBinaries[i].Write(stream))
				CLOSE_AND_RETURN;
		}

		stream.Close();
		return true;
	}

	ShaderCompiler::NamedBindingInfo::NamedBindingInfo()
	{
		needsBindingsSet = true;
	}
}