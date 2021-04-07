#pragma once

#include "Shader.h"

namespace SunEngine
{
	class DefaultShaders
	{
	public:
		static const String Metallic;
		static const String Specular;
		static const String Gamma;
		static const String Deferred;
		static const String ScreenSpaceReflection;
		static const String SceneCopy;

	private:
		DefaultShaders() = delete;
		DefaultShaders(const DefaultShaders&) = delete;
		DefaultShaders& operator= (const DefaultShaders&) = delete;
	};

	class ShaderMgr
	{
	public:
		ShaderMgr(const ShaderMgr&) = delete;
		ShaderMgr& operator = (const ShaderMgr&) = delete;

		static ShaderMgr& Get();

		Shader* GetShader(const String& name) const;

		bool LoadShaders(String& errMsg);
	private:
		ShaderMgr();
		~ShaderMgr();

		StrMap<UniquePtr<Shader>> _shaders;
	};
}