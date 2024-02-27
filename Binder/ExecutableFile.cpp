#include "ExecutableFile.h"
#include <fstream>
#include <iostream>
#include <functional>
#include "Utils.h"

ExecutableFile::ExecutableFile(std::string_view executablePath) : m_rewriteImportsOnSave(false)
{
	// Open the executable file
	std::ifstream file(executablePath.data(), std::ios::binary);

	if (!file.is_open())
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

ExecutableFile::ExecutableFile(std::string_view executablePath, std::string_view pdbPath)
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

	m_pdbFile = PDBFile(this, pdbPath);
}


void ExecutableFile::save(std::string_view savePath)
{
	if (m_rewriteImportsOnSave)
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


	HANDLE saveFile = CreateFileA(savePath.data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
	                              nullptr);
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


		std::cout << "Virtual Address: 0x" << std::hex << section.getVirtualAddress() << std::dec << "\n";
		std::cout << "Virtual Size: 0x" << std::hex << section.getVirtualSize() << std::dec << "\n";

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
		if(totalBytesWritten < section.getRawAddress())
		{
			std::vector<uint8_t> padding(section.getRawAddress() - totalBytesWritten);
			std::fill(padding.begin(), padding.end(), 0);
			WriteFile(saveFile, padding.data(), padding.size(), &bytesWritten, nullptr);
			totalBytesWritten += bytesWritten;
		}

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

	// We can create a new import directory and section to hold the new import data. We can copy the import directory entries, and just write a new directory entry with the solely new imports.
	// We also will want to add the original import addresses to the addressmap to be used later.

	uint32_t iatSize = 0;
	uint32_t hintTableSize = 0;
	for(const auto& module : m_imports)
	{
		iatSize += sizeof(IMAGE_THUNK_DATA); // Null entry per module, need a way to identify original modules.
		// if its not an original function, we need to add size
		for(const auto& function : module.getFunctions())
		{
			if(!function.getOriginal())
			{
				iatSize += sizeof(IMAGE_THUNK_DATA);

				hintTableSize += sizeof(WORD); // Hint
				hintTableSize += static_cast<uint32_t>(function.getName().size() + 1); // Name

				if ((function.getName().size() + 1) % 2 != 0)
				{
					++hintTableSize;
				}
			}
		}
		hintTableSize += static_cast<uint32_t>(module.getName().size() + 1); // DllName
	}

	iatSize = Binder::alignTo(iatSize, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment));
	uint32_t iltSize = iatSize;

	// Null entry + 1 entry per module
	uint32_t idtSize = sizeof(IMAGE_IMPORT_DESCRIPTOR) + (sizeof(IMAGE_IMPORT_DESCRIPTOR) * static_cast<uint32_t>(m_imports.size()));

	hintTableSize = Binder::alignTo(hintTableSize, static_cast<uint32_t>(m_ntHeaders.OptionalHeader.FileAlignment));

	uint32_t importDataSize = iatSize + idtSize + iltSize + hintTableSize;


	
	// Create the new import section. Just need read write
	const AddressPair importAddressPair = getNextAddress();
	ImageSection* newImportSection = nullptr;

	if (!_createSection(".newimp", importAddressPair.virtualAddress, importDataSize, importAddressPair.rawAddress,
	                    importDataSize, IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE,
	                    &newImportSection))
	{
		throw std::runtime_error("Failed to create new import section");
	}

	m_newImportData.resize(importDataSize);

	uint32_t iatOffset = 0;
	uint32_t idtOffset = iatSize;
	uint32_t iltOffset = iatSize + idtSize;
	uint32_t hintTableOffset = iatSize + idtSize + iltSize;

	std::span iatData(m_newImportData.data(), iatSize);
	std::span idtData(m_newImportData.data() + idtOffset, idtSize);
	std::span iltData(m_newImportData.data() + iltOffset, iltSize);
	std::span hintTableData(m_newImportData.data() + hintTableOffset, hintTableSize);




	// We need to copy the original import directory entries, and while we're at it, capture the original import addresses. (todo: move to fetching part)
	// todo: also make one if it doesnt exist.
	auto importDirectory = m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (importDirectory.Size == 0)
	{
		return;
	}

	std::vector<IMAGE_IMPORT_DESCRIPTOR*> originalImportDescriptors;

	auto importSection = getSection(importDirectory.VirtualAddress);
	auto importDescriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(m_data.data() + importSection->vaToFo(
		importDirectory.VirtualAddress));

	uint32_t count = 0;

	while (importDescriptor->Name != 0)
	{
		originalImportDescriptors.push_back(importDescriptor);
		std::string moduleName = reinterpret_cast<char*>(m_data.data() + importSection->vaToFo(importDescriptor->Name));
		uint32_t thunkOffset = importDescriptor->FirstThunk;

		auto thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(m_data.data() + importSection->vaToFo(thunkOffset));

		while (thunk->u1.AddressOfData != 0)
		{
			if (thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
			{
				m_addressMap[moduleName + "." + std::to_string(thunk->u1.Ordinal)] = thunk->u1.AddressOfData; // Save the original import address
				++count;
				++thunk;
				continue;
			}

			std::string importName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(m_data.data() + importSection->vaToFo(
				static_cast<uint32_t>(thunk->u1.AddressOfData)))->Name;

			m_addressMap[moduleName + "." + importName] = thunk->u1.AddressOfData; // Save the original import address

			++count;
			++thunk;
		}
		++importDescriptor;
	}


	std::vector <IMAGE_IMPORT_DESCRIPTOR> newImportDescriptors;

	uint32_t moduleCount = 0;
	uint32_t importCount = 0;
	uint32_t hintNameTableOffset = 0;

	// We now need to write the new import directories and import addresses.
	for(const auto& importedModule : m_imports)
	{
		bool notOriginalModule = false;
		uint32_t notOriginalFunctions = 0;

		for(const auto& function : importedModule.getFunctions())
		{
			if(!function.getOriginal())
			{
				notOriginalModule = true;
				notOriginalFunctions += 1;
				IMAGE_THUNK_DATA thunkData = {};

				if(!function.getIsOrdinal())
				{
					thunkData.u1.AddressOfData = importAddressPair.virtualAddress + hintTableOffset + hintNameTableOffset;

					memcpy(iatData.data() + importCount *  sizeof(IMAGE_THUNK_DATA), &thunkData, sizeof(thunkData));
					memcpy(iltData.data() + importCount *  sizeof(IMAGE_THUNK_DATA), &thunkData, sizeof(thunkData));

					m_addressMap[std::string(importedModule.getName()) + "." + std::string(function.getName())] = importAddressPair.virtualAddress + importCount * sizeof(IMAGE_THUNK_DATA);

					uint16_t hint = 0;
					memcpy(hintTableData.data() + hintNameTableOffset, &hint, sizeof(hint));
					hintNameTableOffset += sizeof(hint);

					memcpy(hintTableData.data() + hintNameTableOffset, function.getName().data(), function.getName().size());
					hintNameTableOffset += static_cast<uint32_t>(function.getName().size() + 1);

					if ((function.getName().size() + 1) % 2 != 0)
					{
						++hintNameTableOffset;
					}
				}
				else
				{
					thunkData.u1.Ordinal = function.getOrdinal();

					memcpy(iatData.data() + importCount *  sizeof(IMAGE_THUNK_DATA), &thunkData, sizeof(thunkData));
					memcpy(iltData.data() + importCount *  sizeof(IMAGE_THUNK_DATA), &thunkData, sizeof(thunkData));
					
					m_addressMap[std::string(importedModule.getName()) + "." + std::to_string(function.getOrdinal())] = importAddressPair.virtualAddress + importCount * sizeof(IMAGE_THUNK_DATA);
				}
				++importCount;
			}
		}

		if(notOriginalModule)
		{
			IMAGE_THUNK_DATA nullThunkData = {0};
			memcpy(iatData.data() + importCount *  sizeof(IMAGE_THUNK_DATA), &nullThunkData, sizeof(nullThunkData));
			memcpy(iltData.data() + importCount *  sizeof(IMAGE_THUNK_DATA), &nullThunkData, sizeof(nullThunkData));
			++importCount;

			IMAGE_IMPORT_DESCRIPTOR importDescriptor = {};

			importDescriptor.Name = importAddressPair.virtualAddress + hintTableOffset + hintNameTableOffset;

			// calculate using the module count and import count
			
			// TODO: these are probably wrong
			importDescriptor.OriginalFirstThunk = importAddressPair.virtualAddress + iltOffset + (importCount - notOriginalFunctions) * sizeof(IMAGE_THUNK_DATA) - sizeof(IMAGE_THUNK_DATA);
			importDescriptor.FirstThunk = importAddressPair.virtualAddress + ((importCount - notOriginalFunctions) * sizeof(IMAGE_THUNK_DATA)) - sizeof(IMAGE_THUNK_DATA);

			newImportDescriptors.push_back(importDescriptor);

			memcpy(hintTableData.data() + hintNameTableOffset, importedModule.getName().data(), importedModule.getName().size());
			hintNameTableOffset += static_cast<uint32_t>(importedModule.getName().size() + 1);

			moduleCount++;
		}
	}

	IMAGE_IMPORT_DESCRIPTOR nullImportDescriptor = {0};
	newImportDescriptors.push_back(nullImportDescriptor);


	// Need to write the new import descriptors, starting with the original ones. Dereference the pointers.
	//memcpy(idtData.data(), originalImportDescriptors.data(), originalImportDescriptors.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR));

	int test = 0;
	for(const auto& importDescriptor : originalImportDescriptors)
	{
		IMAGE_IMPORT_DESCRIPTOR newImportDescriptor = *importDescriptor;
		memcpy(idtData.data() + test * sizeof(IMAGE_IMPORT_DESCRIPTOR), &newImportDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR));
		test++;
	}


	// Write the new import descriptors
	memcpy(idtData.data() + originalImportDescriptors.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR), newImportDescriptors.data(), newImportDescriptors.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR));


	newImportSection->setData(m_newImportData);

	m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = newImportSection->getVirtualAddress() + idtOffset;
	m_ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = originalImportDescriptors.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR) + newImportDescriptors.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR);


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

const ImageSection* ExecutableFile::getSection(std::string_view name)
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

const ImageSection* ExecutableFile::getSection(uint32_t virtualAddress)
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

bool ExecutableFile::_createSection(std::string_view name, uint32_t virtualAddress, uint32_t virtualSize,
                                    uint32_t rawAddress, uint32_t rawSize, uint32_t characteristics,
                                    ImageSection** outSection)
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
