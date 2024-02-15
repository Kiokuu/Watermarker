#include <iostream>
#include "ExecutableFile.h"

int main(int argc, char *argv[]) {

	// Print the arguments passed to the program.

	for(int i = 0; i < argc; i++) {
		std::cout << "arg[" << i << "]: " << argv[i] << "\n";
	}

	if(argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <executable_path>" << "\n";
		return 1;
	}

	// arg[0] is the program path, arg[1] is the first argument.


	ExecutableFile file(argv[1]);
	// Save the modified executable file to a new file
	file.save("modified.exe");


	std::cin.get();
	return 0;
}