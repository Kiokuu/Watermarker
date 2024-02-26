// Watermark.cpp : Source file for your target.
//

#include "Watermark.h"
#include <Windows.h>

int main()
{
	//MessageBoxA(0, "Box", "Title", 0);
	return 0;
}

__declspec(dllexport) void Watermark()
{
	MessageBoxA(0, "Box", "Title", 0);
}
