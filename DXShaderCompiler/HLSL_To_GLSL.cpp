#include <stdio.h>
#include "Types.h"
#include "FileReader.h"
#include "FileWriter.h"
#include "StringUtil.h"

#include "HLSL_To_GLSL.h"

#define UBO_SET 0
#define TEXTURE_SET 1
#define SAMPLER_SET 2

using namespace SunEngine;

namespace HLSL_To_GLSL
{

enum EShaderPrimitive
{
	SP_UNDEFINED,
	SP_POINT,
	SP_TRIANGLE,
};

const char* GLSL_ExtTable[]
{
	"vert",
	"frag",
	"geom"
};

struct HLSL_Var
{
	HLSL_Var()
	{
	}

	String name;
	String type;
	String semantic;
};

struct HLSL_Struct
{
	String name;
	Vector<HLSL_Var> variables;
};

struct HLSL_GeomShaderInfo
{
	HLSL_GeomShaderInfo()
	{
		maxVertexCount = 0;
		inPrimitive = SP_UNDEFINED;
		outPrimitive = SP_UNDEFINED;
	}

	const char* GetInPrimStr() const
	{
		switch (inPrimitive)
		{
		case SP_POINT:
			return "points";
		case SP_TRIANGLE:
			return "triangles";
		case SP_UNDEFINED:
		default:
			return "";
		}
	}

	const char* GetOutPrimStr() const
	{
		switch (outPrimitive)
		{
		case SP_POINT:
			return "points";
		case SP_TRIANGLE:
			return "triangle_strip";
		case SP_UNDEFINED:
		default:
			return "";
		}
	}

