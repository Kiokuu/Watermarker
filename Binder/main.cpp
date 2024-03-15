#include <iostream>
#include "GUI/ImguiHandler.h"

int main(int argc, char* argv[])
{
	ImguiHandler imguiHandler;
	imguiHandler.initialize();

	std::cin.get();
	return 0;
}


/*
 * Rewrite imports on startup 
 * Iterate through old import directory, what imports i need, where theey need to be written, iterate through new one, write them over.
 * entrypoint copies the new stuff over to the old one
 * Write DX11 hook  (watermark side)
 *
*/