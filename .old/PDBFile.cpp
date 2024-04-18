#include "PDBFile.h"
#include "utils.h"
#include "ExecutableFile.h"
#include <DbgHelp.h>
#include <map>
#include <comutil.h>
#include <comdef.h>
#include <fstream>
#include <atlbase.h>
#include <iostream>
#include <array>
#include <regex>


static int EXE_COUNT = 1;

const std::regex nameRegex = std::regex("[^a-zA-Z0-9_]");

PDBFile::PDBFile(ExecutableFile* executableFile, std::string_view path) :
	m_executableFile(executableFile)
{
	std::cout << "\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n";
	std::cout << "PDBFile: " << EXE_COUNT++ << "\n";
	std::cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n";

	initializeCOM();
	loadDataFromPDB(path);
	openSession();
	getGlobalScope();
	processSymbols();
}

void PDBFile::initializeCOM()
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to initialize COM");
	}
}

void PDBFile::loadDataFromPDB(std::string_view path)
{
	HRESULT hr = CoCreateInstance(CLSID_DiaSource,
	                              nullptr,
	                              CLSCTX_INPROC_SERVER,
	                              __uuidof(IDiaDataSource),
	                              (void**)&m_pSource);

	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create DIA source");
	}

	hr = m_pSource->loadDataFromPdb(Binder::convertToWChar(path.data()).c_str());

	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to load PDB file");
	}
}

void PDBFile::openSession()
{
	HRESULT hr = m_pSource->openSession(&m_pSession);

	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to open session");
	}
}

void PDBFile::getGlobalScope()
{
	HRESULT hr = m_pSession->get_globalScope(&m_pGlobal);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to get global scope");
	}
}

void PDBFile::processSymbols()
{
	constexpr std::array symEnums{SymTagFunction, SymTagData, SymTagLabel, SymTagPublicSymbol};

	for (const auto symEnum : symEnums)
	{
		CComPtr<IDiaEnumSymbols> pEnumSymbols;
		HRESULT hr = m_pGlobal->findChildren(symEnum, nullptr, nsNone, &pEnumSymbols);
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to find children");
		}

		ULONG celt = 0;
		CComPtr<IDiaSymbol> pSymbol;
		while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1)
		{
			processSymbol(pSymbol);
			pSymbol.Release();
			celt = 0;
		}
	}
}

void PDBFile::processSymbol(const CComPtr<IDiaSymbol>& pSymbol)
{
	DWORD rva;
	pSymbol->get_relativeVirtualAddress(&rva);

	BSTR name;
	pSymbol->get_name(&name);


	// Undecorate the symbol name
	wchar_t m_symbolBuffer[1024];
	UnDecorateSymbolNameW(name, m_symbolBuffer, std::size<wchar_t>(m_symbolBuffer), UNDNAME_NAME_ONLY);

	std::string symbolName = Binder::convertToChar(m_symbolBuffer);
	std::string filteredName = std::regex_replace(symbolName, nameRegex, "") + "_" + std::to_string(
		m_executableFile->getHash() * rva); // Add hash to avoid name collisions


	uint64_t len = getLength(pSymbol);
	DWORD type = 0;
	//pSymbol->get_length(&len);

	// Get the type of the symbol
	DWORD symTag;
	pSymbol->get_symTag(&symTag);

	// things can be public symbol, how does this affect it...?

	// look into get_function/get_code.
	// table of enums


	std::cout << "Symbol: " << filteredName << " (0x" << std::hex << rva << ")" << std::dec << " (len: 0x" << std::hex
		<< len << std::dec << ")" << " (type: " << symTag << ")" << "\n";

	BOOL isFunction = false;

	switch (symTag)
	{
	case SymTagFunction:
		m_symbols[rva] = {filteredName, len, SymbolType::Function};
		break;
	case SymTagData:
		m_symbols[rva] = {filteredName, len, SymbolType::Data};
		break;
	case SymTagPublicSymbol:
		if (m_symbols.contains(rva))
		{
			m_symbols[rva].setPublic(true);
		}
		else
		{
			// It may be a function or code...?
			pSymbol->get_function(&isFunction);
			if (isFunction)
			{
				m_symbols[rva] = {filteredName, len, SymbolType::Function, true};
			}
			else
			{
				m_symbols[rva] = {filteredName, len, SymbolType::Data, true};
			}
		}
		break;
	case SymTagLabel:
		m_symbols[rva] = {filteredName, len, SymbolType::Label};
		break;
	case SymTagCompiland:
		break;
	default:
		break;
	}

	SysFreeString(name);
}

size_t PDBFile::getLength(const CComPtr<IDiaSymbol>& pSymbol)
{
	DWORD symTag;
	pSymbol->get_symTag(&symTag);
	uint64_t len = 0;

	if (symTag == SymTagData)
	{
		IDiaSymbol* pType;

		if (SUCCEEDED(pSymbol->get_type(&pType)))
		{
			pType->get_length(&len);
		}
		else
		{
			if (FAILED(pSymbol->get_length(&len)))
			{
				len = 0;
			}
		}

		pType->Release();
	}
	else
	{
		if (FAILED(pSymbol->get_length(&len)))
		{
			len = 0;
		}
	}

	return len;
}

bool PDBFile::getSymbol(uint64_t address, Symbol* outSymbol) const
{
	if (m_symbols.contains(address))
	{
		*outSymbol = m_symbols.at(address);
		return true;
	}
	return false;
}