	uint maxVertexCount;
	EShaderPrimitive inPrimitive;
	EShaderPrimitive outPrimitive;
	String outStreamName;
	String outStructName;
};

bool CompareVar(const HLSL_Var &var, const void *userData)
{
	String *pStr = (String*)userData;
	return *pStr == var.name;
}

bool CompareStruct(const HLSL_Struct &hStruct, const void *userData)
{
	String *pStr = (String*)userData;
	return *pStr == hStruct.name;
}

//TODO add more as they become noted
Vector<char> CodeTokens =
{
	'(',
	')',
	';',
	':',
	',',
	'.',
	'[',
	']',
};

StrMap<String> TypeConvTable;
StrMap<String> FuncConvTable;
Vector<HLSL_Struct> Structs;
HLSL_GeomShaderInfo GeomInfo;

EShaderType ShaderType;

void Tokenize(const String &code, Vector<String> &tokens);
String ParseCBuffer(String* pTokens, uint count, uint& tokensAdvanced);
String ParseStruct(String* pTokens, uint count, uint& tokensAdvanced);
String ParseTexture(String* pTokens, uint count, uint& tokensAdvanced);
String ParseSampler(String* pTokens, uint count, uint& tokensAdvanced);
String ParseFunction(String* pTokens, uint count, uint& tokensAdvanced);
String ParseStatic(String* pTokens, uint count, uint& tokensAdvanced);
String GetSampleString(String* pTokens, uint count, uint& tokensAdvanced);

String ParseMacro(const String &macro);
String RegisterToUnit(const String &reg);
String ToGLSLType(const String &type);

StrMap<String> TextureTypeMap;
StrMap<String> SamplerMap;

void CreateMaps();
bool TokenIsDataType(const String &token);

bool Convert(EShaderType type, const String& source, String& convertedSource)
{
	ShaderType = type;

	CreateMaps();

	Vector<String> lines;
	StrSplit(source, lines, '\n');

	Vector<String> macros;

	String code;
	for (uint i = 0; i < lines.size(); i++)
	{
		String line = lines[i];

		if (line.find("#define") != String::npos)
		{
			macros.push_back(line);
		}
		else
		{
			if (StrStartsWith(StrTrimStart(line), "//") == false)
			{
				code += line;
			}	
			else
			{
				int m = 5;
				m++;
			}
		}
	}

	Vector<String> codeParts;
	Tokenize(code, codeParts);

	String shaderCode;

	for (uint i = 0; i < codeParts.size(); )
	{

		String token = codeParts[i];
		String glsl;

		uint tokenCount = codeParts.size() - i;
		uint tokensAdvanced = 0;

		if (token == "cbuffer")
		{
			glsl = ParseCBuffer(&codeParts[i], tokenCount, tokensAdvanced);
		}

		else if (token == "struct")
		{
			glsl = ParseStruct(&codeParts[i], tokenCount, tokensAdvanced);
		}

		else if (token == "Texture2D")
		{
			glsl = ParseTexture(&codeParts[i], tokenCount, tokensAdvanced);
		}

		else if (token == "Texture2DArray")
		{
			glsl = ParseTexture(&codeParts[i], tokenCount, tokensAdvanced);
		}

		else if (token == "TextureCube")
		{
			glsl = ParseTexture(&codeParts[i], tokenCount, tokensAdvanced);
		}

		else if (token == "SamplerState")
		{
			glsl = ParseSampler(&codeParts[i], tokenCount, tokensAdvanced);
		}

		else if (token == "static")
		{
			glsl = ParseStatic(&codeParts[i], tokenCount, tokensAdvanced);
		}

		//if(token == "#")

		//a function is defined as having either a void return type or a valid data return type, followed by a open open parenthesis
		else if ((token == "void" || (TokenIsDataType(token)) && codeParts[i + 2] == "("))
		{
			glsl = ParseFunction(&codeParts[i], tokenCount, tokensAdvanced);
		}
			
		else if (token == "maxvertexcount")
		{
			GeomInfo.maxVertexCount = StrToInt(codeParts[i + 2]);
			i += 3;
		}

		else
		{
			i++;
		}

		i += tokensAdvanced;

		if (glsl.size())
		{
			shaderCode += glsl;
			shaderCode += "\n";
		}
	}

			

	convertedSource = "#version 460\n\n";

	//TODO: need to check macro for hlsl-to-glsl specific things?
	for (uint i = 0; i < macros.size(); i++)
	{
		convertedSource += ParseMacro(macros[i]);
		convertedSource += "\n";
	}

	convertedSource += "mat4 mul(mat4 lhs, mat4 rhs) { return lhs * rhs; }\n";
	convertedSource += "mat3 mul(mat3 lhs, mat3 rhs) { return lhs * rhs; }\n";
	convertedSource += "mat2 mul(mat2 lhs, mat2 rhs) { return lhs * rhs; }\n";
	convertedSource += "vec4 mul(vec4 lhs, mat4 rhs) { return rhs * lhs; }\n";
	convertedSource += "vec3 mul(vec3 lhs, mat3 rhs) { return rhs * lhs; }\n";
	//outShader += "gvec4 texture(gsampler2D sampler, vec2 uv, ivec2 offset) { return  vec4(0.0f); }";

	convertedSource += "\n";

	if (ShaderType == ST_GEOM)
	{
		convertedSource += StrFormat("layout(%s) in;\n", GeomInfo.GetInPrimStr());
		convertedSource += StrFormat("layout(%s, max_vertices=%d) out;\n", GeomInfo.GetOutPrimStr(), GeomInfo.maxVertexCount);
		convertedSource += "\n";
	}

	convertedSource += shaderCode;
	return true;
}

void Tokenize(const String &code, Vector<String> &tokens)
{
	Vector<char> replaceChars =
	{
		'\t', '\r', '\n',
		//'n' has been removed in macro parsing...
	};

	String tokenCode;
	tokenCode = StrReplace(code, replaceChars, ' ');

	//max size possible for safety....
	char * buff = new char[tokenCode.size() * 3];

	uint buffCount = 0;
	for (uint i = 0; i < tokenCode.size(); i++)
	{
		if (Contains(CodeTokens, tokenCode[i]))
		{
			buff[buffCount++] = ' ';
			buff[buffCount++] = tokenCode[i];
			buff[buffCount++] = ' ';
		}
		else
		{
			buff[buffCount++] = tokenCode[i];
		}
	}

	buff[buffCount] = '\0';
	tokenCode = buff;
	delete[] buff;

	StrSplit(tokenCode, tokens, ' ');
}

String ParseCBuffer(String* pTokens, uint count, uint& tokensAdvanced)
{
	uint i = 0;

	String buffName;
	String unit;;

	Vector<String> variables;

	while (pTokens[i] != "}")
	{
		if (pTokens[i] == "cbuffer")
		{
			buffName = pTokens[i + 1];
		}

		if (pTokens[i] == "register")
		{
			unit = RegisterToUnit(pTokens[i + 2].data());
		}

		if (pTokens[i] == ";")
		{
			if (pTokens[i - 1] != "]")
			{
				String varName = pTokens[i - 1];
				String varType = ToGLSLType(pTokens[i - 2]);

				variables.push_back(varType);
				variables.push_back(varName);
			}
			else
			{
				String varName = pTokens[i - 4];
				String varType = ToGLSLType(pTokens[i - 5]);
				varType += pTokens[i - 3];
				varType += pTokens[i - 2];
				varType += pTokens[i - 1];

				variables.push_back(varType);
				variables.push_back(varName);
			}
		}

		i++;
	}

	tokensAdvanced = i;

	if (!unit.size())
	{
		return "";
	}

	String uboStr;
	uboStr += StrFormat("layout (std140, set = %d", UBO_SET);
	if (unit.size())
	{
		uboStr += ", binding = ";
		uboStr += unit;
	}
	uboStr += ") uniform ";
	uboStr += buffName;
	uboStr += "\n{\n";
	for (i = 0; i < variables.size(); i += 2)
	{
		uboStr += "\t";
		uboStr += variables[i + 0];
		uboStr += " ";
		uboStr += variables[i + 1];
		uboStr += ";\n";
	}

	uboStr += "};\n";

	return uboStr;
}

String ParseStruct(String* pTokens, uint count, uint& tokensAdvanced)
{

	HLSL_Struct hStruct;
	hStruct.name = pTokens[1];

	uint i = 0;
	while (pTokens[i] != "}")
	{
		if (pTokens[i] == ";")
		{
			HLSL_Var sv;

			if (pTokens[i - 2] == ":")
			{
				sv.semantic = pTokens[i - 1];
				sv.type = pTokens[i - 4];
				sv.name = pTokens[i - 3];
			}
			else
			{
				if (pTokens[i - 1] != "]")
				{
					sv.type = pTokens[i - 2];
					sv.name = pTokens[i - 1];
				}
				else
				{
					sv.type = pTokens[i - 5];
					sv.name = pTokens[i - 4];
					sv.name += pTokens[i - 3];
					sv.name += pTokens[i - 2];
					sv.name += pTokens[i - 1];
				}
			}

			sv.type = ToGLSLType(sv.type);
			hStruct.variables.push_back(sv);
		}

		i++;
	}

	tokensAdvanced = i;

	String structStr = "struct ";
	structStr += hStruct.name;
	structStr += "\n{\n";

	for (i = 0; i < hStruct.variables.size(); i++)
	{
		HLSL_Var &var = hStruct.variables[i];

		structStr += "\t";
		structStr += var.type;
		structStr += " ";
		structStr += var.name;
		structStr += ";\n";
	}

	structStr += "};\n";
	Structs.push_back(hStruct);
	return structStr;
}

String ParseTexture(String* pTokens, uint count, uint& tokensAdvanced)
{
	String texType;
	String texName;
	String unit;

	String arrayBrackets;

	uint i = 0;
	while (pTokens[i] != ";")
	{
		if (pTokens[i] == "Texture2D")
		{
			texType = "texture2D";
			texName = pTokens[i + 1];
		}
		if (pTokens[i] == "Texture2DArray")
		{
			texType = "texture2DArray";
			texName = pTokens[i + 1];
		}
		if (pTokens[i] == "TextureCube")
		{
			texType = "textureCube";
			texName = pTokens[i + 1];
		}

		if (pTokens[i] == "[")
		{
			arrayBrackets += pTokens[i];
			arrayBrackets += pTokens[i + 1];
			if(pTokens[i + 2] == "]")
				arrayBrackets += pTokens[i + 2];
		}

		if (pTokens[i] == "register")
		{
			unit = RegisterToUnit(pTokens[i + 2]);
		}

		i++;
	}

	tokensAdvanced = i;

	String texStr;
	texStr += StrFormat("layout(set = %d", TEXTURE_SET);
	if (unit.size())
	{
		texStr += ",binding = ";
		texStr += unit;
	}
	texStr += ") ";
	texStr += "uniform ";
	texStr += texType;
	texStr += " ";
	texStr += texName;
	texStr += arrayBrackets;
	texStr += ";";

	TextureTypeMap[texName] = texType;

	return texStr;
}

String ParseSampler(String * pTokens, uint count, uint & tokensAdvanced)
{
	String samplerName;
	String unit;

	String arrayBrackets;

	uint i = 0;
	while (pTokens[i] != ";")
	{
		if (pTokens[i] == "SamplerState")
		{
			samplerName = pTokens[i + 1];
		}

		if (pTokens[i] == "[")
		{
			arrayBrackets += pTokens[i];
			arrayBrackets += pTokens[i + 1];
			if (pTokens[i + 2] == "]")
				arrayBrackets += pTokens[i + 2];
		}

		if (pTokens[i] == "register")
		{
			unit = RegisterToUnit(pTokens[i + 2]);
		}

		i++;
	}

	tokensAdvanced = i;

	String samplerStr;
	samplerStr += StrFormat("layout(set = %d", SAMPLER_SET);
	if (unit.size())
	{
		samplerStr += ",binding = ";
		samplerStr += unit;
	}
	samplerStr += ") ";
	samplerStr += "uniform sampler ";
	samplerStr += samplerName;
	samplerStr += arrayBrackets;
	samplerStr += ";";

	//TextureTypeMap[samplerName] = texType;

	return samplerStr;
}

String ParseFunction(String* pTokens, uint count, uint& tokensAdvanced)
{
	String retType = pTokens[0];
	retType = ToGLSLType(retType);
	String funcName = pTokens[1];

	String geomShaderInLayout;
	String geomShaderOutLayout;

	Vector<HLSL_Var> arguments;

	uint i = 2;
	Vector<String> argumentTokens;
	while (true)
	{
		if (pTokens[i] == "," || (pTokens[i] == ")" && pTokens[i - 1] != "("))
		{
			HLSL_Var var;

			if (ShaderType == ST_VERT || ShaderType == ST_FRAG)
			{
				//argument has a semantic
				if (pTokens[i - 2] == ":")
				{
					var.semantic = pTokens[i - 1];
					var.type = ToGLSLType(pTokens[i - 4]);
					var.name = pTokens[i - 3];
				}
				else
				{
					var.type = ToGLSLType(pTokens[i - 2]);
					var.name = pTokens[i - 1];
				}
			}
			else if (ShaderType == ST_GEOM)
			{
				if (argumentTokens.front() == "point" || argumentTokens.front() == "triangle")
				{
					var.type = ToGLSLType(argumentTokens[1]);
					var.name = argumentTokens[2];
				
					const String &strPrim = argumentTokens.front();
					if (strPrim == "point")
						GeomInfo.inPrimitive = SP_POINT;
					else if (strPrim == "triangle")
						GeomInfo.inPrimitive = SP_TRIANGLE;
				}
				else if (argumentTokens.front() == "inout")
				{
					if (StrContains(argumentTokens[1], "PointStream"))
					{
						GeomInfo.outPrimitive = SP_POINT;
					}
					else if (StrContains(argumentTokens[1], "TriangleStream"))
					{
						GeomInfo.outPrimitive = SP_TRIANGLE;
					}			

					String argStr = VecToStr(argumentTokens, ' ');
					uint typeStart = argStr.find('<') + 1;
					uint typeEnd = argStr.find('>');
					GeomInfo.outStructName = StrRemove(argStr.substr(typeStart, typeEnd - typeStart), ' ');
					retType = GeomInfo.outStructName;
					GeomInfo.outStreamName = argumentTokens.back();
				}
			}

			if (var.name.size())
			{
				arguments.push_back(var);
			}
			argumentTokens.clear();
		}
		else
		{
			if(pTokens[i] != "(")
				argumentTokens.push_back(pTokens[i]);
		}

		if (pTokens[i] == ")")
		{
			break;
		}

		i++;
	}

	String retSemantic;

	//this function returns a built in semantic
	if (pTokens[i + 1] == ":")
	{
		retSemantic = StrToLower(pTokens[i + 2]);
		i += 3;
	}
	else
	{
		i += 1;
	}

	String outputStr;
	bool isFunctionDeclaration = pTokens[i] == ";";

	Vector<String> funcBodyTokens;
	if (!isFunctionDeclaration)
	{
		String glPositionStr;
		HLSL_Struct *pRetStruct = Find(Structs, &retType, CompareStruct);
		if (pRetStruct)
		{
			if (pRetStruct->variables[0].semantic.size())
			{
				//handle multi render targets
				//if (StrToLower(pRetStruct->variables[0].semantic) == "sv_target0")
				{
					outputStr += "layout(location = 0) ";
				}

				outputStr += "out ";
				outputStr += pRetStruct->name;
				outputStr += " ";
			}
		}

		uint bracketStack = 0;

		do
		{
			String token = pTokens[i];
			String appendStr = pTokens[i];

			if (token == "{")
			{
				appendStr += "\n";
				bracketStack++;
			}

			if (token == "}")
			{
				appendStr = "\n}\n";
				bracketStack--;
			}

			if (pRetStruct && token == pRetStruct->name)
			{
				for (uint j = 0; j < pRetStruct->variables.size(); j++)
				{
					if (StrToLower(pRetStruct->variables[j].semantic) == "sv_position")
					{
						glPositionStr = "gl_Position = ";
						glPositionStr += pTokens[i + 1];
						glPositionStr += ".";
						glPositionStr += pRetStruct->variables[j].name;
						glPositionStr += ";\n";
						break;
					}
				}

				//TODO pass in entry point?
				if (funcName == "main")
				{
					appendStr = "";
					pTokens[i + 1] = "";
					pTokens[i + 2] = "";
				}
			}

			if (token == "return")
			{
				if (retSemantic.size())
				{
					if (retSemantic == "sv_position")
					{
						appendStr = "gl_Position=";
					}
					if (retSemantic == "sv_target")
					{
						appendStr = "fragColor=";
						outputStr = "layout(location = 0) out vec4 fragColor;\n";
					}
				}
				else
				{
					if (outputStr.size())
					{
						outputStr += pTokens[i + 1];
						outputStr += ";\n";
						appendStr = "";
						pTokens[i + 1] = "";
						pTokens[i + 2] = "";
					}
				}
			}

			if (TextureTypeMap.find(token) != TextureTypeMap.end())
			{
				uint sampleOffset = 0;
				appendStr = GetSampleString(&pTokens[i], count - i, sampleOffset);
				i += sampleOffset;
			}

			if (token == "[")
			{
				if (pTokens[i + 1] == "unroll")
				{
					appendStr = "";
					pTokens[i + 1] = "";
					pTokens[i + 2] = "";
				}
			}

			if (token == ";")
			{
				appendStr += "\n";
			}

			if (StrStartsWith(token, "#if"))
			{
				appendStr += " ";
				appendStr += pTokens[i + 1];
				pTokens[i + 1] = "";
				appendStr += "\n";
			}

			if (token == "#else")
			{
				appendStr += "\n";
			}

			if (token == "#endif")
			{
				appendStr += "\n";
			}

			if (ShaderType == ST_GEOM)
			{
				if (token == GeomInfo.outStreamName)
				{
					if (pTokens[i + 2] == "Append")
					{
						appendStr = glPositionStr;
						appendStr += "EmitVertex()";
						outputStr += pTokens[i + 4];
						outputStr += ";\n";

						uint n = i;
						do
						{
							pTokens[n++] = "";
						} while (pTokens[n] != ";");
					}
					else if (pTokens[i + 2] == "RestartStript")
					{

					}
				}
			}

			StrMap<String>::iterator strIter = TypeConvTable.find(appendStr);
			if(strIter != TypeConvTable.end())
			{
				appendStr = (*strIter).second;
			}

			strIter = FuncConvTable.find(appendStr);
			if (strIter != FuncConvTable.end())
			{
				appendStr = (*strIter).second;
			}

			if (ShaderType == ST_VERT)
			{
				if (bracketStack == 0 && glPositionStr.size())
				{
					funcBodyTokens.push_back(glPositionStr);
				}
			}
			funcBodyTokens.push_back(appendStr);

			i++;
		} while (bracketStack != 0);
	}

	tokensAdvanced = i;

	if (retSemantic.size() || outputStr.size())
	{
		retType = "void";
	}

	String funcDef;

	funcDef += retType;
	funcDef += " ";
	funcDef += funcName;
	funcDef += "(";

	Vector<HLSL_Var> inputVars;
	HLSL_Var *pInputStructVar = 0;

	for (i = 0; i < arguments.size(); i++)
	{
		HLSL_Var &var = arguments[i];

		if (var.semantic.size() == 0)
		{
			HLSL_Struct *pStruct = Find(Structs, &var.type, CompareStruct);
			if (pStruct)
			{
				//TODO pass in entry point?
				if (pStruct->variables[0].semantic.size() && funcName == "main")
				{
					pInputStructVar = &var;
					continue;
				}
			}

			funcDef += var.type;
			funcDef += " ";
			funcDef += var.name;

			if (i + 1 != arguments.size())
			{
				funcDef += ", ";
			}
		}
		else
		{
			inputVars.push_back(var);
		}
	}

	String funcStr;

	if (!isFunctionDeclaration)
	{
		funcDef += ")\n";

		StrMap<String> replaceMap;

		String inputsStr;
		uint validInputs = 0;
		for (i = 0; i < inputVars.size(); i++)
		{
			if (StrToLower(inputVars[i].semantic) == "sv_vertexid")
			{
				replaceMap[inputVars[i].name] =  "gl_VertexIndex";
			}
			else
			{
				if (!pInputStructVar)
				{
					String inStr = "layout(location = ";
					inStr += IntToStr(validInputs++);
					inStr += ") in ";
					inStr += inputVars[i].type;
					inStr += " ";
					inStr += inputVars[i].name;
					inStr += ";\n";

					inputsStr += inStr;
				}
			}
		}

		if (pInputStructVar)
		{
			if (ShaderType == ST_VERT)
			{
				HLSL_Struct *structData = Find(Structs, &pInputStructVar->type, CompareStruct);
				for (i = 0; i < structData->variables.size(); i++)
				{
					inputsStr += StrFormat("layout(location = %d) in ", i);
					inputsStr += structData->variables[i].type;
					inputsStr += " ";
					inputsStr += pInputStructVar->name;
					inputsStr += "_";
					inputsStr += structData->variables[i].name;
					inputsStr += ";\n";
				}
			}
			else
			{
				inputsStr += "layout(location = 0) in ";
				inputsStr += pInputStructVar->type;
				inputsStr += " ";
				inputsStr += pInputStructVar->name;

				if (ShaderType == ST_GEOM)
				{
					inputsStr += "[]";
				}

				inputsStr += ";\n";
			}
		}

		String funcBody;
		for (i = 0; i < funcBodyTokens.size(); i++)
		{
			String funcToken = funcBodyTokens[i];

			StrMap<String>::iterator strIter = replaceMap.find(funcToken);
			if (strIter != replaceMap.end())
			{
				funcToken = (*strIter).second;
			}

			if (ShaderType == ST_VERT && pInputStructVar && funcToken == pInputStructVar->name)
			{
				funcToken += "_";
				funcToken += funcBodyTokens[i + 2];
				funcBodyTokens[i + 1] = "";
				funcBodyTokens[i + 2] = "";
			}

			funcBody += funcToken;

			int nearToken = 0;

			if (i < funcBodyTokens.size() - 1 && Contains(CodeTokens, funcBodyTokens[i + 1][0]))
			{
				nearToken++;
			}
			//if (i > 0 && CodeTokens.Contains(funcBodyTokens[i - 1][0]))
			//{
			//	nearToken++;
			//}

			if (Contains(CodeTokens, funcToken[0]))
			{
				nearToken++;
			}

			if (nearToken == 0 && funcToken.size())
			{
				funcBody += " ";
			}

		}

		funcStr = inputsStr + outputStr + funcDef + funcBody;
	}
	else
	{
		funcStr = funcDef + ");\n";
	}

	return funcStr;
}

String ParseStatic(String* pTokens, uint count, uint& tokensAdvanced)
{
	String dataType = pTokens[1];

	dataType = ToGLSLType(dataType);

	String arrayStr;
	bool isArray = false;
	if (pTokens[3] == "[")
	{
		arrayStr += dataType;
		uint arrDef = 3;
		while (pTokens[arrDef] != "]")
		{
			arrayStr += pTokens[arrDef];
			arrDef++;
		}
		arrayStr += "]";
		isArray = true;
	}

	String staticStr = dataType + " ";

	uint i = 2;
	while (pTokens[i] != ";")
	{
		String token = pTokens[i];
		token = ToGLSLType(token);

		if (isArray && token == "{")
		{
			token = "(";
		}

		if (isArray && token == "}")
		{
			token = ")";
		}

		staticStr += token;
		//staticStr += " ";

		if (isArray && token == "=")
		{
			staticStr += arrayStr;
			staticStr += " ";
		}

		i++;
	}
	staticStr += ";";

	tokensAdvanced += i;
	return staticStr;
}

String ParseMacro(const String & macro)
{
	bool hasArguments = false;
	Vector<String> spaceParts;
	StrSplit(macro, spaceParts, ' ');
	if (spaceParts[1].find('(') != String::npos)
	{
		hasArguments = true;
	}

	Vector<String> parts;
	Tokenize(macro, parts);

	Vector<String> macroParts;
	for (uint i = 0; i < parts.size(); i++)
	{
		String token = parts[i];
		token = ToGLSLType(token);

		if(TextureTypeMap.find(parts[i]) != TextureTypeMap.end())
		{		
			uint tokensAdvanced = 0;
			token = GetSampleString(&parts[i], parts.size() - i, tokensAdvanced);
			i += tokensAdvanced;
		}

		macroParts.push_back(token);
	}

	String outMacro;

	outMacro += macroParts[0];
	outMacro += " ";
	outMacro += macroParts[1];
	if (!hasArguments)
		outMacro += " ";
	for (uint i = 2; i < macroParts.size(); i++)
	{
		outMacro += macroParts[i];
	
		int nearToken = 0;

		if (i < macroParts.size() - 1 && Contains(CodeTokens, macroParts[i + 1][0]))
		{
			nearToken++;
		}
		//if (i > 0 && CodeTokens.Contains(funcBodyTokens[i - 1][0]))
		//{
		//	nearToken++;
		//}

		if (Contains(CodeTokens, macroParts[i][0]))
		{
			nearToken++;
		}

		if (nearToken == 0 && macroParts[i].size())
		{
			outMacro += " ";
		}
	}

	return outMacro;
}

String GetSampleString(String* pTokens, uint count, uint& tokensAdvanced)
{
	uint i = 0;

	String texName = pTokens[i];

	if (pTokens[i + 1] == "[")
	{
		pTokens[i] = "";
		texName += pTokens[++i];
		pTokens[i] = "";
		texName += pTokens[++i];
		pTokens[i] = "";
		if (pTokens[i+1] == "]")
		{
			texName += pTokens[++i];
			pTokens[i] = "";
		}
	}
	
	uint paramCount = 1;
	uint idx = i;
	uint scope = 0;

	String samplerName;

	//BART note: what is this doing..?
	while (pTokens[idx] != ")")
	{
		if (pTokens[idx] == "(")
			scope++;
		if (pTokens[idx] == ")")
			scope--;

		if (pTokens[idx] == "Sample")
			samplerName = pTokens[idx + 2];

		if (pTokens[idx] == "," && scope == 1)
			paramCount++;

		idx++;
	}

	String funcName;
	if (paramCount == 2)
		funcName = "texture";
	if (paramCount == 3)
		funcName = "textureOffset";


	String texType = (*TextureTypeMap.find(texName)).second;
	String samplerFunc = "sampler" + texType.substr(sizeof("texture") - 1);

	String samplerStr;
	samplerStr += funcName;
	samplerStr += "(";
	samplerStr += samplerFunc;
	samplerStr += "(";
	samplerStr += texName;
	samplerStr += ", ";
	samplerStr += samplerName;
	samplerStr += ")";
	i += 4;
	if (pTokens[i + 1] == "[")
	{
		i += 3;
	}

	tokensAdvanced += i;
	return samplerStr;
}

String RegisterToUnit(const String &reg)
{
	//BART NOTE IS THIS SAME IN STD::STRING?????
	return reg.substr(1, reg.size());
}

String ToGLSLType(const String &type)
{
	StrMap<String>::iterator strIter = TypeConvTable.find(type);

	if (strIter == TypeConvTable.end())
	{
		return type;
	}
	else
	{
		return (*strIter).second;
	}
}

void CreateMaps()
{
	{
		TypeConvTable["float2"] =  "vec2";
		TypeConvTable["float3"] =  "vec3";
		TypeConvTable["float4"] =  "vec4";

		TypeConvTable["int2"] =  "ivec2";
		TypeConvTable["int3"] =  "ivec3";
		TypeConvTable["int4"] =  "ivec4";

		TypeConvTable["uint2"] =  "uvec2";
		TypeConvTable["uint3"] =  "uvec3";
		TypeConvTable["uint4"] =  "uvec4";

		TypeConvTable["float2x2"] =  "mat2";
		TypeConvTable["float3x3"] =  "mat3";
		TypeConvTable["float4x4"] =  "mat4";

		TypeConvTable["matrix"] =  "mat4";

		TypeConvTable["int"] =  "int";
		TypeConvTable["uint"] =  "uint";
		TypeConvTable["float"] =  "float";


	}

	{

		FuncConvTable["lerp"] = "mix";
		FuncConvTable["frac"] = "fract";
		FuncConvTable["fmod"] = "mod";
	}
}

bool TokenIsDataType(const String & token)
{
	if (TypeConvTable.find(token) != TypeConvTable.end())
	{
		return true;
	}
	else
	{
		bool found = Find(Structs, &token, CompareStruct) != 0;
		if (found)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

}

}