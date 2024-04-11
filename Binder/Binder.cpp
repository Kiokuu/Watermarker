// Binder.cpp : Defines the entry point for the application.
//
#include "Binder.h"

#include <iostream>
#include <filesystem>


#include <zasm/formatter/formatter.hpp>
#include "zasm/base/mode.hpp"
#include "zasm/serialization/serializer.hpp"
#include "zasm/x86/assembler.hpp"
#include "zasm/x86/memory.hpp"

Binder::Binder(): m_executable(nullptr), m_program(zasm::MachineMode::AMD64), m_assembler(m_program), m_serializer()
{
}

bool Binder::bind(std::string_view path)
{
	m_executablePath = path;

	if (!loadExecutable())
	{
		return false;
	}

	return true;
}

bool Binder::loadExecutable()
{
	// Attempt to load the executable file from the path
	try
	{
		m_executable = new ExecutableFile(m_executablePath);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to load executable: " << e.what() << "\n";
		std::cerr << "Path: " << m_executablePath << "\n";
		return false;
	}

	return true;
}

bool Binder::writeAssembly(std::string_view watermark)
{
	// Add the watermark function to the imports.
	m_executable->addImport("Watermark.dll", "Watermark");
	m_executable->rewriteImports();

	ImageSection* watermarkSection;

	// Create a new section for the additional code.
	m_executable->createSection(".newsec", 0x1000, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_EXECUTE,
	                            &watermarkSection);

	// Get the original entry point of the executable to jump back to later
	uint32_t OEP = m_executable->getEntryPoint();

	// Get the address of the watermark function
	uint32_t waterAddress = m_executable->getImportAddress("Watermark.dll", "Watermark");

	// Create two labels, one for the start and one to point to the watermark data.
	auto startLabel = m_program.createLabel("start");
	auto watermarkLabel = m_program.createLabel("watermark");

	// Attach to the start label
	m_assembler.bind(startLabel);

	std::vector<zasm::x86::Gp> regsToSave = {
		zasm::x86::rcx, zasm::x86::rdx, zasm::x86::r8, zasm::x86::r9, zasm::x86::rbp
	};

	for (auto reg : regsToSave)
	{
		m_assembler.push(reg);
	}

	// Save the stack
	m_assembler.mov(zasm::x86::rbp, zasm::x86::rsp);

	// Push the watermark data address to the RCX register
	m_assembler.lea(zasm::x86::rcx, qword_ptr(zasm::x86::rip, watermarkLabel));

	// Call the watermark function
	m_assembler.call(qword_ptr(zasm::x86::rip, waterAddress));

	// Restore the stack
	m_assembler.mov(zasm::x86::rsp, zasm::x86::rbp);

	for (auto reg : regsToSave)
	{
		m_assembler.pop(reg);
	}

	// Write the jump back to the original entry point
	m_assembler.jmp(zasm::Imm32(OEP));

	// Attach to the watermark label
	m_assembler.bind(watermarkLabel);

	// Write the watermark data within the watermark label with a null terminator.
	const char* watermarkData = watermark.data();
	m_assembler.embed(watermarkData, strlen(watermarkData) + 1);

	// Serialize the assembly into bytes
	zasm::Error err = m_serializer.serialize(m_program, watermarkSection->getVirtualAddress());

	if (err != zasm::Error::None)
	{
		std::cerr << "Failed to serialize assembly: " << "\n";
		return false;
	}

	// Set the data of the section to the serialized bytes
	watermarkSection->setData({const_cast<std::uint8_t*>(m_serializer.getCode()), m_serializer.getCodeSize()});

	// Set the entry point of the executable file
	m_executable->setEntryPoint(watermarkSection->getVirtualAddress());
	
	return true;
}

bool Binder::save(std::string_view name, std::string_view watermark)
{
	if (m_executable == nullptr)
	{
		return false;
	}

	// Get the executablePath parent directory
	std::filesystem::path parentPath = std::filesystem::path(m_executablePath).parent_path();

	// Create a path with parent path + name
	

	writeAssembly(watermark);
	m_executable->save(parentPath.string() + "\\" + name.data());

	// Check if watermark.dll is in the current directory (not the executable directory)
	std::filesystem::path watermarkPath = std::filesystem::current_path() / "Watermark.dll";

	if (!std::filesystem::exists(watermarkPath))
	{
		std::cerr << "Watermark.dll not found in current directory\n";
		return false;
	}

	// Copy the watermark.dll to the executable directory
	std::filesystem::copy(watermarkPath, parentPath.string() + "\\" + "Watermark.dll", std::filesystem::copy_options::overwrite_existing);

	delete m_executable;
	m_executable = nullptr;

	return true;
}
