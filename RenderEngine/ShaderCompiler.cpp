#include <d3dcompiler.h>
#include <direct.h>
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
	const uint SHADER_COMPILER_VERSION = 1;

	const uint HLSL_MASK_XYZW = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z | D3D_COMPONENT_MASK_W);
	const uint HLSL_MASK_XYZ = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z);
	const uint HLSL_MASK_XY = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y);

	const uint ENGINE_RESOURCE_COUNT = 3;

	void ShaderCompiler::InitShaderBindingNames(ShaderBindingType type)
	{
		ShaderBindingNames names;
		switch (type)
		{
		case SunEngine::SBT_CAMERA:
			names.bufferNames.push_back(ShaderStrings::CameraBufferName);
			break;
		case SunEngine::SBT_OBJECT:
			names.bufferNames.push_back(ShaderStrings::ObjectBufferName);
			break;
		case SunEngine::SBT_LIGHT:
			names.bufferNames.push_back(ShaderStrings::SunlightBufferName);
			names.bufferNames.push_back(ShaderStrings::PointlightBufferName);
			names.bufferNames.push_back(ShaderStrings::SpotlightBufferName);
			break;
		case SunEngine::SBT_ENVIRONMENT:
			names.bufferNames.push_back(ShaderStrings::EnvBufferName);
			names.bufferNames.push_back(ShaderStrings::FogBufferName);
			break;
		case SunEngine::SBT_SHADOW:
			names.bufferNames.push_back(ShaderStrings::ShadowBufferName);
			names.textureNames.push_back(ShaderStrings::ShadowTextureName);
			names.samplerNames.push_back(ShaderStrings::ShadowSamplerName);
			break;
		case SunEngine::SBT_SCENE:
			names.textureNames.push_back(ShaderStrings::SceneTextureName);
			names.textureNames.push_back(ShaderStrings::SceneSamplerName);
			names.samplerNames.push_back(ShaderStrings::DepthTextureName);
			names.samplerNames.push_back(ShaderStrings::DepthSamplerName);
			break;
		case SunEngine::SBT_BONES:
			names.bufferNames.push_back(ShaderStrings::SkinnedBoneBufferName);
			break;
		case SunEngine::SBT_MATERIAL:
			names.bufferNames.push_back(ShaderStrings::MaterialBufferName);
			names.bufferNames.push_back(ShaderStrings::TextureTransformBufferName);
			break;
		default:
			break;
		}

		for (uint i = 0; i < names.bufferNames.size(); i++)
			_bindingNameLookup.insert(names.bufferNames[i]);

		for (uint i = 0; i < names.textureNames.size(); i++)
			_bindingNameLookup.insert(names.textureNames[i]);

		for (uint i = 0; i < names.samplerNames.size(); i++)
			_bindingNameLookup.insert(names.samplerNames[i]);

		_bindingNames[type] = names;
	}
   
	String g_ShaderAuxDir = "";

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
	}

	void ShaderCompiler::SetDefines(const Vector<String>& defines)
	{
		_defines = defines;
	}

	void ShaderCompiler::SetVertexShaderSource(const String& vertexShader)
	{
		_vertexSource = vertexShader;
	}

	void ShaderCompiler::SetPixelShaderSource(const String& pixelShader)
	{
		_pixelSource = pixelShader;
	}

	bool ShaderCompiler::Compile(const String& uniqueName)
	{
		_shaderInfo.resources.clear();
		_shaderInfo.buffers.clear();
		_bindingNames.clear();
		_numUserTextures = 0;
		_numUserSamplers = 0;
		_uniqueName = uniqueName;

		for (uint i = 0; i <= SBT_MATERIAL; i++)
			InitShaderBindingNames((ShaderBindingType)i);

		uint shaderFlags = 0;

		if (_vertexSource.length())
		{
			PreProcessText(_vertexSource, _vertexHLSL, _vertexGLSL);
			shaderFlags |= SS_VERTEX;
		}

		if (_pixelSource.length())
		{
			PreProcessText(_pixelSource, _pixelHLSL, _pixelGLSL);
			shaderFlags |= SS_PIXEL;
		}

		String cacheDir =  g_ShaderAuxDir + "Cached/";
		int dirStatus = _mkdir(cacheDir.c_str());

		String cachedFile = cacheDir + _uniqueName + ".scached";
		if (_uniqueName.length())
		{
			if (MatchesCachedFile(cachedFile, shaderFlags))
				return true;
		}

		if (_vertexSource.length())
		{
			if (!CompileShader(SS_VERTEX))
				return false;
		}

		if (_pixelSource.length())
		{
			if (!CompileShader(SS_PIXEL))
				return false;
		}

		Vector<String> unusedBuffers, unusedResources;

		for (auto iter = _shaderInfo.buffers.begin(); iter != _shaderInfo.buffers.end(); ++iter)
		{
			if (strlen((*iter).second.name) == 0)
				unusedBuffers.push_back((*iter).first);
		}
		for (uint i = 0; i < unusedBuffers.size(); i++)
		{
			_shaderInfo.buffers.erase(unusedBuffers[i]);
		}

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

		Vector<Pair<String, String>> ResourceDefintions = 
		{
			{"b", "cbuffer"},
			{"t", "Texture2DArray"},
			{"t", "Texture2D"},
			{"s", "SamplerState"},
		};

		const String STR_REGISTER_PLACEHOLDER = "SHADER_COMPILER_REGISTER=";
		Vector<String> registerUpdateLines;

		for (uint i = 0; i < lines.size(); i++)
		{
			String line = lines[i];

			for (uint j = 0; j < ResourceDefintions.size(); j++)
			{
				usize resPos = lines[i].find(ResourceDefintions[j].second);
				if (resPos != String::npos)
				{
					bool valid = true;
					for (uint k = i; k < lines.size() && valid; k++)
					{
						//Add other things as needed to invalidate any resources which are not declerations(ie function arguments)
						if (lines[k].find('(') != String::npos || lines[k].find(')') != String::npos)
							valid = false;

						if (lines[k].find(';'))
							break;
					}

					if (valid)
					{
						String resName = lines[i].substr(resPos + ResourceDefintions[j].second.length() + 1);
						for (uint k = 0; k < resName.length(); k++)
						{
							if (isalpha(resName[k]) == 0)
							{
								resName = resName.substr(0, k);
								break;
							}
						}

						String strResType = ResourceDefintions[j].first;
						if (_bindingNameLookup.count(resName) == 0)
						{
							if (strResType == "b")
							{
								_bindingNames[SBT_MATERIAL].bufferNames.push_back(resName);
							}
							else if (strResType == "t")
							{
								_bindingNames[SBT_MATERIAL].textureNames.push_back(resName);
							}
							else if (strResType == "s")
							{
								_bindingNames[SBT_MATERIAL].samplerNames.push_back(resName);
							}
							_bindingNameLookup.insert(resName);
						}

						line = line + STR_REGISTER_PLACEHOLDER + resName;
					}
					break;
				}
			}

			registerUpdateLines.push_back(line);
		}

		struct NamedBindingInfo
		{
			ShaderBindingType bindType;
			String hlslRegister;
			uint bindings[MAX_GRAPHICS_API_TYPES];
		};

		uint bufferIndexCounter = 0;
		uint textureIndexCounter = 0;
		uint samplerIndexCounter = 0;
		StrMap<NamedBindingInfo> knownBindingInfo;

		for (uint i = 0; i <= SBT_MATERIAL; i++)
		{
			auto& names = _bindingNames.at((ShaderBindingType)i);

			uint vulkanBinding = 0; //vulkan bindings are local to the 'set' they are in, the set being '(ShaderBindingType)i';
			for (uint j = 0; j < names.bufferNames.size(); j++)
			{
				auto& namedBinding = knownBindingInfo[names.bufferNames[j]];
				namedBinding.bindType = (ShaderBindingType)i;
				namedBinding.bindings[SE_GFX_VULKAN] = vulkanBinding++;
				namedBinding.bindings[SE_GFX_D3D11] = bufferIndexCounter++;
				namedBinding.hlslRegister = "b";

				auto& shaderBuffer = _shaderInfo.buffers[names.bufferNames[j]];
				shaderBuffer.binding[SE_GFX_VULKAN] = namedBinding.bindings[SE_GFX_VULKAN];
				shaderBuffer.binding[SE_GFX_D3D11] = namedBinding.bindings[SE_GFX_D3D11];
				shaderBuffer.bindType = (ShaderBindingType)i;
				shaderBuffer.size = 0;
			}

			for (uint j = 0; j < names.textureNames.size(); j++)
			{
				auto& namedBinding = knownBindingInfo[names.textureNames[j]];
				namedBinding.bindType = (ShaderBindingType)i;
				namedBinding.bindings[SE_GFX_VULKAN] = vulkanBinding++;
				namedBinding.bindings[SE_GFX_D3D11] = textureIndexCounter++;
				namedBinding.hlslRegister ="t";

				auto& shaderResource = _shaderInfo.resources[names.textureNames[j]];
				shaderResource.binding[SE_GFX_VULKAN] = namedBinding.bindings[SE_GFX_VULKAN];
				shaderResource.binding[SE_GFX_D3D11] = namedBinding.bindings[SE_GFX_D3D11];
				shaderResource.bindType = (ShaderBindingType)i;
				shaderResource.bindingCount = 0;
			}

			for (uint j = 0; j < names.samplerNames.size(); j++)
			{
				auto& namedBinding = knownBindingInfo[names.samplerNames[j]];
				namedBinding.bindType = (ShaderBindingType)i;
				namedBinding.bindings[SE_GFX_VULKAN] = vulkanBinding++;
				namedBinding.bindings[SE_GFX_D3D11] = samplerIndexCounter++;
				namedBinding.hlslRegister = "s";

				auto& shaderResource = _shaderInfo.resources[names.samplerNames[j]];
				shaderResource.binding[SE_GFX_VULKAN] = namedBinding.bindings[SE_GFX_VULKAN];
				shaderResource.binding[SE_GFX_D3D11] = namedBinding.bindings[SE_GFX_D3D11];
				shaderResource.bindType = (ShaderBindingType)i;
				shaderResource.bindingCount = 0;
			}
		}

		for (uint i = 0; i < registerUpdateLines.size(); i++)
		{
			usize placeholderPos = registerUpdateLines[i].find(STR_REGISTER_PLACEHOLDER);
			if (placeholderPos != String::npos)
			{
				auto& namedBinding = knownBindingInfo.at(registerUpdateLines[i].substr(placeholderPos + STR_REGISTER_PLACEHOLDER.length()));
				String line = registerUpdateLines[i].substr(0, placeholderPos);

				if(StrContains(line, ";"))
					outHLSL += StrFormat("%s: register(%s%d);\n", StrRemove(line, ';').c_str(), namedBinding.hlslRegister.c_str(), namedBinding.bindings[SE_GFX_D3D11]);
				else
					outHLSL += StrFormat("%s: register(%s%d)\n", line.c_str(), namedBinding.hlslRegister.c_str(), namedBinding.bindings[SE_GFX_D3D11]);
				outGLSL += StrFormat("[[vk::binding(%d, %d)]]\n%s\n", namedBinding.bindings[SE_GFX_VULKAN], (uint)namedBinding.bindType, line.c_str());
			}
			else
			{
				outHLSL += registerUpdateLines[i] + "\n";
				outGLSL += registerUpdateLines[i] + "\n";
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
		String* pHLSL = NULL;
		String* pGLSL = NULL;
		if (type == SS_VERTEX)
		{
			targetHLSL = "vs_5_0";
			targetGLSL = "vert";
			pHLSL = &_vertexHLSL;
			pGLSL = &_vertexGLSL;
		}
		else if (type == SS_PIXEL)
		{
			targetHLSL = "ps_5_0";
			targetGLSL = "frag";
			pHLSL = &_pixelHLSL;
			pGLSL = &_pixelGLSL;
		}
		else
		{
			return false;
		}

		BaseShader::CreateInfo& shader = _shaderInfo;

		MemBuffer* pBinBuffer = 0;
		if (type == SS_VERTEX) pBinBuffer = shader.vertexBinaries;
		if (type == SS_PIXEL) pBinBuffer = shader.pixelBinaries;
		if (type == SS_GEOMETRY) pBinBuffer = shader.geometryBinaries;

		FileStream fr;

		ID3DBlob* pShaderBlod = NULL;
		ID3DBlob* pErrorBlod = NULL;
		if (D3DCompile(pHLSL->data(), pHLSL->size(), NULL, NULL, NULL, "main", targetHLSL.data(), 0, 0, &pShaderBlod, &pErrorBlod) == S_OK)
		{
			pBinBuffer[SE_GFX_D3D11].SetSize((uint)pShaderBlod->GetBufferSize());
			pBinBuffer[SE_GFX_D3D11].SetData(pShaderBlod->GetBufferPointer(), (uint)pShaderBlod->GetBufferSize());

			String glslInPath = "ShaderCompilerTempShader.glsl";
			String glslOutPath = "ShaderCompilerTempShader.spv";
			FileStream fw;
			fw.OpenForWrite(glslInPath.data());
			fw.WriteText(*pGLSL);
			fw.Close();

			String compileSpvCmd = StrFormat("glslangValidator -D -S %s -e main -o %s -V %s", targetGLSL.data(), glslOutPath.data(), glslInPath.data());
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

						if (StrContains(inputDesc.SemanticName, "SV_"))
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

				for (uint i = 0; i < shaderDesc.ConstantBuffers; i++)
				{
					ID3D11ShaderReflectionConstantBuffer* pBuffer = pReflector->GetConstantBufferByIndex(i);

					D3D11_SHADER_BUFFER_DESC bufferDesc;
					pBuffer->GetDesc(&bufferDesc);

					auto found = shader.buffers.find(bufferDesc.Name);
					if (found != shader.buffers.end())
					{
						IShaderBuffer& buffer = (*found).second;
						strncpy_s(buffer.name, bufferDesc.Name, strlen(bufferDesc.Name));
						buffer.size = max(buffer.size, bufferDesc.Size);
						buffer.stages |= type;

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
					else
					{
						(*found).second.stages |= type;
					}
				}

				for (uint i = 0; i < shaderDesc.BoundResources; i++)
				{
					D3D11_SHADER_INPUT_BIND_DESC resDesc = {};
					pReflector->GetResourceBindingDesc(i, &resDesc);

					if (resDesc.Type == D3D_SIT_TEXTURE || resDesc.Type == D3D_SIT_SAMPLER)
					{
						auto found = shader.resources.find(resDesc.Name);
						if (found != shader.resources.end())
						{
							IShaderResource& resource = (*found).second;
							strncpy_s(resource.name, resDesc.Name, strlen(resDesc.Name));
							resource.stages |= type;
							resource.bindingCount = max(resource.bindingCount, resDesc.BindCount);

							if (resDesc.Type == D3D_SIT_TEXTURE)
							{
								resource.type = SRT_TEXTURE;
								if (resDesc.Dimension == D3D_SRV_DIMENSION_TEXTURE2D)
									resource.dimension = SRD_TEXTURE_2D;
								if (resDesc.Dimension == D3D_SRV_DIMENSION_TEXTURE2DARRAY)
									resource.dimension = SRD_TEXTURE_ARRAY;
								if (resDesc.Dimension == D3D_SRV_DIMENSION_TEXTURECUBE)
									resource.dimension = SRD_TEXTURE_CUBE;
							}
							else if (resDesc.Type == D3D_SIT_SAMPLER)
							{
								resource.type = SRT_SAMPLER;
							}
						}
					}
				}
			}

			return true;
		}
		else
		{
			_lastErr = (char*)pErrorBlod->GetBufferPointer();
			Vector<String> shaderLines;
			StrSplit(*pHLSL, shaderLines, '\n');
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
					if (includeFiles.count(includeName))
						continue;

					includeFiles.insert(includeName);

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

		uint numStages = 0;
		if (stageFlags & SS_VERTEX) numStages++;
		if (stageFlags & SS_PIXEL) numStages++;

		for (uint i = 0; i < numStages; i++)
		{
			uint currStage;
			if (!stream.Read(currStage))
				CLOSE_AND_RETURN;

			String currText;
			if (!stream.Read(currText))
				CLOSE_AND_RETURN;

			bool textMatches = false;
			switch (currStage)
			{
			case SS_VERTEX:
				textMatches = currText == _vertexHLSL;
				break;
			case SS_PIXEL:
				textMatches = currText == _pixelHLSL;
				break;
			default:
				break;
			}

			if (!textMatches)
				CLOSE_AND_RETURN;
		}

		BaseShader::CreateInfo newInfo;

		if (!stream.ReadSimple(newInfo.buffers))
			CLOSE_AND_RETURN;
		if (!stream.ReadSimple(newInfo.resources))
			CLOSE_AND_RETURN;
		if (!stream.ReadSimple(newInfo.vertexElements))
			CLOSE_AND_RETURN;

		for (uint i = 0; i < MAX_GRAPHICS_API_TYPES; i++)
		{
			if (!newInfo.vertexBinaries[i].Read(stream))
				CLOSE_AND_RETURN;
			if (!newInfo.pixelBinaries[i].Read(stream))
				CLOSE_AND_RETURN;
			if (!newInfo.geometryBinaries[i].Read(stream))
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

		if (stageFlags & SS_VERTEX)
		{
			uint currStage = SS_VERTEX;
			if (!stream.Write(currStage))
				CLOSE_AND_RETURN;

			if (!stream.Write(_vertexHLSL))
				CLOSE_AND_RETURN;
		}

		if (stageFlags & SS_PIXEL)
		{
			uint currStage = SS_PIXEL;
			if (!stream.Write(currStage))
				CLOSE_AND_RETURN;

			if (!stream.Write(_pixelHLSL))
				CLOSE_AND_RETURN;
		}

		if (!stream.WriteSimple(_shaderInfo.buffers))
			CLOSE_AND_RETURN;
		if (!stream.WriteSimple(_shaderInfo.resources))
			CLOSE_AND_RETURN;
		if (!stream.WriteSimple(_shaderInfo.vertexElements))
			CLOSE_AND_RETURN;

		for (uint i = 0; i < MAX_GRAPHICS_API_TYPES; i++)
		{
			if (!_shaderInfo.vertexBinaries[i].Write(stream))
				CLOSE_AND_RETURN;
			if (!_shaderInfo.pixelBinaries[i].Write(stream))
				CLOSE_AND_RETURN;
			if (!_shaderInfo.geometryBinaries[i].Write(stream))
				CLOSE_AND_RETURN;
		}

		stream.Close();
		return true;
	}
}