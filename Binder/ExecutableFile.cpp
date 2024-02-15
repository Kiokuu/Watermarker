#include "ExecutableFile.h"
#include <fstream>
#include <iostream>
#include "Utils.h"

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

bool ExecutableFile::createSection(std::string_view name, uint32_t virtualSize, uint32_t characteristics,
	ImageSection** outSection)
{
	const AddressPair addressPair = getNextAddress();
	ImageSection* section = nullptr;

	if (!_createSection(name, addressPair.virtualAddress, virtualSize, addressPair.rawAddress,
	                    Binder::alignTo(virtualSize, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment)),
	                    characteristics, &section))
	{
		return false;
	}

	*outSection = section;
	return true;
}

AddressPair ExecutableFile::getNextAddress() const
{
	uint32_t virtualAddress = 0;
	uint32_t rawAddress = 0;

	for (const auto& section : m_sections)
	{
		virtualAddress = std::max<uint32_t>(virtualAddress, section.getVirtualAddress() + section.getVirtualSize());
		rawAddress = std::max<uint32_t>(rawAddress, section.getRawAddress() + section.getRawSize());
	}

	return {
		Binder::alignTo(virtualAddress, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.SectionAlignment)),
		Binder::alignTo(rawAddress, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment))
	};
}

AddressPair ExecutableFile::getFirstSectionAddress() const
{
	uint32_t virtualAddress = 0;
	uint32_t rawAddress = 0;

	for (const auto& section : m_sections)
	{
		virtualAddress = std::min<uint32_t>(virtualAddress, section.getVirtualAddress());
		rawAddress = std::min<uint32_t>(rawAddress, section.getRawAddress());
	}

	return {
		Binder::alignTo(virtualAddress, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.SectionAlignment)),
		Binder::alignTo(rawAddress, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment))
	};
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
	const auto importDirectory = m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (importDirectory.Size == 0)
	{
		return;
	}

	const auto importSection = getSection(importDirectory.VirtualAddress);
	if (importSection == nullptr)
	{
		throw std::runtime_error("Invalid import section");
	}

	auto importDescriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(m_data.data() + importSection->vaToFo(
		importDirectory.VirtualAddress));

	while (importDescriptor->Name != 0)
	{
		auto moduleName = reinterpret_cast<char*>(m_data.data() + importSection->vaToFo(importDescriptor->Name));
		m_imports.emplace_back(moduleName);

		const auto thunkOffset = importDescriptor->OriginalFirstThunk != 0
			                         ? importDescriptor->OriginalFirstThunk
			                         : importDescriptor->FirstThunk;

		auto thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(m_data.data() + importSection->vaToFo(thunkOffset));
		while (thunk->u1.AddressOfData != 0)
		{
			if (thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
			{
				// TODO: Handle ordinal imports
				

				++thunk;
				continue;
			}

			const auto importName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(m_data.data() + importSection->vaToFo(
				static_cast<uint32_t>(thunk->u1.AddressOfData)));
			m_imports.back().addFunction(importName->Name, true);

			++thunk;
		}

		++importDescriptor;
	}
}

void ExecutableFile::parseExports()
{
	
}

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

bool ExecutableFile::_createSection(std::string_view name, uint32_t virtualAddress, uint32_t virtualSize, uint32_t rawAddress, uint32_t rawSize, uint32_t characteristics, ImageSection** outSection)
{
	if (getSection(virtualAddress) != nullptr)
	{
		// Section already exists
		return false;
	}

	if (getSection(name) != nullptr)
	{
		// Section already exists
		return false;
	}

	if (name.size() > 7)
	{
		// Name can only be 7 characters long
		return false;
	}

	rawSize = Binder::alignTo(rawSize, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment));

	m_ntHeaders.FileHeader.NumberOfSections++;

	m_ntHeaders.OptionalHeader.SizeOfHeaders = Binder::alignTo(
		m_ntHeaders.OptionalHeader.SizeOfHeaders + sizeof(IMAGE_SECTION_HEADER),
		static_cast<uint64_t>(m_ntHeaders.OptionalHeader.FileAlignment)); // wrong?

	m_ntHeaders.OptionalHeader.SizeOfImage = Binder::alignTo(
		m_ntHeaders.OptionalHeader.SizeOfImage + virtualSize + sizeof(IMAGE_SECTION_HEADER),
		static_cast<uint64_t>(m_ntHeaders.OptionalHeader.SectionAlignment));

	*outSection = &m_sections.emplace_back(ImageSection{
		name, virtualAddress, virtualSize, rawAddress, rawSize, characteristics, {}
	});
	return true;
}
