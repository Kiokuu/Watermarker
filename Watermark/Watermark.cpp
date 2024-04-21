#include <iostream>
#include <Windows.h>
#include <d3d11.h>

#include "D3D11RenderHook.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool __stdcall DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return true;
}

D3D11RenderHook* g_pRenderHook = nullptr;

/*
 * @brief Entry point for the watermark DLL.
 * @param watermark_data The watermark text to render.
*/
extern "C" __declspec(dllexport) void Watermark(const char* watermark_data)
{
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	// Create the window class
	WNDCLASS wnd = {};
	wnd.lpfnWndProc = DefWindowProc;
	wnd.hInstance = GetModuleHandle(nullptr);
	wnd.lpszClassName = "Watermark";
	wnd.style = CS_HREDRAW | CS_VREDRAW;

	// Register the window class
	if (!RegisterClass(&wnd))
	{
		MessageBoxA(nullptr, "Failed to register window class", "Error", 0);
		return;
	}

	// Create the window, but don't show it
	HWND hWnd = CreateWindow("Watermark", "Watermark", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, NULL, NULL);

	// If the window creation failed, show an error message and return
	if (hWnd == nullptr)
	{
	    MessageBoxA(nullptr, "Failed to create window", "Error", 0);
	    return;
	}

	// Create the render hook, which will create the D3D11 device and hook the swapchain
	g_pRenderHook = new D3D11RenderHook(hWnd, watermark_data);

    // Destroy the window and unregister the class as it is no longer needed.
	DestroyWindow(hWnd);
	UnregisterClass("Watermark", GetModuleHandle(nullptr));
}
