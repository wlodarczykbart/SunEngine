#pragma once

#include "Scene.h"

namespace SunEngine
{
	class SceneMgr
	{
	public:
		SceneMgr(const SceneMgr&) = delete;
		SceneMgr& operator = (const SceneMgr&) = delete;

		static SceneMgr& Get();

		Scene* AddScene(const String& name);

		bool SetActiveScene(const String& name);
		Scene* GetActiveScene() const { return _activeScene; }
	private:
		SceneMgr();
		~SceneMgr();

		Scene* _activeScene;
		StrMap<UniquePtr<Scene>> _scenes;
	};
}