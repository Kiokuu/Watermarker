#pragma once
#include <string_view>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"


/**
 * Notes:
 * PE Format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 * Dos Header -> NT Headers -> Section Headers -> Import Info -> Export Info -> ... -> Data
 */


class ExecutableFile
{
public:
	explicit ExecutableFile() = default;
	explicit ExecutableFile(std::string_view executablePath);

	~ExecutableFile() = default;
	void save(std::string_view savePath);

private:
	/**
	 * Parsing
	 */
	void parse();
	void parseDosHeader();
	void parseNtHeaders();
	void parseSections();
	void parseImports();
	void parseExports();

	std::vector<uint8_t> m_data;
	IMAGE_DOS_HEADER m_dosHeader;
	IMAGE_NT_HEADERS m_ntHeaders;
};
