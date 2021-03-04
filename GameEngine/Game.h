#pragma once

namespace SunEngine
{
	class Game
	{
		Game();
		Game(const Game&) = delete;
		Game& operator = (const Game&) = delete;
		~Game();
	};
}