#include <Windows.h>
#include <windowsx.h>

#include <map>
#include <mutex>

#include "GraphicsWindow.h"


namespace SunEngine
{
	int NativeKeyCodes[KEY_CODE_COUNT]
	{
		'0',
		'1',
		'2',
		'3',
		'4',
		'5',
		'6',
		'7',
		'8',
		'9',

		'A',
		'B',
		'C',
		'D',
		'E',
		'F',
		'G',
		'H',
		'I',
		'J',
		'K',
		'L',
		'M',
		'N',
		'O',
		'P',
		'Q',
		'R',
		'S',
		'T',
		'U',
		'V',
		'W',
		'X',
		'Y',
		'Z',

		VK_UP,
		VK_DOWN,
		VK_LEFT,
		VK_RIGHT,

		VK_BACK,
		VK_ESCAPE,
		VK_TAB,

		VK_RETURN,
		VK_SPACE,
		VK_PRIOR,
		VK_NEXT,
		VK_END,
		VK_HOME,
		VK_INSERT,
		VK_DELETE,
		VK_HELP,

		VK_LBUTTON,
		VK_RBUTTON,
		VK_MBUTTON,

		VK_CONTROL,
		VK_MENU,
		VK_LSHIFT,
		VK_RSHIFT,
	};


	struct KeyCodeNode
	{
		KeyCode code;
		bool supported;
	};

	KeyCodeNode GWSupportedKeys[4096];
	Map<HANDLE, GraphicsWindow*> GraphicsWindowMap;

