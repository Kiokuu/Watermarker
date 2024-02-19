#pragma once
#include <string_view>
#include <map>
#include <dia2.h>
#include <DbgHelp.h>
#include <atlbase.h>
#include <unordered_map>

// Forward declare executable file
class ExecutableFile;


enum class SymbolType
{
	Function,
	Variable,
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

	void setPublic(bool isPublic) { m_isPublic = isPublic; }
	void setLength(size_t length) { m_length = length; }

private:
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

	//~PDBFile();

	void initializeCOM();
	void loadDataFromPDB(std::string_view path);
	void openSession();
	void getGlobalScope();
	void processSymbols();

	void processSymbol(const CComPtr<IDiaSymbol>& pSymbol);



	DWORD getType(const CComPtr<IDiaSymbol>& pSymbol);
	size_t getLength(const CComPtr<IDiaSymbol>& pSymbol);


	bool getSymbol(uint64_t address, Symbol* outSymbol) const;
	const std::unordered_map<uint64_t, Symbol>& getSymbols() const { return m_symbols; }


	ExecutableFile* getExecutableFile() const { return m_executableFile; }

private:
	ExecutableFile* m_executableFile;
	std::unordered_map<uint64_t, Symbol> m_symbols;


	CComPtr<IDiaDataSource> m_pSource;
	CComPtr<IDiaSession> m_pSession;
	CComPtr<IDiaSymbol> m_pGlobal;

};
