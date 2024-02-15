#include <iostream>
#include "ExecutableFile.h"
#include "zasm/program/program.hpp"
#include "zasm/serialization/serializer.hpp"
#include "zasm/x86/assembler.hpp"

int main(int argc, char *argv[]) {
	for(int i = 0; i < argc; i++) {
		std::cout << "arg[" << i << "]: " << argv[i] << "\n";
	}

	if(argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <executable_path>" << "\n";
		return 1;
	}

	/*
	zasm::Program program(zasm::MachineMode::AMD64);
	zasm::x86::Assembler assembler(program);
	zasm::Serializer serializer;

	ExecutableFile file(argv[1]);
	file.addImport("USER32.dll", "MessageBoxA");
	file.rewriteImports();

	ImageSection* newSection;
	file.createSection(".newsec", 0x1000, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, &newSection);

	uint32_t OEP = file.getEntryPoint();
	uint32_t messageAddress = file.getImportAddress("USER32.dll", "MessageBoxA");

	// Assembly program to jump to the original entry point.
	auto startLabel = program.createLabel("start");
	assembler.bind(startLabel);
	assembler.jmp(zasm::Imm32(OEP));


	serializer.serialize(program, newSection->getVirtualAddress());
	std::uint8_t* code = const_cast<uint8_t*>(serializer.getCode());

	const std::size_t codeSize = serializer.getCodeSize();

	newSection->setData({code, codeSize});

	// Set the entry point of the executable file
	file.setEntryPoint(newSection->getVirtualAddress());
	
	*/

	
	ExecutableFile file(argv[1]);
	ImageSection* newSection;
	file.createSection(".newsec", 0x1000, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, &newSection);

	uint32_t OEP = file.getEntryPoint();

	file.setEntryPoint(newSection->getVirtualAddress());

	// Save the modified executable file to a new file
	file.save("modified.exe");

	std::cin.get();
	return 0;
}