	MouseButtonCode GetMouseButtonCode(uint windowCode)
	{
		switch (windowCode)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			return MouseButtonCode::MOUSE_LEFT;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			return MouseButtonCode::MOUSE_RIGHT;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			return MouseButtonCode::MOUSE_MIDDLE;
		default:
			return MouseButtonCode::MOUSE_BUTTON_COUNT;
		}
	}

	GraphicsWindow::GraphicsWindow()
	{
		_width = 0;
		_height = 0;
		_hWnd = 0;
		_parentHWnd = 0;
		_alive = false;
		_eventCounter = 0;
		memset(_events, 0, sizeof(_events));
	}


	GraphicsWindow::~GraphicsWindow()
	{
	}

	uint GraphicsWindow::Width() const
	{
		return _width;
	}

	uint GraphicsWindow::Height() const
	{
		return _height;
	}

	HWND GraphicsWindow::Handle() const
	{
		return _hWnd;
	}

	bool GraphicsWindow::Create(const CreateInfo& info)
	{
		uint width = info.width;
		uint height = info.height;

		static WNDCLASSEX ex = {};
		static bool classRegistered = false;

		if (!classRegistered)
		{
			ex.cbSize = sizeof(WNDCLASSEX);
			ex.hInstance = GetModuleHandle(0);
			ex.hCursor = LoadCursor(0, IDC_ARROW);
			ex.lpfnWndProc = WndProc;
			ex.lpszClassName = "GraphicsWindow";
			ex.style = CS_VREDRAW | CS_HREDRAW;
			ex.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

			if (!RegisterClassEx(&ex))
			{
				_errStr = "Failed to register window class";
				return false;
			}

			classRegistered = true;
		}

		if (!classRegistered)
			return false;

		DWORD style = info.windowStyle;
		RECT wr = {};
		wr.right = width;
		wr.bottom = height;
		AdjustWindowRect(&wr, style, false);

		uint rWidth = wr.right - wr.left;
		uint rHeight = wr.bottom - wr.top;

		_hWnd = CreateWindow(
			ex.lpszClassName,
			info.title.data(),
			style,
			0,
			0,
			rWidth,
			rHeight,
			0,
			0,
			ex.hInstance,
			0
		);

		if (!_hWnd)
		{
			_errStr = "Failed to create win32 handle";
			return false;
		}
		
		GraphicsWindowMap[_hWnd] = this;

		_width = width;
		_height = height;

		memset(GWSupportedKeys, 0, sizeof(GWSupportedKeys));

		uint supportedKeyCount = sizeof(NativeKeyCodes) / sizeof(*NativeKeyCodes);
		for (uint i = 0; i < supportedKeyCount; i++)
		{
			GWSupportedKeys[NativeKeyCodes[i]].supported = true;
			GWSupportedKeys[NativeKeyCodes[i]].code = (KeyCode)i;
		}

		_parentHWnd = info.parentHwnd;
		if (_parentHWnd)
		{
			SetParent(_hWnd, _parentHWnd);
		}

		return true;
	}

	bool GraphicsWindow::Destroy()
	{
		if (_hWnd)
		{
			Close();
			GraphicsWindowMap.erase(_hWnd);
			BOOL bDestroyed = DestroyWindow(_hWnd);
			_hWnd = 0;
			return bDestroyed;
		}
		else
		{
			return true;
		}
	}

	void GraphicsWindow::Open()
	{
		if (_hWnd)
		{
			_alive = true;
			ShowWindow(_hWnd, SW_SHOW);
		}
	}

	void GraphicsWindow::Close()
	{
		if (_hWnd)
		{
			_alive = false;
			ShowWindow(_hWnd, SW_HIDE);
		}
	}

	void GraphicsWindow::Update(GWEventData** ppEvents, uint& numEvents)
	{
		MSG msg = {};
		while (PeekMessage(&msg, _hWnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (ppEvents)
		{
			*ppEvents = _events;
		}
		numEvents = _eventCounter;
		_eventCounter = 0;
	}

	bool GraphicsWindow::IsAlive() const
	{
		return _alive;
	}

	void GraphicsWindow::UpdateFromParent()
	{
		RECT r;
		GetWindowRect(_parentHWnd, &r);
		MoveWindow(_hWnd, 0, 0, r.right - r.left, r.bottom - r.top, true);
	}

	LRESULT GraphicsWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		GraphicsWindow *pWindow = 0;
		Map<HANDLE, GraphicsWindow*>::iterator it = GraphicsWindowMap.find(hWnd);
		if (it != GraphicsWindowMap.end())
		{
			pWindow = (*it).second;
		}

		GWEventData e;
		e.type = GWE_INVALID;

		if (pWindow)
		{
			switch (msg)
			{
			case WM_DESTROY:
				pWindow->Close();
				break;

			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			{
				
				e.type = GWE_MOUSE_DOWN;
				e.mouseButtonCode = GetMouseButtonCode(msg);
				e.x = GET_X_LPARAM(lParam);
				e.y = GET_Y_LPARAM(lParam);
			}
				break;

				break;
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			{
				e.type = GWE_MOUSE_UP;
				e.mouseButtonCode = GetMouseButtonCode(msg);
				e.x = GET_X_LPARAM(lParam);
				e.y = GET_Y_LPARAM(lParam);
			}
			break;

			case WM_MOUSEMOVE:
			{
				e.type = GWE_MOUSE_MOVE;
				e.x = GET_X_LPARAM(lParam);
				e.y = GET_Y_LPARAM(lParam);
			}
			break;

			case WM_KEYDOWN:
			{
				uint key = (uint)wParam;
				if (GWSupportedKeys[key].supported)
				{
					e.type = GWE_KEY_DOWN;
					e.keyCode = GWSupportedKeys[key].code;
					e.nativeKey = key;
				}
			}
				break;

			case WM_KEYUP:
			{
				uint key = (uint)wParam;
				if (GWSupportedKeys[key].supported)
				{
					e.type = GWE_KEY_UP;
					e.keyCode = GWSupportedKeys[key].code;
					e.nativeKey = key;
				}
			}
			break;

			case WM_MOUSEWHEEL:
			{
				e.type = GWE_MOUSE_WHEEL;
				e.x = GET_X_LPARAM(lParam);
				e.y = GET_Y_LPARAM(lParam);
				uint key = GET_KEYSTATE_WPARAM(wParam);
				if (GWSupportedKeys[key].supported)
					e.keyCode = GWSupportedKeys[key].code;
				e.delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			}
			break;

			case WM_CHAR:
			{
				e.type = GWE_KEY_INPUT;
				e.nativeKey = (int)wParam;
			}
			break;

			case WM_SETFOCUS:
			{
				e.type = GWE_FOCUS_SET;
			}
			break;

			case WM_KILLFOCUS:
			{
				e.type = GWE_FOCUS_LOST;
			}
			break;

			default:
				break;
			}
		}

		if (pWindow && pWindow->_eventCounter < MAX_EVENTS && e.type != GWE_INVALID)
		{
			pWindow->_events[pWindow->_eventCounter] = e;
			++pWindow->_eventCounter;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}


	const String& GraphicsWindow::GetErrStr() const
	{
		return _errStr;
	}

	void GraphicsWindow::SetPosition(int x, int y)
	{
		RECT r;
		GetWindowRect(_hWnd, &r);
		MoveWindow(_hWnd, x, y, r.right - r.left, r.bottom - r.top, true);
	}

	void GraphicsWindow::GetMousePosition(int &x, int &y) const
	{
		POINT p = {};
		GetCursorPos(&p);
		ScreenToClient(_hWnd, &p);
		x = (int)p.x;
		y = (int)p.y;
	}

	bool GraphicsWindow::KeyDown(KeyCode code)
	{
		return (GetAsyncKeyState(NativeKeyCodes[code]) & 0x8000) != 0;
	}

}