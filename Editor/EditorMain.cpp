#include "stdafx.h"
#include "Editor.h"
#include "./Core/Window.h"
#include "./ImGui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain
(
	HINSTANCE hInstance,
	HINSTANCE prevInstance,
	LPSTR lpszCmdParam,
	int nCmdShow
)
{
	
	{
		Window::Create(hInstance, L"D3D11Editor", 1280, 720, false);
		Window::Show();
		D3DDesc desc;
		desc.AppName = Window::AppName;
		desc.Instance = Window::Instance;
		desc.bFullScreen = Window::IsFullScreen;
		desc.bVsync = false;
		desc.Handle = Window::Handle;
		desc.Width = Window::GetWidth();
		desc.Height = Window::GetHeight();
		
		D3D::SetDesc(desc);
	}
	
	
	{
		Thread::Create();
		Time::Create();
		Time::Get()->Start();
		Keyboard::Create();
		Mouse::Create();
		//DebugLine::Create();
	}

	auto editor = std::make_unique<Editor>();
	

	Window::bEdtor = true;
	Window::EditorProc = ImGui_ImplWin32_WndProcHandler;
	Window::Resize = [&editor](const uint& width, const uint& height)
	{
		editor->Resize(width, height);
	};
	
	MSG msg = { 0 };
	
	while (true)
	{
		
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
				
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//Update
			{
			
				Keyboard::Get()->Update();
				Mouse::Get()->Update();
				Time::Get()->Update();
				editor->Update();
			}

			editor->Render();
						
		}
		
	}


	{
		//DebugLine::Delete();
		Mouse::Delete();
		Keyboard::Delete();
		Time::Delete();
		Thread::Delete();
	}
	

	Window::Destroy();
	return 0;
}