#pragma once
#include <string_view>
#include <vector>
#define NOMINMAX // http://www.suodenjoki.dk/us/archive/2010/min-max.htm 
#define WIN32_LEAN_AND_MEAN
#include <unordered_map>

#include "ImageSection.h"
#include "Windows.h"
#include "ImportedModule.h"
#include "ExportedModule.h"


/**
 * Notes:
 * PE Format: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 * Dos Header -> NT Headers -> Section Headers -> Import Info -> Export Info -> ... -> Data
 */

/**
 * @brief Represents a pair of addresses: virtual and raw.
 */
struct AddressPair
{
	uint32_t virtualAddress; /**< Virtual address */
	uint32_t rawAddress; /**< Raw address */
};

/**
 * @brief Represents an executable file.
 */
class ExecutableFile
{
public:
	/**
	 * @brief Default constructor.
	 */
	explicit ExecutableFile() = default;

	/**
	 * @brief Constructor with executable path.
	 * @param executablePath Path to the executable file.
	 */
	explicit ExecutableFile(std::string_view executablePath);

	/**
	 * @brief Constructor with executable and PDB paths.
	 * @param executablePath Path to the executable file.
	 * @param pdbPath Path to the PDB file.
	 */
	explicit ExecutableFile(std::string_view executablePath, std::string_view pdbPath);

	/**
	 * @brief Destructor.
	 */
	~ExecutableFile() = default;

	/**
	 * @brief Saves the executable file to the specified path.
	 * @param savePath Path to save the executable file.
	 */
	void save(std::string_view savePath);

	/**
	 * @brief Creates a new section in the executable.
	 * @param name Name of the section.
	 * @param virtualSize Virtual size of the section.
	 * @param characteristics Characteristics of the section.
	 * @param outSection Pointer to store the created section.
	 * @return True if section creation is successful, false otherwise.
	 */
	bool createSection(std::string_view name, uint32_t virtualSize, uint32_t characteristics,
	                   ImageSection** outSection);

	/**
	 * @brief Adds an import to the executable.
	 * @param moduleName Name of the module to import.
	 * @param functionName Name of the function to import.
	 */
	void addImport(std::string moduleName, std::string functionName);

	/**
	 * @brief Rewrites the import table in the executable with the new import data.
	 */
	void rewriteImports();

	/**
	 * @brief Gets the image base of the executable.
	 * @return The image base.
	 */
	uint64_t getImageBase() const { return m_ntHeaders.OptionalHeader.ImageBase; }

	/**
	 * @brief Gets the hash of the executable.
	 * @return The hash value.
	 */
	uint64_t getHash() const { return m_data.size(); }

	/**
	 * @brief Gets a pointer to the PDB file.
	 * @return Pointer to the PDB file.
	 */
	//PDBFile* getPDBFile() { return &m_pdbFile; }

	/**
	 * @brief Gets the address of an imported function.
	 * @param moduleName Name of the imported module.
	 * @param functionName Name of the imported function.
	 * @return The address pair containing virtual and raw addresses.
	 */
	uint32_t getImportAddress(std::string_view moduleName, std::string_view functionName);

	/**
	 * @brief Gets the address of the first section in the executable.
	 * @return The address pair containing virtual and raw addresses.
	 */
	AddressPair getFirstSectionAddress() const;

	/**
	 * @brief Sets the entry point of the executable.
	 * @param entryPoint The entry point address.
	 */
	void setEntryPoint(uint32_t entryPoint) { m_ntHeaders.OptionalHeader.AddressOfEntryPoint = entryPoint; }

	/**
	 * @brief Gets the entry point of the executable.
	 * @return The entry point address.
	 */
	uint32_t getEntryPoint() const { return m_ntHeaders.OptionalHeader.AddressOfEntryPoint; }

	/**
	 * @brief Gets the data of the executable.
	 * @return Reference to the data vector.
	 */
	const std::vector<uint8_t>& getData() const { return m_data; }

	/**
	 * @brief Gets the sections of the executable.
	 * @return Reference to the sections vector.
	 */
	const std::vector<ImageSection>& getSections() const { return m_sections; }

	/**
	 * @brief Gets the imports of the executable.
	 * @return Reference to the imports vector.
	 */
	const std::vector<ImportedModule>& getImports() const { return m_imports; }

	/**
	 * @brief Gets a section by name.
	 * @param name Name of the section.
	 * @return Pointer to the found section.
	 */
	const ImageSection* getSection(std::string_view name);

	/**
	 * @brief Gets a section by virtual address.
	 * @param virtualAddress Virtual address of the section.
	 * @return Pointer to the found section.
	 */
	const ImageSection* getSection(uint32_t virtualAddress);

private:
	/**
	 * @brief Gets the next address in the executable.
	 * @return The next address pair.
	 */
	AddressPair getNextAddress() const;

	/**
	 * @brief Parses the executable file.
	 */
	void parse();

	/**
	 * @brief Parses the DOS header of the executable.
	 */
	void parseDosHeader();

	/**
	 * @brief Parses the NT headers of the executable.
	 */
	void parseNtHeaders();

	/**
	 * @brief Parses the sections of the executable.
	 */
	void parseSections();

	/**
	 * @brief Parses the import information of the executable.
	 */
	void parseImports();

	/**
	 * @brief Parses the export information of the executable.
	 */
	void parseExports();

	/**
	 * @brief Creates a section in the executable.
	 * @param name Name of the section.
	 * @param virtualAddress Virtual address of the section.
	 * @param virtualSize Virtual size of the section.
	 * @param rawAddress Raw address of the section.
	 * @param rawSize Raw size of the section.
	 * @param characteristics Characteristics of the section.
	 * @param outSection Pointer to store the created section.
	 * @return True if section creation is successful, false otherwise.
	 */
	bool _createSection(std::string_view name, uint32_t virtualAddress, uint32_t virtualSize, uint32_t rawAddress,
	                    uint32_t rawSize, uint32_t characteristics, ImageSection** outSection);

	std::vector<uint8_t> m_data; /**< Raw data of the executable. */
	std::vector<ImageSection> m_sections; /**< Sections of the executable. */
	std::vector<ImportedModule> m_imports; /**< Imported modules of the executable. */
	std::vector<ExportedModule> m_exports; /**< Exported modules of the executable. */
	std::vector<uint8_t> m_newImportData; /**< New import data of the executable. */
	std::vector<uint8_t> m_trampolineData; /**< Trampoline data of the executable. */
	std::unordered_map<std::string, uintptr_t> m_addressMap = {}; /**< Address map of the executable. */

	IMAGE_DOS_HEADER m_dosHeader; /**< DOS header of the executable. */
	IMAGE_NT_HEADERS m_ntHeaders; /**< NT headers of the executable. */
	//PDBFile m_pdbFile; /**< PDB file associated with the executable. */

	bool m_rewriteImportsOnSave; /**< Flag indicating whether to rewrite imports on save. */
};
