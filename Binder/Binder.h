#pragma once
#include "ExecutableFile.h"
#include "zasm/program/program.hpp"
#include "zasm/serialization/serializer.hpp"
#include "zasm/x86/assembler.hpp"

class Binder
{
public:
	Binder();
	bool bind(std::string_view path);
	bool save(std::string_view path, std::string_view watermark);

	bool isReadyToBind() const { return m_executable != nullptr; }
private:
	bool loadExecutable();
	bool writeAssembly(std::string_view watermark);

	std::string m_executablePath;
	ExecutableFile* m_executable;

	std::uint8_t* m_watermarkData;
	std::size_t m_watermarkSize;

	zasm::Program m_program;
	zasm::x86::Assembler m_assembler;
	zasm::Serializer m_serializer;
};
