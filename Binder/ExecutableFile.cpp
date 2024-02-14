#include "ExecutableFile.h"
#include <fstream>
#include <iostream>

ExecutableFile::ExecutableFile(std::string_view executablePath)
{
	// Open the executable file
	std::ifstream file(executablePath.data(), std::ios::binary);

	if(!file.is_open())
	{
		throw std::runtime_error("Failed to open file");
	}

	// Ensure the vector is large enough to hold the file
	file.seekg(0, std::ios::end);
	m_data.resize(file.tellg());
	file.seekg(0, std::ios::beg);

	// Read the file into the vector
	file.read(reinterpret_cast<char*>(m_data.data()), m_data.size());

	// Parse the file
	parse();

	// Close the file
	file.close();
}

void ExecutableFile::save(std::string_view savePath)
{

}

void ExecutableFile::parse()
{
	parseDosHeader();
	parseNtHeaders();
	parseSections();
	parseImports();
	parseExports();
}

void ExecutableFile::parseDosHeader()
{
	m_dosHeader = *reinterpret_cast<IMAGE_DOS_HEADER*>(m_data.data());

	if (m_dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
	{
		throw std::runtime_error("Invalid DOS header");
	}

	if (m_dosHeader.e_lfanew == 0 || m_dosHeader.e_lfanew >= m_data.size())
	{
		throw std::runtime_error("Invalid DOS header -- NT Header offset");
	}
}

void ExecutableFile::parseNtHeaders()
{
	m_ntHeaders = *reinterpret_cast<IMAGE_NT_HEADERS*>(m_data.data() + m_dosHeader.e_lfanew);
	if (m_ntHeaders.Signature != IMAGE_NT_SIGNATURE)
	{
		throw std::runtime_error("Invalid NT headers");
	}

	if (m_ntHeaders.FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER))
	{
		throw std::runtime_error("Invalid optional header size");
	}

	if (m_ntHeaders.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) // 64bit
	{
		throw std::runtime_error("Invalid optional header magic");
	}

	if (m_ntHeaders.OptionalHeader.AddressOfEntryPoint == 0)
	{
		throw std::runtime_error("Invalid entry point");
	}
}

void ExecutableFile::parseSections()
{
	m_sections.resize(m_ntHeaders.FileHeader.NumberOfSections);

	for (size_t i = 0; i < m_ntHeaders.FileHeader.NumberOfSections; i++)
	{
		const auto section = reinterpret_cast<IMAGE_SECTION_HEADER*>(m_data.data() + m_dosHeader.e_lfanew + sizeof(
			IMAGE_NT_HEADERS) + i * sizeof(IMAGE_SECTION_HEADER));

		m_sections[i] = ImageSection(
			std::string_view(reinterpret_cast<const char*>(section->Name), sizeof(section->Name)),
			section->VirtualAddress,
			section->Misc.VirtualSize,
			section->PointerToRawData,
			section->SizeOfRawData,
			section->Characteristics,
			std::span(m_data.data() + section->PointerToRawData, section->SizeOfRawData)
		);
	}
}

void ExecutableFile::parseImports()
{
	
}

void ExecutableFile::parseExports()
{
	

ImageSection* ExecutableFile::getSection(std::string_view name)
{
	for (auto& section : m_sections)
	{
		if (name == section.getName())
		{
			return &section;
		}
	}
	return nullptr;
}

ImageSection* ExecutableFile::getSection(uint32_t virtualAddress)
{
	for (auto& section : m_sections)
	{
		if (virtualAddress >= section.getVirtualAddress() && virtualAddress < section.getVirtualAddress() + section.
			getVirtualSize())
		{
			return &section;
		}
	}
	return nullptr;
}