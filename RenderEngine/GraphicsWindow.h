#pragma once

#include <Windows.h>
#include "InputCodes.h"
#include "Types.h"

namespace SunEngine
{

	enum GWEvent
	{
		GWE_KEY_DOWN,
		GWE_KEY_UP,
		GWE_MOUSE_DOWN,
		GWE_MOUSE_UP,
		GWE_MOUSE_MOVE,
		GWE_RESIZE,
		GWE_MOUSE_WHEEL,
		GWE_KEY_INPUT,
		GWE_FOCUS_SET,
		GWE_FOCUS_LOST,

		GWE_INVALID = 0xFFFF77B
	};

	struct GWEventData
	{
		GWEvent type;
		KeyCode keyCode;
		MouseButtonCode mouseButtonCode;
		WindowSizeCode windowSizeCode;
		int x;
		int y;
		int delta;
		int nativeKey;
	};

	class GraphicsWindow
	{
	public:
		struct CreateInfo
		{
			String title;
			uint width;
			uint height;
			HWND parentHwnd;
			uint windowStyle;
		};

		GraphicsWindow();
		GraphicsWindow(const GraphicsWindow&) = delete;
		GraphicsWindow & operator = (const GraphicsWindow&) = delete;
		~GraphicsWindow();

		uint Width() const;
		uint Height() const;
		HWND Handle() const;

		bool Create(const CreateInfo& info);
		bool Destroy();

		void Open();
		void Close();
		void Update(GWEventData** ppEvents, uint& numEvents);

		bool IsAlive() const;

		void UpdateFromParent();

		const String& GetErrStr() const;

		void SetPosition(int x, int y);

		void GetMousePosition(int &x, int &y) const;

		static bool KeyDown(KeyCode code);

	private:
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static const uint MAX_EVENTS = 256;

		uint _width;
		uint _height;
		HWND _hWnd;
		HWND _parentHWnd;

		GWEventData _events[MAX_EVENTS];
		uint _eventCounter;

		String _errStr;

		bool _alive;
	};

}
