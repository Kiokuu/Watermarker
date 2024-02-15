#pragma once
#include <string_view>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include "ImageSection.h"
#include "Windows.h"
#include "ImportedModule.h"
#include "ExportedModule.h"


/**
 * Notes:
 * PE Format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 * Dos Header -> NT Headers -> Section Headers -> Import Info -> Export Info -> ... -> Data
 */


struct AddressPair
{
	uint32_t virtualAddress;
	uint32_t rawAddress;
};

class ExecutableFile
{
public:
	explicit ExecutableFile() = default;
	explicit ExecutableFile(std::string_view executablePath);

	~ExecutableFile() = default;
	void save(std::string_view savePath);

	const std::vector<uint8_t>& getData() const { return m_data; }
	const std::vector<ImageSection>& getSections() const { return m_sections; }
	const std::vector<ImportedModule>& getImports() const { return m_imports; }

private:
	AddressPair getNextAddress() const;
	AddressPair getFirstSectionAddress() const;

	/**
	 * Parsing
	 */
	void parse();
	void parseDosHeader();
	void parseNtHeaders();
	void parseSections();
	void parseImports();
	void parseExports();


	ImageSection* getSection(std::string_view name);
	ImageSection* getSection(uint32_t virtualAddress);

	std::vector<uint8_t> m_data;
	std::vector<ImageSection> m_sections;
	std::vector<ImportedModule> m_imports;
	std::vector<ExportedModule> m_exports;

	IMAGE_DOS_HEADER m_dosHeader;
	IMAGE_NT_HEADERS m_ntHeaders;
};
