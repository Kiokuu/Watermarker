#pragma once
#include <string_view>
#include <map>


// Forward declare executable file
class ExecutableFile;


class PDBFile
{
public:
	PDBFile() = default;
	PDBFile(ExecutableFile* executableFile, std::string_view path);

	std::string getSymbol(uint64_t address) const;

	ExecutableFile* getExecutableFile() const { return m_executableFile; }

private:
	ExecutableFile* m_executableFile;
	std::map<uint64_t, std::string> m_symbols;
};
