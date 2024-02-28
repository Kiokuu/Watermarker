#include <iostream>

#include "Binder.h"
#include "ExecutableFile.h"
#include "zasm/program/program.hpp"
#include "zasm/serialization/serializer.hpp"
#include "zasm/x86/assembler.hpp"
#include "zasm/x86/memory.hpp"

int main(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++)
	{
		std::cout << "arg[" << i << "]: " << argv[i] << "\n";
	}

	/*
	if (argc < 4)
	{
		std::cerr << "Usage: " << argv[0] << " <first_path> " << "<first_pdb> " << "<second_path> " << "<second_pdb>" <<
			"\n";
		return 1;
	}

	const char* originalPath = argv[1];
	const char* originalPDBPath = argv[2];
	const char* targetPath = argv[3];
	const char* targetPDBPath = argv[4];

	ExecutableFile originalExecutable(originalPath, originalPDBPath);
	ExecutableFile targetExecutable(targetPath, targetPDBPath);

	Binder binder(&originalExecutable, &targetExecutable);
	*/

	zasm::Program program(zasm::MachineMode::AMD64);
	zasm::x86::Assembler assembler(program);
	zasm::Serializer serializer;

	ExecutableFile file(argv[1]);

	
	file.addImport("Watermark.dll", "Watermark");
	file.rewriteImports();

	ImageSection* newSection;
	file.createSection(".newsec", 0x1000, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE, &newSection);

	uint32_t OEP = file.getEntryPoint();
	uint32_t waterAddress = file.getImportAddress("Watermark.dll", "Watermark");

	auto startLabel = program.createLabel("start");
	assembler.bind(startLabel);

	std::vector<zasm::x86::Gp> regsToSave = { zasm::x86::rcx, zasm::x86::rdx, zasm::x86::r8, zasm::x86::r9, zasm::x86::rbp };
	for(auto reg : regsToSave)
	{
		assembler.push(reg);
	}


	assembler.mov(zasm::x86::rbp, zasm::x86::rsp);

	assembler.call(zasm::x86::qword_ptr(zasm::x86::rip, waterAddress));

	assembler.mov(zasm::x86::rsp, zasm::x86::rbp);

	for(auto reg : regsToSave)
	{
		assembler.pop(reg);
	}

	assembler.jmp(zasm::Imm32(OEP));

	serializer.serialize(program, newSection->getVirtualAddress());
	std::uint8_t* code = const_cast<uint8_t*>(serializer.getCode());
	const std::size_t codeSize = serializer.getCodeSize();

	newSection->setData({code, codeSize});

	// Set the entry point of the executable file
	file.setEntryPoint(newSection->getVirtualAddress());

	file.save("modified.exe");

	/*
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

	
	// Save the modified executable file to a new file
	//file.save("modified.exe");


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