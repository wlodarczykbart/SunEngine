#include <assert.h>
#include <direct.h>
#include "StringUtil.h"
#include "FileWriter.h"
#include "FileReader.h"
#include "HLSL_To_GLSL.h"
#include "VulkanShader.h"
#include "ShaderCompiler.h"

#define CAMERA_BUFFER_NAME "CameraBuffer"
#define OBJECT_BUFFER_NAME "ObjectBuffer"
#define MATERIAL_BUFFER_NAME "MaterialBuffer"
#define SUN_LIGHT_BUFFER_NAME "SunlightBuffer"
#define FOG_BUFFER_NAME "FogBuffer"
#define TEXTURE_TRANSFORM_BUFFER_NAME "TextureTransformBuffer"
#define SKINNED_BONE_BUFFER_NAME "SkinnedBoneBuffer"
#define ENV_BUFFER_NAME "EnvBuffer"
#define SHADOW_BUFFER_NAME "ShadowBuffer"
#define SAMPLE_SCENE_NAME "SampleSceneBuffer"

#define PACK_MATRIX_STR "#pragma pack_matrix(row_major)\r\n"
#define APPLY_LIGHTING_STR "#define APPLY_LIGHTING 1\r\n"
#define APPLY_FOG_STR "#define APPLY_FOG 1\r\n"
#define APPLY_ALPHA_TEST_STR "#define APPLY_ALPHA_TEST 1\r\n"
#define RENDER_NORMALS_STR "#define RENDER_NORMALS 1\r\n"
#define RENDER_VIEW_VECTOR_STR "#define RENDER_VIEW_VECTOR 1\r\n"
#define APPLY_SHADOWS_STR "#define APPLY_SHADOWS 1\r\n"

#define MAPPING_TOKEN_NAME "//Mapping"
#define SAMPLER_STATE_NAME "SamplerState"
#define TEXTURE_2D_NAME "Texture2D"
#define TEXTURE_2D_ARRAY_NAME "Texture2DArray"
#define TEXTURE_CUBE_NAME "TextureCube"
#define CBUFFER_NAME "cbuffer"

#define BEGIN_PASS_HINT "BEGIN_PASS"
#define END_PASS_HINT "END_PASS"

#define INJECT_PS_STRUCT_HINT "##INJECT_PS_IN##"

namespace SunEngine
{
namespace ShaderCompiler
{

const Vector<char> CodeSeperators =
{
	'{',
	'}',
	'(',
	')',
	';',
	',',
	'=',
};

const Vector<char> CodeReplace =
{
	'\r',
	'\n',
	'\t'
};

const StrMap<String> GLSLExtMap
{
	{ "vs", "vert" },
	{ "ps", "frag" },
	{ "gs", "geom" },
};

const StrMap<HLSL_To_GLSL::EShaderType> GLSLTypeMap
{
	{ "vs", HLSL_To_GLSL::ST_VERT },
	{ "ps", HLSL_To_GLSL::ST_FRAG },
	{ "gs", HLSL_To_GLSL::ST_GEOM },
};

const Vector<Pair<String, uint>> DefinedBufferMap
{
	{ CAMERA_BUFFER_NAME, SBL_CAMERA_BUFFER },
	{ OBJECT_BUFFER_NAME, SBL_OBJECT_BUFFER },
	{ MATERIAL_BUFFER_NAME, SBL_MATERIAL },
	{ SUN_LIGHT_BUFFER_NAME, SBL_SUN_LIGHT },
	{ TEXTURE_TRANSFORM_BUFFER_NAME, SBL_TEXTURE_TRANSFORM },
	{ FOG_BUFFER_NAME, SBL_FOG },
	{ SKINNED_BONE_BUFFER_NAME, SBL_SKINNED_BONES },
	{ ENV_BUFFER_NAME, SBL_ENV },
	{ SHADOW_BUFFER_NAME, SBL_SHADOW_BUFFER },
	{ SAMPLE_SCENE_NAME, SBL_SAMPLE_SCENE },
};

const Vector<Pair<String, uint>> DefinedTextureMap
{
	{ SE_SCENE_TEXTURE_NAME, STL_SCENE },
	{ SE_DEPTH_TEXTURE_NAME, STL_DEPTH },
	{ SE_SHADOW_TEXTURE_NAME, STL_SHADOW },
};

const Vector<Pair<String, uint>> DefinedSamplerMap
{
	{ SE_SCENE_SAMPLER_NAME, STL_SCENE },
	{ SE_DEPTH_SAMPLER_NAME, STL_DEPTH },
	{ SE_SHADOW_SAMPLER_NAME, STL_SHADOW },
};

struct ShaderPassData
{
	String shaderText;
	HashSet<String> usedResources;
};

struct ShaderOptions
{
	bool lighting;
	bool shadows;
	bool fog;
	bool debugNormals;
	bool debugViewVector;
	bool alphaTest;
	bool noTextureTransform;

