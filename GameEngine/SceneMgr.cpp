#include "StringUtil.h"

#include "SceneMgr.h"

namespace SunEngine
{
	SceneMgr& SceneMgr::Get()
	{
		static SceneMgr mgr;
		return mgr;
	}

	SceneMgr::SceneMgr()
	{
		_activeScene = 0;
	}

	SceneMgr::~SceneMgr()
	{

	}

	Scene* SceneMgr::AddScene(const String& sceneName)
	{
		uint counter = 0;
		String unqiueName = sceneName;
		while (_scenes.find(unqiueName) != _scenes.end())
		{
			unqiueName = StrFormat("%s_%d", sceneName.c_str(), ++counter);
		}

		Scene* pscene = new Scene();
		pscene->_name = unqiueName;
		_scenes[unqiueName] = UniquePtr<Scene>(pscene);
		return pscene;
	}

	bool SceneMgr::SetActiveScene(const String& name)
	{
		auto found = _scenes.find(name);
		if (found != _scenes.end())
		{
			_activeScene = (*found).second.get();
			return true;
		}
		else
		{
			return false;
		}
	}


}