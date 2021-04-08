#pragma once

#include "BaseShader.h"

namespace SunEngine
{
	class ShaderCompiler
	{
	public:
		ShaderCompiler();
		ShaderCompiler(const ShaderCompiler&) = delete;
		ShaderCompiler& operator = (const ShaderCompiler&) = delete;
		~ShaderCompiler();

		//Must be set at some point before compiling shaders
		static void SetAuxiliaryDir(const String& path);

		void SetDefines(const Vector<String>& defines);
		void SetVertexShaderSource(const String& vertexShader);
		void SetPixelShaderSource(const String& pixelShader);

		const String& GetLastError() const { return _lastErr; }
		const BaseShader::CreateInfo& GetCreateInfo() const { return _shaderInfo; }

		bool Compile(const String& uniqueName = "");

	private:
		void GetBinding(const String& name, const String& type, uint* pBindings, ShaderBindingType& bindType);
		void PreProcessText(const String& inText, String& outHLSL, String& outGLSL);
		bool CompileShader(ShaderStage type);
		bool ParseShaderFile(String& output, const String& input, HashSet<String>& includeFiles);
		void ConvertToLines(const String& input, Vector<String>& lines) const;
		bool MatchesCachedFile(const String& path, uint stageFlags);
		bool WriteCachedFile(const String& path, uint stageFlags);

		BaseShader::CreateInfo _shaderInfo;

		uint _numUserTextures;
		uint _numUserSamplers;

		String _vertexSource;
		String _vertexHLSL;
		String _vertexGLSL;

		String _pixelSource;
		String _pixelHLSL;
		String _pixelGLSL;

		String _lastErr;
		String _uniqueName;

		Vector<String> _defines;

	};
}