	Vector<String> macros;

	ShaderOptions()
	{
		lighting = false;
		shadows = false;
		fog = false;
		debugNormals = false;
		debugViewVector = false;
		alphaTest = false;
		noTextureTransform = false;
	}
};

bool ProcessShader(const String& shaderFile, const ShaderOptions& options, StrMap<ShaderPassData>& outputShaderMap, StrMap<IShaderResource>& bufferResources, StrMap<IShaderResource>& textureResources, StrMap<IShaderResource>& samplerResources, String& errorStr);
void ParseShader(const String &code, const String &type, StrMap<APIVertexElement>& inputElements);
String AppendTextureTransforms(const String &code, const StrMap<IShaderResource>& bindingMap);
void Tokenize(const String &line, Vector<String> &parts);
Vector<String> AppendRegisterToResource(String& shaderText, const String& resourceType, const String& registerType, const String& ext, StrMap<IShaderResource>& existingResMap);
bool TokenIsShaderResouce(const String& token);
String AddLineNumbers(const String& str);

bool Compile(int argc, const char ** argv, String& errStr, ConfigFile* pConfig)
{
	ConfigFile defaultConfig;
	ConfigFile* Config;
	String* ErrorString;

	if (pConfig)
	{
		Config = pConfig;
	}
	else
	{
		Config = &defaultConfig;
	}

	ErrorString = &errStr;
	ErrorString->clear();
	Config->Clear();
	ShaderOptions options;

	if (argc > 2)
	{
		Config->AddSection("source");
		Config->AddSection("dx_shaders");
		Config->AddSection("vk_shaders");
		Config->AddSection("shader_resources");
		Config->AddSection("input_elements");
		Config->AddSection("shader_info");
		Config->AddSection("shader_passes");

		String cmdStr;
		String fxcPath;

		String shaderName = argv[1];
		cmdStr += GetFileName(shaderName) + " ";

		Vector<String> compileTypes;
		Map<String, String> compileOverrideFiles;

		for (int i = 2; i < argc; i++)
		{
			String option = argv[i];

			uint nCompiled = compileTypes.size();

			bool bAddOption = true;
			if (StrStartsWith(option, "-vs"))
			{
				compileTypes.push_back("vs");
			}
			if (StrStartsWith(option, "-ps"))
			{
				compileTypes.push_back("ps");
			}
			if (StrStartsWith(option, "-gs"))
			{
				compileTypes.push_back("gs");
			}	
			else if (option == "-lit")
			{
				options.lighting = true;
			}
			else if (option == "-fog")
			{
				options.fog = true;
			}
			else if (option == "-shadows")
			{
				options.shadows = true;
			}
			else if (StrStartsWith(option, "-fxc"))
			{
				fxcPath = option.substr(4);
				bAddOption = false;
			}
			else if (StrStartsWith(option, "-normals"))
			{
				options.debugNormals = true;
			}
			else if (StrStartsWith(option, "-viewVec"))
			{
				options.debugViewVector = true;
			}
			else if (StrStartsWith(option, "-alphaTest"))
			{
				options.alphaTest = true;
			}
			else if (StrStartsWith(option, "-D"))
			{
				options.macros.push_back(option.substr(2));
			}
			else if (StrStartsWith(option, "-noTexTransform"))
			{
				options.noTextureTransform = true;
			}

			if(bAddOption)
				cmdStr += option + " ";

			if (nCompiled < compileTypes.size())
			{
				//is there a relative file path?
				if (option.size() > 3)
				{
					compileOverrideFiles[compileTypes[compileTypes.size() - 1]] = option.substr(3);
				}
			}
		}

		ConfigSection* pInputSection = Config->GetSection("source");
		pInputSection->SetString("cmdLine", cmdStr.data());

		StrMap<APIVertexElement> inputElements;

		StrMap<IShaderResource> bufferMap;
		StrMap<IShaderResource> textureMap;
		StrMap<IShaderResource> samplerMap;

		for (uint i = 0; i < DefinedBufferMap.size(); i++)
		{
			IShaderResource& res = bufferMap[DefinedBufferMap[i].first];
			res.name = DefinedBufferMap[i].first;
			res.binding = DefinedBufferMap[i].second;
		}

		for (uint i = 0; i < DefinedTextureMap.size(); i++)
		{
			IShaderResource& res = textureMap[DefinedTextureMap[i].first];
			res.name = DefinedTextureMap[i].first;
			res.binding = DefinedTextureMap[i].second;
		}

		for (uint i = 0; i < DefinedSamplerMap.size(); i++)
		{
			IShaderResource& res = samplerMap[DefinedSamplerMap[i].first];
			res.name = DefinedSamplerMap[i].first;
			res.binding = DefinedSamplerMap[i].second;
		}

		String cmdFilename = "ShaderCompileCommandResult.txt";
		FileReader cmdReader;

		StrMap<HashSet<String>> shaderPassResources;

		String subDir = "compiled\\";
		String outputDir = GetDirectory(shaderName) + "\\" + subDir;
		String basePath = GetDirectory(shaderName);
		int dirCreated = _mkdir(outputDir.data());

		for (uint i = 0; i < compileTypes.size(); i++)
		{
			String type = compileTypes[i];
			String shaderFile = shaderName + "." + type;
			StrMap<String>::iterator overrideIter = compileOverrideFiles.find(type);
			if (overrideIter != compileOverrideFiles.end())
			{
				String dir = GetDirectory(shaderFile);
				shaderFile = dir + "\\" + (*overrideIter).second + "." + type;
			}

			pInputSection->SetString(type.data(), shaderFile.substr(basePath.size()).data());

			StrMap<ShaderPassData> processedShaderMap;
			if (!ProcessShader(shaderFile, options, processedShaderMap, bufferMap, textureMap, samplerMap, *ErrorString))
			{
				return false;
			}

			OrderedStrMap<String> d3dBlock;
			OrderedStrMap<String> spvBlock;

			for (auto iter = processedShaderMap.begin(); iter != processedShaderMap.end(); ++iter)
			{
				String shaderPass = (*iter).first;
				String& shaderText = (*iter).second.shaderText;

				String processedPath = outputDir;
				processedPath += GetFileNameNoExt(shaderFile);
				processedPath += shaderPass;
				processedPath += ".";
				processedPath += GetExtension(shaderFile);

				FileWriter fw;
				if (!fw.Open(processedPath.data()))
					return false;
				fw.Write(shaderText.data(), shaderText.size());
				fw.Close();
				
				ParseShader(shaderText, type, inputElements);

				String csoFile = processedPath + ".cso";
				String compileCmd = StrFormat("%s \"%s\" /T %s_5_0 /E main /Fe \"%s\" /Fo \"%s\"", 
					fxcPath.data(), processedPath.data(), type.data(), cmdFilename.data(), csoFile.data());
				int compiled = system(compileCmd.data());
				if (compiled == 0) //success
				{
					String fileName = GetFileName(csoFile).data();
					d3dBlock[shaderPass] = subDir + fileName;

					Vector<String> lines;
					StrSplit(shaderText, lines, '\n');

					String registerHint = "register(";

					Vector<String> glslLines;
					for (uint l = 0; l < lines.size(); l++)
					{
						String line = lines[l];
						glslLines.push_back(line);

						usize regPos = line.find(registerHint);
						if (regPos != String::npos)
						{
							regPos += registerHint.size();
							char regNum = line[regPos];

							regPos++;
							uint regEnd = line.find(')', regPos);
							uint binding = StrToInt(line.substr(regPos, regEnd - regPos));

							ShaderResourceType srt = SRT_UNSUPPORTED;
							StrMap<IShaderResource>* pResMap = 0;
							if (regNum == 'b')
							{
								srt = SRT_CONST_BUFFER;
								pResMap = &bufferMap;
							}
							else if (regNum == 't')
							{
								srt = SRT_TEXTURE;
								pResMap = &textureMap;
							}
							else if (regNum == 's')
							{
								srt = SRT_SAMPLER;
								pResMap = &samplerMap;
							}

							uint bindingNum, setNumber;
							bool foundSetAndBinding = VulkanShader::QuerySetAndBinding(srt, binding, setNumber, bindingNum);
							assert(foundSetAndBinding);

							String attribute = StrFormat("[[vk::binding(%d, %d)]]\n", bindingNum, setNumber);

							String resName;
							for (auto resIter = pResMap->begin(); resIter != pResMap->end(); ++resIter)
							{
								if (binding == (*resIter).second.binding)
								{
									resName = (*resIter).first;
									break;
								}
							}

							for (int l1 = (int)l; l1 >= 0; l1--)
							{
								if (StrContains(glslLines[l1], resName.data()))
								{
									glslLines[l1] = attribute + glslLines[l1];
									break;
								}
							}

							assert(resName.length());


						}

						//register(%s%d)
					}

					String glslText;
					for (uint l = 0; l < glslLines.size(); l++)
					{
						glslText += glslLines[l] + "\n";
					}

					String glslFile = processedPath;
					glslFile = glslFile.replace(glslFile.find(type), type.length(), GLSLExtMap.at(type));
					if (!fw.Open(glslFile.data()))
						return false;
					fw.Write(glslText.data(), glslText.size());
					fw.Close();

					String spvFile = glslFile + ".spv";
					String compileSpvCmd = StrFormat("glslangValidator -e main -o %s -V -D %s", spvFile.data(), glslFile.data());
					compiled = system(compileSpvCmd.data());
					if (compiled == 0) //success
					{
						fileName = GetFileName(spvFile).data();
						spvBlock[shaderPass] = subDir + fileName;
					}


					//String strGLSL;
					//if (HLSL_To_GLSL::Convert(GLSLTypeMap[type], processShaderText, strGLSL))
					//{
					//	String glslFile = outputDir + outName + shaderPasses[i] + "." + GLSLExtMap[type];
					//	String spvFile = glslFile + ".spv";

					//	FileWriter tmpFW;

					//	tmpFW.Open(glslFile.data());
					//	tmpFW.Write(strGLSL.data(), strGLSL.length());
					//	tmpFW.Close();

					//	String compileSpvCmd = StrFormat("glslc %s -D%s -o %s > %s", glslFile.data(), strPassMacro.data(), spvFile.data(), cmdFilename.data());
					//	compiled = system(compileSpvCmd.data());
					//	if (compiled == 0) //success
					//	{
					//		fileName = GetFileName(spvFile).data();
					//		spvBlock[type] = fileName;
					//	}
					//	else
					//	{
					//		if (cmdReader.Open(cmdFilename.data()))
					//		{
					//			String strCmd;
					//			cmdReader.ReadAllText(strCmd);
					//			*ErrorString += strCmd;
					//			cmdReader.Close();
					//		}
					//		*ErrorString += StrFormat("\n\n###SPV ERROR -- SHADER DUMP####\n%s", AddLineNumbers(strGLSL).data());
					//	}
					//}
				}
				else
				{
					if (cmdReader.Open(cmdFilename.data()))
					{
						String strCmd;
						cmdReader.ReadAllText(strCmd);
						*ErrorString += strCmd;
						cmdReader.Close();
					}
					*ErrorString += StrFormat("\n\n###HLSL ERROR -- SHADER DUMP####\n%s", AddLineNumbers(shaderText).data());
					return false;
				}

				HashSet<String>& currResources = (*iter).second.usedResources;
				shaderPassResources[(*iter).first].insert(currResources.begin(), currResources.end());
			}

			ConfigSection *pDXShaderSection = Config->GetSection("dx_shaders");
			pDXShaderSection->SetBlock(type.data(), d3dBlock);

			ConfigSection* pVKShaderSection = Config->GetSection("vk_shaders");
			pVKShaderSection->SetBlock(type.data(), spvBlock);
		}

		ConfigSection *pUniforms = Config->GetSection("shader_resources");
		StrMap<IShaderResource>* resMaps[]
		{
			&bufferMap,
			&textureMap,
			&samplerMap,
		};

		for (uint i = 0; i < 3; i++)
		{
			StrMap<IShaderResource>::iterator uniformIt = resMaps[i]->begin();
			while (uniformIt != resMaps[i]->end())
			{
				IShaderResource& data = (*uniformIt).second;
				if (data.stages != 0)
				{

					OrderedStrMap<String> block;
					block["type"] = IntToStr(data.type);
					block["binding"] = IntToStr(data.binding);
					block["stages"] = IntToStr(data.stages);
					block["flags"] = IntToStr(data.flags);
					block["dataString"] = data.dataStr;
					pUniforms->SetBlock(data.name.data(), block);
				}
				uniformIt++;
			}
		}

		ConfigSection *pElements = Config->GetSection("input_elements");
		StrMap<APIVertexElement>::iterator elemIt = inputElements.begin();
		while (elemIt != inputElements.end())
		{
			APIVertexElement& data = (*elemIt).second;

			OrderedStrMap<String> block;
			block["semantic"] = data.semantic;
			block["format"] = IntToStr(data.format);
			block["size"] = IntToStr(data.size);
			block["offset"] = IntToStr(data.offset);

			pElements->SetBlock(data.name.data(), block);
			++elemIt;
		}

		ConfigSection* pShaderPasses = Config->GetSection("shader_passes");
		StrMap<HashSet<String>>::iterator passIt = shaderPassResources.begin();
		while (passIt != shaderPassResources.end())
		{
			HashSet<String>& resSet = (*passIt).second;
			String resString;
			for (auto iter = resSet.begin(); iter != resSet.end(); ++iter)
			{
				resString += (*iter) + ";";
			}
			resString = resString.substr(0, resString.size() - 1);

			OrderedStrMap<String> block;
			block["resources"] = resString;
			pShaderPasses->SetBlock((*passIt).first.data(), block);
			++passIt;
		}

		String configName = shaderName + ".config";
		Config->Save(configName.data());

		return true;
	}
	else
	{
		return false;
	}
}

bool ProcessShader(const String& shaderFile, const ShaderOptions& options, StrMap<ShaderPassData>& outputShaderMap, StrMap<IShaderResource>& bufferResources, StrMap<IShaderResource>& textureResources, StrMap<IShaderResource>& samplerResources, String& errorStr)
{
	String baseShaderDir = GetDirectory(shaderFile) + "../";
	FileReader reader;

	String baseSrcText;
	if (!reader.Open(shaderFile.data()))
	{
		errorStr += StrFormat("Failed to open %s\n", shaderFile.data());
		return false;
	}
	reader.ReadAllText(baseSrcText);
	reader.Close();
	String ext = GetExtension(shaderFile);

	Vector<String> lines;
	StrSplit(baseSrcText, lines, '\n');

	StrMap<LinkedList<Pair<uint, String>>> shaderPassMap;

	Vector<Pair<uint, String>> sharedLines;
	for (uint i = 0; i < lines.size(); i++)
	{
		const String& line = lines[i];
		if (StrStartsWith(line, BEGIN_PASS_HINT))
		{
			String shaderPassName = StrTrimEnd(StrTrimStart(&line[sizeof(BEGIN_PASS_HINT)]));
			Pair<uint, String> lineNum_CodeBlock;
			lineNum_CodeBlock.first = i;

			uint lineEnd = i;
			while (!StrStartsWith(lines[++lineEnd], END_PASS_HINT))
			{
				lineNum_CodeBlock.second += lines[lineEnd] + "\n";
				lines[lineEnd] = "##REMOVED##";
			}

			shaderPassMap[shaderPassName].push_back(lineNum_CodeBlock);
			lines[i] = "##REMOVED##";
			lines[lineEnd] = "##REMOVED##";
		}

		if (line != "##REMOVED##")
		{
			sharedLines.push_back({ i , line + "\n" });
		}
	}

	if (shaderPassMap.size() == 0)
	{
		//insert default shader pass...
		shaderPassMap[SE_DEFAULT_SHADER_PASS];
	}

	for (auto iter = shaderPassMap.begin(); iter != shaderPassMap.end(); ++iter)
	{
		String shaderText;
		LinkedList<Pair<uint, String>>& passBlocks = (*iter).second;

		for (uint i = 0; i < sharedLines.size(); i++)
		{
			uint currLine = sharedLines[i].first;

			Vector<LinkedList<Pair<uint, String>>::iterator> removeList;
			for (auto blockIter = passBlocks.begin(); blockIter != passBlocks.end(); ++blockIter)
			{
				if (currLine > (*blockIter).first)
				{
					shaderText += (*blockIter).second;
					removeList.push_back(blockIter);
				}
			}

			for (auto rmIter : removeList)
				passBlocks.erase(rmIter);

			shaderText += sharedLines[i].second;
		}

		for (auto blockIter = passBlocks.begin(); blockIter != passBlocks.end(); ++blockIter)
		{
			shaderText += (*blockIter).second;
		}
		passBlocks.clear();

		//make sure to call this here so only Texture2D are populated
		Vector<String> tex2DVec = AppendRegisterToResource(shaderText, TEXTURE_2D_NAME, "t", ext, textureResources);
		Vector<String> texArrayVec = AppendRegisterToResource(shaderText, TEXTURE_2D_ARRAY_NAME, "t", ext, textureResources);
		Vector<String> texCubeVec = AppendRegisterToResource(shaderText, TEXTURE_CUBE_NAME, "t", ext, textureResources);

		uint numTransformableTextures = tex2DVec.size();
		if (!options.noTextureTransform && numTransformableTextures)
			shaderText = AppendTextureTransforms(shaderText, textureResources);

		Vector<String> samplerVec = AppendRegisterToResource(shaderText, SAMPLER_STATE_NAME, "s", ext, samplerResources);

		//Append include files...
		while (true)
		{
			usize includeStart = shaderText.find("#include", 0);
			if (includeStart == String::npos)
				break;

			usize fileStart = includeStart;
			while (shaderText[fileStart++] != '\"') {}
			usize fileEnd = fileStart + 1;
			while (shaderText[fileEnd++] != '\"') {}

			fileStart;
			fileEnd--;
			String includeFile = baseShaderDir + shaderText.substr(fileStart, fileEnd - fileStart);

			if (!reader.Open(includeFile.data()))
			{
				errorStr += StrFormat("Failed to open %s\n", includeFile.data());
				return false;
			}

			String includeText;
			reader.ReadAllText(includeText);
			reader.Close();
			includeText += "\r\n";
			shaderText = shaderText.replace(includeStart, (fileEnd - includeStart) + 1, includeText);
		}

		usize injectPsStructPos = shaderText.find(INJECT_PS_STRUCT_HINT);
		if (injectPsStructPos != String::npos)
		{
			usize structOffset = 0;
			while (true)
			{
				structOffset = shaderText.find("struct", structOffset);
				if (structOffset == String::npos)
					break;

				usize bracketPos = shaderText.find('{', structOffset);
				assert(bracketPos != String::npos);

				String structNameBlock = shaderText.substr(structOffset, bracketPos - structOffset);
				if (structNameBlock.find("PS_In") != String::npos)
				{
					bracketPos = shaderText.find('}', structOffset);

					String struckBlock = shaderText.substr(structOffset, (bracketPos - structOffset) + 1) + ";";

					shaderText = shaderText.replace(structOffset, struckBlock.size(), "");
					shaderText = shaderText.replace(shaderText.find(INJECT_PS_STRUCT_HINT), sizeof(INJECT_PS_STRUCT_HINT), struckBlock);
					break;
				}

				structOffset++;
			}
		}

		Vector<String> engineBuffersVec = AppendRegisterToResource(shaderText, CBUFFER_NAME, "b", ext, bufferResources);
		Vector<String> engineTexturesVec = AppendRegisterToResource(shaderText, TEXTURE_2D_NAME, "t", ext, textureResources);
		Vector<String> engineSamplersVec = AppendRegisterToResource(shaderText, SAMPLER_STATE_NAME, "s", ext, samplerResources);

		String outputText = PACK_MATRIX_STR;
		if (options.lighting) outputText += APPLY_LIGHTING_STR;
		if (options.fog) outputText += APPLY_FOG_STR;
		if (options.alphaTest) outputText += APPLY_ALPHA_TEST_STR;
		if (options.debugNormals) outputText += RENDER_NORMALS_STR;
		if (options.debugViewVector) outputText += RENDER_VIEW_VECTOR_STR;
		if (options.shadows) outputText += APPLY_SHADOWS_STR;
		outputText += "\r\n";

		for (uint i = 0; i < options.macros.size(); i++)
		{
			outputText += StrFormat("#define %s\r\n", options.macros[i].data());
		}

		outputText += StrFormat("%s\r\n\r\n", shaderText.data());

		ShaderPassData& passData = outputShaderMap[(*iter).first];
		passData.shaderText = outputText;
		
		Vector<String> shaderResources;
		shaderResources.insert(shaderResources.end(), tex2DVec.begin(), tex2DVec.end());
		shaderResources.insert(shaderResources.end(), texArrayVec.begin(), texArrayVec.end());
		shaderResources.insert(shaderResources.end(), texCubeVec.begin(), texCubeVec.end());
		shaderResources.insert(shaderResources.end(), samplerVec.begin(), samplerVec.end());
		shaderResources.insert(shaderResources.end(), engineBuffersVec.begin(), engineBuffersVec.end());
		shaderResources.insert(shaderResources.end(), engineTexturesVec.begin(), engineTexturesVec.end());
		shaderResources.insert(shaderResources.end(), engineSamplersVec.begin(), engineSamplersVec.end());
		for (uint i = 0; i < shaderResources.size(); i++)
		{
			passData.usedResources.insert(shaderResources[i]);
		}
	}
	return true;
}

bool ShaderContainsMappings(const String& shaderText, const String& mappingText)
{
	Vector<String> lines;
	StrSplit(mappingText, lines, '\n');
	for (uint i = 0; i < lines.size(); i++)
	{
		if (StrStartsWith(lines[i], MAPPING_TOKEN_NAME))
		{
			String mappingStr = StrRemove(lines[i].substr(sizeof(MAPPING_TOKEN_NAME)), '\r');
			
			Vector<String> mappingParts;
			StrSplit(mappingStr, mappingParts, ',');

			for (uint j = 0; j < mappingParts.size(); j++)
			{
				if (shaderText.find(mappingParts[j]) != String::npos)
					return true;
			}

			return false;;
		}
	}

	return false;
}

Vector<String> AppendRegisterToResource(String& shaderText, const String& resourceType, const String& registerType, const String& shaderExt, StrMap<IShaderResource> &bindings)
{
	Vector<String> resNameVec;
	uint offset = 0;
	while (true)
	{
		usize resPos = shaderText.find(resourceType, offset);
		if (resPos == String::npos)
			break;

		char nextResToken = shaderText[resPos + resourceType.size()];
		if (CharAlphaNumeric(nextResToken))
		{
			offset = resPos + resourceType.length();
			continue;
		}

		bool isDeclaration = false;
		if (resourceType == CBUFFER_NAME)
		{
			isDeclaration = true;
		}
		else
		{
			String followingTokens[3];
			uint tokenCount = 0;
			uint charIter = resPos + resourceType.size();
			while (tokenCount != 3)
			{
				if (CharAlphaNumeric(shaderText[charIter]))
				{
					followingTokens[tokenCount] = shaderText[charIter];
					charIter++;
					while (CharAlphaNumeric(shaderText[charIter]))
					{
						char c = shaderText[charIter];
						followingTokens[tokenCount] += c;
						charIter++;
					}
					tokenCount++;

					//found end paren within first three following tokens, this is declaration
					char c = shaderText[charIter];
					if (c == ';')
					{
						isDeclaration=true;
						break;
					}
				}
				else
				{
					charIter++;
				}
			}
		}

		if (isDeclaration)
		{
			usize resNameStart = resPos + resourceType.size();
			usize resNameEnd = shaderText.find(';', resPos);

			String resName = shaderText.substr(resNameStart, resNameEnd - resNameStart);
			if (resName.find("{") != String::npos)
			{
				resNameEnd = shaderText.find("{", resPos);
				resName = shaderText.substr(resNameStart, resNameEnd - resNameStart);
			}

			resName = StrTrimEnd(StrTrimStart(resName));
			
			bool bIsNew = bindings.find(resName) == bindings.end();
			IShaderResource& res = bindings[resName];

			//just keep overriding these, could be better way, but dont know for now
			if (bIsNew)
			{
				res.binding = bindings.size() - 1;
				res.name = resName;
			}

			if (registerType == "b")
				res.type = SRT_CONST_BUFFER;
			else if (registerType == "t")
				res.type = SRT_TEXTURE;
			else if (registerType == "s")
				res.type = SRT_SAMPLER;

			if (res.type == SRT_CONST_BUFFER)
			{
				usize blockStart = shaderText.find('{', resNameEnd);
				usize blockEnd = shaderText.find('}', blockStart);
				res.dataStr = StrReplace(StrRemove(StrTrimEnd(StrTrimStart(shaderText.substr(blockStart + 1, blockEnd - blockStart - 1))), '\r'), CodeReplace, ' ');
			} 
			else if (res.type == SRT_TEXTURE)
			{
				res.dataStr = resourceType;

				//get the actual type of texture...
				if (res.dataStr == TEXTURE_2D_NAME)
				{
					res.flags |= SRF_TEXTURE_2D;
				}
				if (res.dataStr == TEXTURE_CUBE_NAME)
				{
					res.flags |= SRF_TEXTURE_CUBE;
				}	
				if (res.dataStr == TEXTURE_2D_ARRAY_NAME)
				{
					res.flags |= SRF_TEXTURE_ARRAY;
				}
			}

			if (shaderExt == "vs")
				res.stages |= SS_VERTEX;
			if (shaderExt == "ps")
				res.stages |= SS_PIXEL;
			if (shaderExt == "gs")
				res.stages |= SS_GEOMETRY;

			String replaceStr = shaderText.substr(resPos, resNameEnd - resPos) + StrFormat(" : register(%s%d)", registerType.data(), res.binding);
			shaderText = shaderText.replace(resPos, resNameEnd - resPos, replaceStr);
			offset = resPos + replaceStr.size();
			resNameVec.push_back(resName);
		}
		else
		{
			offset = resPos + resourceType.length();
		}
	}

	return resNameVec;
}

bool TokenIsShaderResouce(const String & token)
{
	if (token.length() > 1)
	{
		char type = token[0];
		if (type == 'b' || type == 't' || type == 's')
		{
			for (uint i = 1; i < token.length(); i++)
				if (token[i] < '0' || token[i] > '9')
					return false;

			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void ParseShader(const String & code, const String &shaderTypeStr, StrMap< APIVertexElement>& inputElements)
{
	Vector<String> tokens;
	Tokenize(code, tokens);

	uint inputElemMemberOffset = 0;

	for (uint i = 0; i < tokens.size(); i++)
	{
		String &token = tokens[i];
		if (shaderTypeStr == "vs")
		{
			if (token.find("VS_In") != String::npos && tokens[i - 1] == "struct")
			{
				uint structDataCounter = ++i;
				String vsStructStr;
				do
				{
					vsStructStr += tokens[structDataCounter] + " ";
				} while (tokens[structDataCounter++].find('}') == String::npos);
			
				String removeBrackets = StrRemove(StrRemove(vsStructStr, '{'), '}');

				Vector<String> inputParts;
				StrSplit(removeBrackets, inputParts, ';');

				for (uint j = 0; j < inputParts.size(); j++)
				{
					String vertexMember = StrTrimStart(StrTrimEnd(inputParts[j]));
					Vector<String> memberParts;
					StrSplit(vertexMember, memberParts, ' ');

					String dataType = memberParts[0];
					String dataName = memberParts[1];
					String dataSemantic = memberParts[3];

					APIVertexElement elem;
					elem.semantic = dataSemantic;
					elem.name = dataName;

					if (dataType == "float2")
					{
						elem.format = VIF_FLOAT2;
						elem.size = sizeof(float) * 2;
					}
					else if (dataType == "float3")
					{
						elem.format = VIF_FLOAT3;
						elem.size = sizeof(float) * 3;
					}
					else if (dataType == "float4")
					{
						elem.format = VIF_FLOAT4;
						elem.size = sizeof(float) * 4;
					}
					else
					{
						//TODO add support for in structs?
						assert(false);
					}

					elem.offset = inputElemMemberOffset;
					inputElemMemberOffset += elem.size;

					inputElements[dataName] = elem;
				}
			}
		}
	}
}

String AppendTextureTransforms(const String &code, const StrMap<IShaderResource>& bindingMap)
{
	String newCode;

	char texBuffer[512];
	texBuffer[511] = '\0';

	String sampleHint = ".Sample(";
	usize sampleOffset = code.find(sampleHint);
	uint substrStart = 0;
	uint binding;
	while (sampleOffset != String::npos)
	{
		int texNameCounter = sampleOffset - 1;
		int texBufferCounter = sizeof(texBuffer) - 2;

		bool bFound = false;
		while (texNameCounter >= 0 && texBufferCounter >= 0)
		{
			texBuffer[texBufferCounter] = code[texNameCounter];
			char* texName = &texBuffer[texBufferCounter];
			StrMap<IShaderResource>::const_iterator iter = bindingMap.find(texName);
			if (iter != bindingMap.end())
			{
				binding = (*iter).second.binding;
				bFound = true;
				break;
			}

			texNameCounter--;
			texBufferCounter--;
		}

		String texTransformAppend = "";
		if (bFound)
		{
			int parenStack = 1;
			int sampleEnd = sampleOffset + sampleHint.size();
			while (parenStack)
			{
				if (code[sampleEnd] == '(')
					parenStack++;
				if (code[sampleEnd] == ')')
					parenStack--;

				sampleEnd++;
			}

			sampleOffset = sampleEnd - 1;
			texTransformAppend = StrFormat(" * TextureTransforms[%d].xy + TextureTransforms[%d].zw", binding, binding);

			newCode += code.substr(substrStart, (sampleOffset - substrStart));
			newCode += texTransformAppend;
			substrStart = sampleOffset;
		}
		else
		{
			sampleOffset += sampleHint.size();
		}

		sampleOffset = code.find(sampleHint, sampleOffset);
	}

	newCode += code.substr(substrStart);
	return newCode;
}

void Tokenize(const String & code, Vector<String> &parts)
{
	String tokenCode;
	for (uint i = 0; i < code.length(); i++)
	{
		char token = code[i];

		for (uint j = 0; j < CodeReplace.size(); j++)
		{
			if (code[i] == CodeReplace[j])
			{
				token = ' ';
				break;
			}
		}

		bool isToken = false;
		for (uint j = 0; j < CodeSeperators.size(); j++)
		{
			if (code[i] == CodeSeperators[j])
			{
				tokenCode += ' ';
				isToken = true;
				break;
			}
		}

		tokenCode += token;
		if (isToken) tokenCode += ' ';
	}

	StrSplit(tokenCode, parts, ' ');
}

String AddLineNumbers(const String& str)
{
	String output;
	Vector<String> lines;
	StrSplit(str, lines, '\n');
	for (uint i = 0; i < lines.size(); i++)
	{
		output += StrFormat("%d\t%s\n", i + 1, lines[i].data());
	}
	return output;
}

}
}