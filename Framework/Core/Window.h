#pragma once
#include "Framework.h"

namespace Window
{
	static HINSTANCE Instance;
	static HWND Handle;
	static std::wstring AppName;
	static bool IsFullScreen;
	
	static std::function<LRESULT(HWND, uint, WPARAM, LPARAM)> EditorProc;
	static bool bEdtor = false;
	static std::function<void(const uint&, const uint&)> Resize;

	inline LRESULT CALLBACK WndProc
	(
		HWND handle,
		uint message,
		WPARAM wParam,
		LPARAM lParam
	)
	{
		Mouse::Get()->InputProc(message, wParam, lParam);
			

		if (bEdtor)
			EditorProc(handle, message, wParam, lParam);

		switch (message)
		{
		case WM_DISPLAYCHANGE:
		case WM_SIZE:
			if (Resize!=nullptr&&wParam != SIZE_MINIMIZED)
				Resize(lParam & 0xffff, (lParam >> 16) & 0xffff);
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(handle, message, wParam, lParam);
		}

		return 0;
	}

	inline void Create
	(
		HINSTANCE instance,
		const std::wstring& appName,
		const uint& width,
		const uint& height,
		const bool& bFullScreen = false
	)
	{
		Instance = instance;
		AppName = appName;
		IsFullScreen = bFullScreen;

		WNDCLASSEX wndClass;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
		wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wndClass.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
		wndClass.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);
		wndClass.hInstance = instance;
		wndClass.lpfnWndProc = static_cast<WNDPROC>(WndProc);
		wndClass.lpszClassName = appName.c_str();
		wndClass.lpszMenuName = nullptr;
		wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wndClass.cbSize = sizeof(WNDCLASSEX);

		WORD check = RegisterClassEx(&wndClass);
		assert(check != 0);

		/*if (bFullScreen == true)
		{
			DEVMODE devMode = { 0 };
			devMode.dmSize = sizeof(DEVMODE);
			devMode.dmPelsWidth = (DWORD)width;
			devMode.dmPelsHeight = (DWORD)height;
			devMode.dmBitsPerPel = 32;
			devMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			
			ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);
		}*/
		Handle = CreateWindowExW
		(
			WS_EX_APPWINDOW,
			appName.c_str(),
			appName.c_str(),
			WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			nullptr,
			nullptr,
			instance,
			nullptr
		);
		assert(Handle != nullptr);

		RECT screen_rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};

		uint x = (GetSystemMetrics(SM_CXSCREEN) - width) /2;
		uint y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

		AdjustWindowRect(&screen_rect, WS_OVERLAPPEDWINDOW| WS_POPUPWINDOW, FALSE);
		MoveWindow
		(
			Handle,
			x,
			y,
			screen_rect.right - screen_rect.left,
			screen_rect.bottom - screen_rect.top,
			TRUE
		);
	}

	inline void Show()
	{
		SetForegroundWindow(Handle);
		SetFocus(Handle);
		ShowCursor(TRUE);
		ShowWindow(Handle, SW_SHOWNORMAL);
		UpdateWindow(Handle);
	}

	inline const bool Update()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{

		}

		return msg.message != WM_QUIT;
	}

	inline void Destroy()
	{
		if (IsFullScreen)
		{
			ChangeDisplaySettings(NULL, CDS_RESET);
		}
		DestroyWindow(Handle);
		UnregisterClass(AppName.c_str(), Instance);
	}

	inline const uint GetWidth()
	{
		RECT rect;
		GetClientRect(Handle, &rect);
		return static_cast<uint>(rect.right - rect.left);
	}

	inline const uint GetHeight()
	{
		RECT rect;
		GetClientRect(Handle, &rect);
		return static_cast<uint>(rect.bottom - rect.top);
	}
}