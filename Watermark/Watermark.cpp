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

extern "C" __declspec(dllexport) void Watermark(const char* watermark_data)
{
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	WNDCLASS wnd;
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.lpfnWndProc = DefWindowProc;
	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = 0;
	wnd.hInstance = GetModuleHandle(nullptr);
	wnd.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wnd.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wnd.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	wnd.lpszMenuName = nullptr;
	wnd.lpszClassName = "Watermark";

	if (!RegisterClass(&wnd))
	{
		MessageBoxA(nullptr, "Failed to register window class", "Error", 0);
		return;
	}

	HWND hWnd = CreateWindow("Watermark", "Watermark", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, NULL, NULL);
	if (hWnd == nullptr)
	{
		MessageBoxA(nullptr, "Failed to create window", "Error", 0);
		return;
	}

	g_pRenderHook = new D3D11RenderHook(hWnd, watermark_data);

	DestroyWindow(hWnd);
	UnregisterClass("Watermark", GetModuleHandle(nullptr));
}
