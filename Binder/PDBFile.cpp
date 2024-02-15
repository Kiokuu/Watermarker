#include "PDBFile.h"
#include "utils.h"
#include "ExecutableFile.h"

#include <fstream>
#include <atlbase.h>
#include <iostream>
#include <array>
#include <dia2.h>
#include <DbgHelp.h>
#include <regex>


const std::regex nameRegex = std::regex("[^a-zA-Z0-9_]");

PDBFile::PDBFile(ExecutableFile* executableFile, std::string_view path) :
	m_executableFile(executableFile)
{

	// Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to initialize COM");
    }


	CComPtr<IDiaDataSource> pSource;
	hr = CoCreateInstance(CLSID_DiaSource,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               __uuidof(IDiaDataSource),
                               (void**)&pSource);

    if(FAILED(hr))
	{
		throw std::runtime_error("Failed to create DIA source");
	}


	hr = pSource->loadDataFromPdb(Binder::convertToWChar(path.data()).c_str());

	if(FAILED(hr))
	{
		throw std::runtime_error("Failed to load PDB file");
	}

	// Open a session for querying symbols.
	CComPtr<IDiaSession> pSession;
	hr = pSource->openSession(&pSession);

	if(FAILED(hr))
	{
		throw std::runtime_error("Failed to open session");
	}

	// Print the number of symbols in the program database.
	CComPtr<IDiaSymbol> pGlobal;
	hr = pSession->get_globalScope(&pGlobal);
	if(FAILED(hr))
	{
		throw std::runtime_error("Failed to get global scope");
	}


	constexpr std::array symEnums {SymTagFunction, SymTagLabel, SymTagPublicSymbol};

	for(const auto symEnum : symEnums)
	{
		CComPtr<IDiaEnumSymbols> pEnumSymbols;
		hr = pGlobal->findChildren(symEnum, NULL, nsNone, &pEnumSymbols);
		if(FAILED(hr))
		{
			throw std::runtime_error("Failed to find children");
		}

		ULONG celt = 0;
		CComPtr<IDiaSymbol> pSymbol;
		while (SUCCEEDED(pEnumSymbols->Next(1, &pSymbol, &celt)) && celt == 1)
		{
		    DWORD rva;
		    pSymbol->get_relativeVirtualAddress(&rva);

			if(rva == 0)
			{
				pSymbol.Release();
				continue;
			}

		    BSTR name;
		    pSymbol->get_name(&name);

			// Undecorate the symbol name
			wchar_t m_symbolBuffer[1024];
			UnDecorateSymbolNameW(name, m_symbolBuffer, std::size<wchar_t>(m_symbolBuffer), UNDNAME_NAME_ONLY);

			std::string symbolName = Binder::convertToChar(m_symbolBuffer);
			std::string filteredName = std::regex_replace(symbolName, nameRegex, "") + "_" + std::to_string(executableFile->getHash() * rva); // Add hash to avoid name collisions

		    //std::cout << "RVA: 0x" << std::hex << rva << "  " << symbolName << '\n';

			std::cout << filteredName << '\n';

			m_symbols[rva] = filteredName;
			
		    pSymbol.Release();
			SysFreeString(name);
		    celt = 0;
		}
	}
}