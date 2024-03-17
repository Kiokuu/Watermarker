#pragma once

#include "ExecutableFile.h"
#include "zasm/program/program.hpp"
#include "zasm/serialization/serializer.hpp"
#include "zasm/x86/assembler.hpp"

/**
 * @brief The Binder class is responsible for binding watermark to executable files.
 */
class Binder
{
public:
	/**
	 * @brief Constructs a new Binder object.
	 */
	Binder();

	/**
	 * @brief Binds a watermark to the specified executable file.
	 * @param path The path to the executable file.
	 * @return True if the watermark was successfully bound, false otherwise.
	 */
	bool bind(std::string_view path);

	/**
	 * @brief Saves the executable file with the bound watermark.
	 * @param name The path to save the executable file.
	 * @param watermark The watermark to bind.
	 * @return True if the file was successfully saved, false otherwise.
	 */
	bool save(std::string_view name, std::string_view watermark);

	/**
	 * @brief Checks if the Binder is ready to bind a watermark.
	 * @return True if the Binder is ready, false otherwise.
	 */
	bool isReadyToBind() const { return m_executable != nullptr; }

private:
	/**
	 * @brief Loads the executable file.
	 * @return True if the file was loaded successfully, false otherwise.
	 */
	bool loadExecutable();

	/**
	 * @brief Writes assembly code to bind the watermark.
	 * @param watermark The watermark to bind.
	 * @return True if the assembly code was written successfully, false otherwise.
	 */
	bool writeAssembly(std::string_view watermark);

	std::string m_executablePath; /**< The path to the executable file. */
	ExecutableFile* m_executable; /**< Pointer to the ExecutableFile object. */

	zasm::Program m_program; /**< ZASM program object. */
	zasm::x86::Assembler m_assembler; /**< ZASM x86 assembler object. */
	zasm::Serializer m_serializer; /**< ZASM serializer object. */
};
