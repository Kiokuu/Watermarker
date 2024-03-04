#include "Watermark.h"
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

extern "C" __declspec(dllexport) void Watermark()
{
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	WNDCLASS wnd;
	wnd.style         = CS_HREDRAW | CS_VREDRAW;
	wnd.lpfnWndProc   = DefWindowProc;
	wnd.cbClsExtra    = 0;
	wnd.cbWndExtra    = 0;
	wnd.hInstance     = GetModuleHandle(NULL);
	wnd.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wnd.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wnd.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wnd.lpszMenuName  = NULL;
	wnd.lpszClassName = "Watermark";

	if(!RegisterClass(&wnd))
	{
		MessageBoxA(0, "Failed to register window class", "Error", 0);
		return;
	}

	HWND hWnd = CreateWindow("Watermark", "Watermark", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, NULL, NULL);
	if(hWnd == NULL)
	{
		MessageBoxA(0, "Failed to create window", "Error", 0);
		return;
	}

	g_pRenderHook = new D3D11RenderHook(hWnd);

	DestroyWindow(hWnd);
	UnregisterClass("Watermark", GetModuleHandle(NULL));
}


