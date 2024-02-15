#include "ExecutableFile.h"
#include <fstream>
#include <iostream>
#include <functional>
#include "Utils.h"

ExecutableFile::ExecutableFile(std::string_view executablePath) : m_rewriteImportsOnSave(false)
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

ExecutableFile::ExecutableFile(std::string_view executablePath, std::string_view pdbPath) : m_pdbFile(this, pdbPath)
{
	std::ifstream file(executablePath.data(), std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file");
	}

	file.seekg(0, std::ios::end);
	m_data.resize(file.tellg());
	file.seekg(0, std::ios::beg);

	file.read(reinterpret_cast<char*>(m_data.data()), m_data.size());

	parse();

	file.close();
}


void ExecutableFile::save(std::string_view savePath)
{
	if(m_rewriteImportsOnSave)
		rewriteImports();

	// Recalculate header size and image size.
	uint32_t sizeOfHeaders = m_dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + m_ntHeaders.FileHeader.
		SizeOfOptionalHeader + (sizeof(IMAGE_SECTION_HEADER) * m_sections.size());

	sizeOfHeaders = Binder::alignTo(sizeOfHeaders, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment));

	const auto max_virtual_address = std::ranges::max_element(m_sections,
	  [](const ImageSection& a, const ImageSection& b)
	  {
	      return a.getVirtualAddress() < b.getVirtualAddress();
	  });

	uint32_t sizeOfImage = max_virtual_address->getVirtualAddress() + max_virtual_address->getVirtualSize();
	sizeOfImage = Binder::alignTo(sizeOfImage, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.SectionAlignment));

	m_ntHeaders.OptionalHeader.SizeOfHeaders = sizeOfHeaders;
	m_ntHeaders.OptionalHeader.SizeOfImage = sizeOfImage;
	m_ntHeaders.FileHeader.NumberOfSections = m_sections.size();


	HANDLE saveFile = CreateFileA(savePath.data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (saveFile == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create save file");
	}

	DWORD totalBytesWritten = 0;
	DWORD bytesWritten = 0;

	
	// Write dos header
	WriteFile(saveFile, &m_dosHeader, sizeof(m_dosHeader), &bytesWritten, nullptr);
	totalBytesWritten += bytesWritten;

	if (bytesWritten < m_dosHeader.e_lfanew)
	{
		std::vector<uint8_t> padding(m_dosHeader.e_lfanew - bytesWritten);
		std::fill(padding.begin(), padding.end(), 0);
		WriteFile(saveFile, padding.data(), padding.size(), &bytesWritten, nullptr);
		totalBytesWritten += bytesWritten;
	}

	// Write nt headers
	WriteFile(saveFile, &m_ntHeaders, sizeof(m_ntHeaders), &bytesWritten, nullptr);
	totalBytesWritten += bytesWritten;

	// Write section headers
	for (const auto& section : m_sections)
	{
		IMAGE_SECTION_HEADER sectionHeader = {};
		sectionHeader.VirtualAddress = section.getVirtualAddress();
		sectionHeader.Misc.VirtualSize = section.getVirtualSize();
		sectionHeader.PointerToRawData = section.getRawAddress();
		sectionHeader.SizeOfRawData = section.getRawSize();
		sectionHeader.Characteristics = section.getCharacteristics();

		std::cout << "Writing section: " << section.getName() << "\n";
		std::cout << "Name Size: " << section.getName().size() << "\n";

		// For some reason name includes a null terminator and garbage data.
		memcpy(sectionHeader.Name, section.getName().data(),
		       std::min<size_t>(section.getName().size(), sizeof(sectionHeader.Name)));


		std::cout << "Virtual Address: " << section.getVirtualAddress() << "\n";
		std::cout << "Virtual Size: " << section.getVirtualSize() << "\n";

		WriteFile(saveFile, &sectionHeader, sizeof(sectionHeader), &bytesWritten, nullptr);
		totalBytesWritten += bytesWritten;
	}

	// Pad until file alignment
	const uint32_t nearestFA = Binder::alignTo(totalBytesWritten, m_ntHeaders.OptionalHeader.FileAlignment);
	const uint32_t distanceFA = nearestFA - totalBytesWritten;

	if (distanceFA > 0)
	{
		std::vector<uint8_t> padding(distanceFA);
		std::fill(padding.begin(), padding.end(), 0);
		WriteFile(saveFile, padding.data(), padding.size(), &bytesWritten, nullptr);
		totalBytesWritten += bytesWritten;
	}

	// Write section data
	for (const auto& section : m_sections)
	{
		WriteFile(saveFile, section.getData().data(), section.getData().size(), &bytesWritten, nullptr);
		totalBytesWritten += bytesWritten;

		if (bytesWritten < section.getRawSize())
		{
			std::vector<uint8_t> padding(section.getRawSize() - bytesWritten);
			std::fill(padding.begin(), padding.end(), 0);
			WriteFile(saveFile, padding.data(), padding.size(), &bytesWritten, nullptr);
			totalBytesWritten += bytesWritten;
		}
	}

	CloseHandle(saveFile);
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

void ExecutableFile::addImport(std::string moduleName, std::string functionName)
{
	for (auto& importedModule : m_imports)
	{
		if (_stricmp(importedModule.getName().data(), moduleName.data()) == 0)
		{
			m_rewriteImportsOnSave = true;
			importedModule.addFunction(functionName, false); // Handles checking for duplicates
			return;
		}
	}

	m_rewriteImportsOnSave = true;
	auto& module = m_imports.emplace_back(moduleName);
	module.addFunction(functionName, false);
}

void ExecutableFile::rewriteImports()
{
	m_rewriteImportsOnSave = false;

	std::cout << "Rewriting imports" << "\n";

	uint32_t iatSize = 0;

	for (const auto& module : m_imports)
	{
		iatSize += sizeof(IMAGE_THUNK_DATA) + sizeof(IMAGE_THUNK_DATA) * static_cast<uint32_t>(module.getFunctions().
			size());
	}
	iatSize = Binder::alignTo(iatSize, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment));

	// IDT Size Prediction
	uint32_t idtSize = sizeof(IMAGE_IMPORT_DESCRIPTOR) + (sizeof(IMAGE_IMPORT_DESCRIPTOR) * static_cast<uint32_t>(
		m_imports.size()));
	uint32_t iltSize = iatSize;

	uint32_t hintTableSize = 0;
	for (const auto& module : m_imports)
	{
		for (const auto& function : module.getFunctions())
		{
			hintTableSize += sizeof(WORD); // Hint
			hintTableSize += static_cast<uint32_t>(function.getName().size() + 1); // Name

			if ((function.getName().size() + 1) % 2 != 0)
			{
				++hintTableSize;
			}
		}
		hintTableSize += static_cast<uint32_t>(module.getName().size() + 1); // DllName
	}
	hintTableSize = Binder::alignTo(hintTableSize, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment));


	const uint32_t importDataSize = iatSize + idtSize + iltSize + hintTableSize;

	// Creation of new import section
	const AddressPair importAddressPair = getNextAddress();
	ImageSection* newImportSection = nullptr;

	if (!_createSection(".newimp", importAddressPair.virtualAddress, importDataSize, importAddressPair.rawAddress,
	                    importDataSize, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA,
	                    &newImportSection))
	{
		throw std::runtime_error("Failed to create new import section");
	}

	// Define the trampoline section, fill later
	constexpr size_t trampolineCodeSize = 12;
	size_t trampolinesize = 0;

	for (const auto& module : m_imports)
	{
		for (const auto& function : module.getFunctions())
		{
			trampolinesize += trampolineCodeSize;
		}
	}

	const AddressPair trampolineAddressPair = getNextAddress();

	ImageSection* trampolineSection = nullptr;
	if (!_createSection(".itram", trampolineAddressPair.virtualAddress, trampolinesize,
	                    trampolineAddressPair.rawAddress, trampolinesize,
	                    IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA |
	                    IMAGE_SCN_CNT_CODE, &trampolineSection))
	{
		throw std::runtime_error("Failed to create trampoline section");
	}


	m_newImportData.resize(importDataSize);

	std::span iatData(m_newImportData.data(), iatSize);
	std::span idtData(m_newImportData.data() + iatSize, idtSize);
	std::span iltData(m_newImportData.data() + iatSize + idtSize, iltSize);
	std::span hintTableData(m_newImportData.data() + iatSize + idtSize + iltSize, hintTableSize);

	uint32_t moduleCount = 0;
	uint32_t importCount = 0;
	uint32_t hintNameTableOffset = 0;

	for (const auto& module : m_imports)
	{
		for (const auto& function : module.getFunctions())
		{
			IMAGE_THUNK_DATA thunkData = {};
			//thunkData.u1.AddressOfData = importAddressPair.virtualAddress + (iatSize + idtSize + iltSize + hintNameTableOffset);

			if(!function.getIsOrdinal())
			{
				thunkData.u1.AddressOfData = importAddressPair.virtualAddress + iatSize + idtSize + iltSize +
				hintNameTableOffset;

				memcpy(iatData.data() + importCount * sizeof(IMAGE_THUNK_DATA), &thunkData, sizeof(thunkData));
				memcpy(iltData.data() + importCount * sizeof(IMAGE_THUNK_DATA), &thunkData, sizeof(thunkData));

				m_addressMap[std::string(module.getName()) + "." + std::string(function.getName())] = importAddressPair.
					virtualAddress + importCount * sizeof(IMAGE_THUNK_DATA);


				uint16_t hint = 0;
				memcpy(hintTableData.data() + hintNameTableOffset, &hint, sizeof(hint));
				hintNameTableOffset += sizeof(hint);

				memcpy(hintTableData.data() + hintNameTableOffset, function.getName().data(), function.getName().size());
				hintNameTableOffset += function.getName().size() + 1; // null terminator

				// align on even boundary
				// if name length + 1 is odd, add 1
				if ((function.getName().size() + 1) % 2 != 0)
				{
					++hintNameTableOffset;
				}
			}
			else
			{
				thunkData.u1.Ordinal = function.getOrdinal();
				memcpy(iatData.data() + importCount * sizeof(IMAGE_THUNK_DATA), &thunkData, sizeof(thunkData));
				memcpy(iltData.data() + importCount * sizeof(IMAGE_THUNK_DATA), &thunkData, sizeof(thunkData));

				m_addressMap[std::string(module.getName()) + "." + std::to_string(function.getOrdinal())] = importAddressPair.
					virtualAddress + importCount * sizeof(IMAGE_THUNK_DATA);
			}

			//hintnametable?

			++importCount;
		}

		IMAGE_THUNK_DATA nullThunkData = {0};
		memcpy(iatData.data() + importCount * sizeof(IMAGE_THUNK_DATA), &nullThunkData, sizeof(nullThunkData));
		memcpy(iltData.data() + importCount * sizeof(IMAGE_THUNK_DATA), &nullThunkData, sizeof(nullThunkData));
		++importCount;

		IMAGE_IMPORT_DESCRIPTOR importDescriptor = {};

		importDescriptor.Name = importAddressPair.virtualAddress + (iatSize + idtSize + iltSize + hintNameTableOffset);

		importDescriptor.OriginalFirstThunk = importAddressPair.virtualAddress + iatSize + idtSize + ((importCount -
			static_cast<uint32_t>(module.getFunctions().size())) * sizeof(IMAGE_THUNK_DATA)) - sizeof(IMAGE_THUNK_DATA);
		//importDescriptor.OriginalFirstThunk = importAddressPair.virtualAddress + ((importCount - static_cast<uint32_t>(module.getFunctions().size())) * sizeof(IMAGE_THUNK_DATA)) - sizeof(IMAGE_THUNK_DATA);

		//importDescriptor.FirstThunk = importDescriptor.OriginalFirstThunk;
		importDescriptor.FirstThunk = importAddressPair.virtualAddress + ((importCount - static_cast<uint32_t>(module.
			getFunctions().size())) * sizeof(IMAGE_THUNK_DATA)) - sizeof(IMAGE_THUNK_DATA);

		memcpy(idtData.data() + moduleCount * sizeof(IMAGE_IMPORT_DESCRIPTOR), &importDescriptor,
		       sizeof(importDescriptor));

		memcpy(hintTableData.data() + hintNameTableOffset, module.getName().data(), module.getName().size());
		hintNameTableOffset += static_cast<uint32_t>(module.getName().size() + 1);

		++moduleCount;
	}

	IMAGE_IMPORT_DESCRIPTOR nullImportDescriptor = {};
	memcpy(idtData.data() + moduleCount * sizeof(IMAGE_IMPORT_DESCRIPTOR), &nullImportDescriptor,
	       sizeof(nullImportDescriptor));
	newImportSection->setData(m_newImportData);


	auto importDirectory = m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (importDirectory.Size == 0)
	{
		return;
	}

	auto importSection = getSection(importDirectory.VirtualAddress);
	auto importDescriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(m_data.data() + importSection->vaToFo(
		importDirectory.VirtualAddress));


	uint32_t count = 0;

	while (importDescriptor->Name != 0)
	{
		std::string moduleName = reinterpret_cast<char*>(m_data.data() + importSection->vaToFo(importDescriptor->Name));
		uint32_t thunkOffset = importDescriptor->FirstThunk;

		auto thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(m_data.data() + importSection->vaToFo(thunkOffset));

		while (thunk->u1.AddressOfData != 0)
		{
			if (thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
			{
				if(m_addressMap.contains(moduleName + "." + std::to_string(thunk->u1.Ordinal)))
				{


					thunk->u1.Ordinal = m_ntHeaders.OptionalHeader.ImageBase + m_addressMap[moduleName + "." + std::to_string(thunk->u1.Ordinal)];

				}

				++count;
				++thunk;
				continue;
			}

			std::string importName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(m_data.data() + importSection->vaToFo(
				static_cast<uint32_t>(thunk->u1.AddressOfData)))->Name;

			if (m_addressMap.contains(moduleName + "." + importName))
			{
				// If it's an original function, we need to jump to the trampoline
				bool isOriginal = false;
				for (const auto& module : m_imports)
				{
					for (const auto& function : module.getFunctions())
					{
						if (module.getName() == moduleName && function.getName() == importName)
						{
							isOriginal = function.getOriginal();
							break;
						}
					}
				}

				if (isOriginal)
				{
					std::cout << "Function: " << moduleName << "." << importName << " Trampoline Address: " <<
						trampolineAddressPair.virtualAddress + (trampolineCodeSize * count) << "\n";
					thunk->u1.AddressOfData = m_ntHeaders.OptionalHeader.ImageBase + trampolineAddressPair.
						virtualAddress + (count * trampolineCodeSize);
				}
				else
				{
					thunk->u1.AddressOfData = m_ntHeaders.OptionalHeader.ImageBase + m_addressMap[moduleName + "." +
						importName];
				}
			}

			++count;
			++thunk;
		}
		++importDescriptor;
	}

	//importSection->setCharacteristics(importSection->getCharacteristics() | IMAGE_SCN_MEM_WRITE);

	m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = importAddressPair.
		virtualAddress + iatSize;
	m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = idtSize;

	m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = importAddressPair.
		virtualAddress;
	m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = iatSize;

	m_ntHeaders.OptionalHeader.DllCharacteristics &= ~IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;


	/*
		Fill the trampolines
		

		{ 0x48, 0xA1 }); // mov rax, [&newIATEntryAddress]
		{newIATEntryAddress}
		{ 0xFF, 0xE0 }); // jmp rax
	*/

	std::cout << "Writing trampoline code" << "\n";

	m_trampolineData.resize(trampolinesize);
	std::span trampolineData(m_trampolineData.data(), trampolinesize);

	constexpr uint8_t movInstruction[] = {0x48, 0xA1};
	constexpr uint8_t jmpInstruction[] = {0xFF, 0xE0};

	// Write trampoline code for each function using the address map. If its not a original function, use padding and pad to trampoline code size.
	uint32_t trampolineOffset = 0;
	for (const auto& module : m_imports)
	{
		for (const auto& function : module.getFunctions())
		{
			if (function.getOriginal())
			{
				std::cout << "Original function: " << module.getName() << "." << function.getName() << "\n";
				std::cout << "Trampoline Address: " << trampolineAddressPair.virtualAddress + trampolineOffset <<
					"\n";

				uintptr_t newIATEntryAddress = m_ntHeaders.OptionalHeader.ImageBase + m_addressMap[
					std::string(module.getName()) + "." + std::string(function.getName())];

				memcpy(trampolineData.data() + trampolineOffset, movInstruction, sizeof(movInstruction));
				memcpy(trampolineData.data() + trampolineOffset + sizeof(movInstruction), &newIATEntryAddress,
				       sizeof(newIATEntryAddress));
				memcpy(trampolineData.data() + trampolineOffset + sizeof(movInstruction) + sizeof(newIATEntryAddress),
				       jmpInstruction, sizeof(jmpInstruction));

				trampolineOffset += trampolineCodeSize;
			}
		}
	}

	trampolineSection->setData(m_trampolineData);
	std::cout << "Done" << "\n";
}

uint32_t ExecutableFile::getImportAddress(std::string_view moduleName, std::string_view functionName)
{
	// Return the address map value if it exists, otherwise return 0
	if (m_addressMap.contains(std::string(moduleName) + "." + std::string(functionName)))
	{
		return m_addressMap[std::string(moduleName) + "." + std::string(functionName)];
	}
	return 0;
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
				m_imports.back().addFunction(thunk->u1.Ordinal, true);
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
