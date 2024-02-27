// Watermark.cpp : Source file for your target.
//

#include "Watermark.h"

#include <iostream>

#include <Windows.h>

int main()
{
	//MessageBoxA(0, "Box", "Title", 0);
	return 0;
}

__declspec(dllexport) void Watermark()
{
	std::cout << "Hi from watermark!" << std::endl;
	MessageBoxA(0, "Boo!", "Title", 0);
}
