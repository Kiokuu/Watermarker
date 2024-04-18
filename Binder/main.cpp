#include <filesystem>
#include <iostream>
#include "GUI/ImguiHandler.h"

int main(int argc, char* argv[])
{
	// Set the current working directory to the directory of the executable
	std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());

	// Initialize the GUI + program
	ImguiHandler imguiHandler;
	imguiHandler.initialize();

	std::cin.get();
	return 0;
}
