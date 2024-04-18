#pragma once
#include <string_view>
#include <dia2.h>
#include <atlbase.h>
#include <unordered_map>

// Forward declare executable file
class ExecutableFile;


enum class SymbolType
{
	Function,
	PublicSymbol,
	Data,
	Label
};

class Symbol
{
public:
	Symbol() = default;

	Symbol(std::string_view name, size_t length, SymbolType type, bool isPublic = false)
		: m_name(name), m_length(length), m_type(type), m_isPublic(isPublic)
	{
	}

	std::string getName() const { return m_name; }
	size_t getLength() const { return m_length; }
	SymbolType getType() const { return m_type; }
	bool isPublic() const { return m_isPublic; }

	void setName(std::string_view name) { m_name = name; }
	void setLength(size_t length) { m_length = length; }
	void setType(SymbolType type) { m_type = type; }
	void setPublic(bool isPublic) { m_isPublic = isPublic; }

private:
	uint64_t m_rva;
	std::string m_name;
	size_t m_length;
	SymbolType m_type;
	bool m_isPublic;
};


class PDBFile
{
public:
	PDBFile() = default;
	PDBFile(ExecutableFile* executableFile, std::string_view path);

	void initializeCOM();
	void loadDataFromPDB(std::string_view path);
	void openSession();
	void getGlobalScope();
	void processSymbols();

	void processSymbol(const CComPtr<IDiaSymbol>& pSymbol);
	DWORD getType(const CComPtr<IDiaSymbol>& pSymbol);
	size_t getLength(const CComPtr<IDiaSymbol>& pSymbol);


	bool getSymbol(uint64_t address, Symbol* outSymbol) const;
	std::unordered_map<uint64_t, Symbol>& getSymbols() { return m_symbols; }

	ExecutableFile* getExecutableFile() const { return m_executableFile; }

private:
	ExecutableFile* m_executableFile;
	std::unordered_map<uint64_t, Symbol> m_symbols;

	CComPtr<IDiaDataSource> m_pSource;
	CComPtr<IDiaSession> m_pSession;
	CComPtr<IDiaSymbol> m_pGlobal;
};
