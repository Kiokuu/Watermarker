#include <filesystem>
#include <iostream>
#include "GUI/ImguiHandler.h"

int main(int argc, char* argv[])
{
	std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());

	ImguiHandler imguiHandler;
	imguiHandler.initialize();

	std::cin.get();
	return 0;
}
