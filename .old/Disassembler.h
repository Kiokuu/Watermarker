#pragma once
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "CustomInstructionFormatter.h"
#include "Zydis/DecoderTypes.h"
#include "Function.h"

class ImageSection;
class DataEntry;
class ExecutableFile;

struct CustomUserData
{
	CustomInstructionFormatter* formatter;
};


class Disassembler
{
public:
	
	Disassembler(ExecutableFile* executableFile);

private:
	ExecutableFile* m_executable_file;

	std::unordered_map<uint64_t, std::vector<Function>> m_functions;
	std::vector<ZydisDecodedInstruction> m_instructions;
	std::unordered_map<uint64_t, DataEntry> m_data;
	std::ofstream m_output;

	void disassemble_data();
	void disassemble_functions();
	void write_data();
	void write_functions();
	void write_globals();


	void write_section(const ImageSection* section);


	std::string getAssemblerDataType(uint32_t type);
};


