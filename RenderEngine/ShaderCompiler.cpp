#include <d3dcompiler.h>

#include "GraphicsAPIDef.h"
#include "FileBase.h"
#include "StringUtil.h"
#include "BaseShader.h"

#include "MemBuffer.h"
#include "ShaderCompiler.h"

namespace SunEngine
{
	const uint HLSL_MASK_XYZW = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z | D3D_COMPONENT_MASK_W);
	const uint HLSL_MASK_XYZ = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z);
	const uint HLSL_MASK_XY = (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y);

	const uint ENGINE_RESOURCE_COUNT = 2;
   
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

	bool ShaderCompiler::Compile()
	{
		_shaderInfo.resources.clear();
		_shaderInfo.buffers.clear();
		_numUserTextures = 0;
		_numUserSamplers = 0;

		if (_vertexSource.length())
		{
			if (!CompileShader(SS_VERTEX, _vertexSource))
				return false;
		}

		if (_pixelSource.length())
		{
			if (!CompileShader(SS_PIXEL, _pixelSource))
				return false;
		}

		return true;
	}

	void ShaderCompiler::GetBinding(const String& name, const String& type, uint* pBindings, ShaderBindingType& bindType)
	{
		if (type == "cbuffer")
		{
			auto found = _shaderInfo.buffers.find(name);
			if (found != _shaderInfo.buffers.end())
			{
				pBindings[SE_GFX_VULKAN] = (*found).second.binding[SE_GFX_VULKAN];
				pBindings[SE_GFX_D3D11] = (*found).second.binding[SE_GFX_D3D11];
				bindType = (*found).second.bindType;
				return;
			}
		}
		else 
		{
			auto found = _shaderInfo.resources.find(name);
			if (found != _shaderInfo.resources.end())
			{
				pBindings[SE_GFX_VULKAN] = (*found).second.binding[SE_GFX_VULKAN];
				pBindings[SE_GFX_D3D11] = (*found).second.binding[SE_GFX_D3D11];
				bindType = (*found).second.bindType;
				return;
			}
		}

		if (name == ShaderStrings::CameraBufferName)
		{
			pBindings[SE_GFX_D3D11] =  0;
			pBindings[SE_GFX_VULKAN] =  0;
			bindType = SBT_CAMERA;
		}
		else if (name == ShaderStrings::ObjectBufferName)
		{
			pBindings[SE_GFX_D3D11] =  1;
			pBindings[SE_GFX_VULKAN] =  0;
			bindType = SBT_OBJECT;
		}
		else if (name == ShaderStrings::SkinnedBoneBufferName)
		{
			pBindings[SE_GFX_D3D11] = 2;
			pBindings[SE_GFX_VULKAN] = 1;
			bindType = SBT_BONES;
		}
		else if (name == ShaderStrings::SunlightBufferName)
		{
			pBindings[SE_GFX_D3D11] = 3;
			pBindings[SE_GFX_VULKAN] = 0;
			bindType = SBT_LIGHT;
		}
		else if (name == ShaderStrings::PointlightBufferName)
		{
			pBindings[SE_GFX_D3D11] = 4;
			pBindings[SE_GFX_VULKAN] = 1;
			bindType = SBT_LIGHT;
		}
		else if (name == ShaderStrings::SpotlightBufferName)
		{
			pBindings[SE_GFX_D3D11] = 5;
			pBindings[SE_GFX_VULKAN] = 2;
			bindType = SBT_LIGHT;
		}
		else if (name == ShaderStrings::EnvBufferName)
		{
			pBindings[SE_GFX_D3D11] =  6;
			pBindings[SE_GFX_VULKAN] =  0;
			bindType = SBT_ENVIRONMENT;
		}
		else if (name == ShaderStrings::FogBufferName)
		{
			pBindings[SE_GFX_D3D11] =  7;
			pBindings[SE_GFX_VULKAN] =  1;
			bindType = SBT_ENVIRONMENT;
		}
		else if (name == ShaderStrings::MaterialBufferName)
		{
			pBindings[SE_GFX_D3D11] = 8;
			pBindings[SE_GFX_VULKAN] = 0;
			bindType = SBT_MATERIAL;
		}
		else if (name == ShaderStrings::TextureTransformBufferName)
		{
			pBindings[SE_GFX_D3D11] = 9;
			pBindings[SE_GFX_VULKAN] = 1;
			bindType = SBT_MATERIAL;
		}
		else if (name == ShaderStrings::SceneTextureName)
		{
			pBindings[SE_GFX_D3D11] =  0;
			pBindings[SE_GFX_VULKAN] =  0;
			bindType = SBT_SCENE;
		}
		else if (name == ShaderStrings::DepthTextureName)
		{
			pBindings[SE_GFX_D3D11] =  1;
			pBindings[SE_GFX_VULKAN] =  1;
			bindType = SBT_SCENE;
		}
		else if (name == ShaderStrings::SceneSamplerName)
		{
			pBindings[SE_GFX_D3D11] =  0;
			pBindings[SE_GFX_VULKAN] =  2;
			bindType = SBT_SCENE;
		}
		else if (name == ShaderStrings::DepthSamplerName)
		{
			pBindings[SE_GFX_D3D11] =  1;
			pBindings[SE_GFX_VULKAN] =  3;
			bindType = SBT_SCENE;
		}
		else
		{
			if (type != "cbuffer")
			{
				bool isSampler = type == "SamplerState";

				pBindings[SE_GFX_D3D11] = ENGINE_RESOURCE_COUNT + (isSampler ? _numUserSamplers : _numUserTextures);
				pBindings[SE_GFX_VULKAN] =  (ushort)(2 + _numUserSamplers + _numUserTextures);
				bindType = SBT_MATERIAL;

				if (isSampler) _numUserSamplers++;
				else _numUserTextures++;
			}
			else
			{
				//TODO SUMTHIN FAIL??
			}
		}

		if (type == "cbuffer")
		{
			_shaderInfo.buffers[name].binding[SE_GFX_VULKAN] = pBindings[SE_GFX_VULKAN];
			_shaderInfo.buffers[name].binding[SE_GFX_D3D11] = pBindings[SE_GFX_D3D11];
			_shaderInfo.buffers[name].bindType = bindType;
			_shaderInfo.buffers[name].size = 0;
		}
		else
		{
			_shaderInfo.resources[name].binding[SE_GFX_VULKAN] = pBindings[SE_GFX_VULKAN];
			_shaderInfo.resources[name].binding[SE_GFX_D3D11] = pBindings[SE_GFX_D3D11];
			_shaderInfo.resources[name].bindType = bindType;
			_shaderInfo.resources[name].bindingCount = 0;
		}
	}

	void ShaderCompiler::PreProcessText(const String& inText, String& outHLSL, String& outGLSL)
	{
		Vector<String> lines;
		StrSplit(StrRemove(inText, '\r'), lines, '\n');

		outGLSL.clear();
		outHLSL.clear();

		Vector<Pair<String, String>> ResourceDefintions = 
		{
			{"b", "cbuffer"},
			{"t", "Texture2DArray"},
			{"t", "Texture2D"},
			{"s", "SamplerState"},
		};

		for (uint i = 0; i < lines.size(); i++)
		{
			String hlslLine = lines[i];
			String glslLine = lines[i];

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

						uint bindings[32] = {};
						ShaderBindingType bindType;
						GetBinding(resName, ResourceDefintions[j].second, bindings, bindType);

						hlslLine = StrRemove(hlslLine, ';');
						bool removedEnd = hlslLine.length() != lines[i].length();
						hlslLine += StrFormat(": register(%s%d)", ResourceDefintions[j].first.data(), bindings[SE_GFX_D3D11]);
						if (removedEnd)
							hlslLine += ";";

						glslLine = StrFormat("[[vk::binding(%d, %d)]]\n", bindings[SE_GFX_VULKAN], (uint)bindType) + glslLine;
					}

					break;
				}
			}

			outHLSL += hlslLine;
			outHLSL += "\n";
			outGLSL += glslLine;
			outGLSL += "\n";
		}
	}

	bool ShaderCompiler::CompileShader(ShaderStage type, const String& source)
	{
		String shaderText;
		_includedFiles.clear();
		ParseShaderFile(shaderText, source);

		String targetHLSL, targetGLSL;
		if (type == SS_VERTEX)
		{
			targetHLSL = "vs_5_0";
			targetGLSL = "vert";
		}
		else if (type == SS_PIXEL)
		{
			targetHLSL = "ps_5_0";
			targetGLSL = "frag";
		}

		String hlslText, glslText;
		PreProcessText(shaderText, hlslText, glslText);

		String shaderHeader = "#pragma pack_matrix( row_major )\n";
		for (uint i = 0; i < _defines.size(); i++)
		{
			shaderHeader += "#define " + _defines[i] + "\n";
		}

		hlslText = shaderHeader + hlslText;
		BaseShader::CreateInfo& shader = _shaderInfo;

		MemBuffer* pBinBuffer = 0;
		if (type == SS_VERTEX) pBinBuffer = shader.vertexBinaries;
		if (type == SS_PIXEL) pBinBuffer = shader.pixelBinaries;
		if (type == SS_GEOMETRY) pBinBuffer = shader.geometryBinaries;

		ID3DBlob* pShaderBlod = NULL;
		ID3DBlob* pErrorBlod = NULL;
		if (D3DCompile(hlslText.data(), hlslText.size(), NULL, NULL, NULL, "main", targetHLSL.data(), 0, 0, &pShaderBlod, &pErrorBlod) == S_OK)
		{
			pBinBuffer[SE_GFX_D3D11].SetSize((uint)pShaderBlod->GetBufferSize());
			pBinBuffer[SE_GFX_D3D11].SetData(pShaderBlod->GetBufferPointer(), (uint)pShaderBlod->GetBufferSize());

			glslText = shaderHeader + glslText;

			String glslInPath = "ShaderCompilerTempShader.glsl";
			String glslOutPath = "ShaderCompilerTempShader.spv";
			FileStream fw;
			fw.OpenForWrite(glslInPath.data());
			fw.WriteText(glslText);
			fw.Close();

			FileStream fr;

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

							buffer.Variables.push_back(var);
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
			StrSplit(hlslText, shaderLines, '\n');
			for (uint l = 0; l < shaderLines.size(); l++)
			{
				_lastErr += StrFormat("%d\t%s\n", l + 1, shaderLines[l].data());
			}
			return false;
		}

		return true;
	}

	bool ShaderCompiler::ParseShaderFile(String& output, const String& input)
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
					if (_includedFiles.count(includeName))
						continue;

					_includedFiles.insert(includeName);

					FileStream includeReader;
					if (!includeReader.OpenForRead((g_ShaderAuxDir + includeName).data()))
						return false;

					String includeText;
					if (!includeReader.ReadText(includeText))
						return false;

					if (!includeReader.Close())
						return false;

					String includeTextParsed;
					ParseShaderFile(includeTextParsed, includeText);
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
}