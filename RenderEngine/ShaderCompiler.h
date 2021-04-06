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

		bool Compile();

	private:
		void GetBinding(const String& name, const String& type, uint* pBindings, ShaderBindingType& bindType);
		void PreProcessText(const String& inText, String& outHLSL, String& outGLSL);
		bool CompileShader(ShaderStage type, const String& source);
		bool ParseShaderFile(String& output, const String& input);
		void ConvertToLines(const String& input, Vector<String>& lines) const;

		BaseShader::CreateInfo _shaderInfo;

		uint _numUserTextures;
		uint _numUserSamplers;

		String _vertexSource;
		String _pixelSource;

		String _lastErr;

		HashSet<String> _includedFiles;
		Vector<String> _defines;

	};
}