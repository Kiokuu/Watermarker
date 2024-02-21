#include "Disassembler.h"

#include <cinttypes>

#include "ExecutableFile.h"
#include "Zydis/Decoder.h"
#include "Zydis/Formatter.h"
#include "Zydis/SharedTypes.h"
#include "Zydis/Status.h"

#include <cstring>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "PDBFile.h"
#include "DataEntry.h"


/* 
 * TODO:
 * - Resolve remaining calls, jumps etc. Need their own labels
 	jb 0x0000000040011EF9 ; 0x40011eec
	mov eax, [r9+0x08] ; 0x40011eee
	add eax, ecx ; 0x40011ef2
	cmp rdx, rax ; 0x40011ef4
	jb somewhereelse ; 0x40011ef7
	add r9, 0x28 ; 0x40011ef9

-> 	jb blahblah; 0x40011eec
	mov eax, [r9+0x08] ; 0x40011eee
	add eax, ecx ; 0x40011ef2
	cmp rdx, rax ; 0x40011ef4
	jb somewhereelse ; 0x40011ef7

	blahblah:
	add r9, 0x28 ; 0x40011ef9

    (almost done, needs testing)


 * - Write sections properly (atm its a bit messed up, multiple writes of same section
 *
 * - Add a writer to keep track of indentation
 *
 * - Not all labels are added. alot of undefined. Need full refactor.
 */

Disassembler::Disassembler(ExecutableFile* executableFile) : m_executable_file(executableFile)
{
    m_output =  std::ofstream("disassembled.asm");
    if (!m_output.is_open()) {
        std::cerr << "Error opening disassembled.asm" << "\n";
        return; // Exit constructor if file opening failed
    }

    disassemble_data();
    disassemble_functions();

    write_globals();
    write_data();
    write_functions();
}

void Disassembler::disassemble_data()
{
	std::cout << "\n\n===================================================" << "\n";
    std::cout << "Disassembling data" << "\n";
    std::cout << "===================================================" << "\n";

	const PDBFile* pdbFile = m_executable_file->getPDBFile();
    const std::vector<ImageSection>& sections = m_executable_file->getSections();

    // TODO: First create data with symbols
    for(const ImageSection& section: sections)
    {
        if(section.getCharacteristics() & IMAGE_SCN_MEM_EXECUTE)
        {
	        continue;
        }

	    // its data, use symbols to figure out if data is meaningful
        uint64_t sectionVirtualAddress = section.getVirtualAddress();

        size_t offset = 0;
        std::cout << "\n-------------------------------" << "\n";
        std::cout << "Section: " << section.getName() << "\n";

        while(offset < section.getData().size())
        {
            uint64_t scanAddress = sectionVirtualAddress + offset;

            Symbol symbol;
            if(pdbFile->getSymbol(scanAddress, &symbol))
            {
            	SymbolType type = symbol.getType();
                size_t length = symbol.getLength();

                const std::span<uint8_t>& data = section.getData().subspan(offset, length);
                std::cout << "\nSymbol Name: " << symbol.getName() << " length: " << length << "\n";
                std::cout << "Symbol Data: ";
                for (const auto& byte : data)
                {
                    std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " " << std::dec;
                }
                std::cout << "\n";

                DataEntry dataEnt(symbol.getName(), data);

                m_data[scanAddress] = dataEnt;

                offset += length;
            }
            offset++;
        }
        std::cout << "===================================================" << "\n";
    }
}

void Disassembler::disassemble_functions()
{
	PDBFile* pdbFile = m_executable_file->getPDBFile();
    const std::vector<ImageSection>& sections = m_executable_file->getSections();

    std::cout << "===================================================" << "\n";
    std::cout << "Parsing functions" << "\n";
    std::cout << "===================================================" << "\n";

	ZydisDecodedInstruction instruction;
    ZydisDecoder decoder;
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64))) {
        std::cerr << "Error initializing decoder" << "\n";
        return; // Exit constructor if decoder initialization failed
    }

    CustomInstructionFormatter customFormatter(m_executable_file->getImageBase(), pdbFile->getSymbols());
	const ZydisFormatter* formatter = customFormatter.getFormatter();

    for (const ImageSection& section : sections) {
        ZyanU64 runtime_address = m_executable_file->getImageBase() + section.getVirtualAddress();
        std::vector<Function>& functions = m_functions[section.getVirtualAddress()];


        if(!(section.getCharacteristics() & IMAGE_SCN_MEM_EXECUTE))
		{
			continue;
		}

        // Get section data
        const std::span<uint8_t>& data = section.getData();
        size_t offset = 0;

        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

        std::cout << "Section: " << section.getName() << "\n";


        // TODO: not do this, and find another way.
        while (offset < data.size()) {
            uint32_t remaining_size = data.size() - offset;
            ZyanStatus status = ZydisDecoderDecodeFull(&decoder, data.data() + offset, remaining_size, &instruction, operands);
            Symbol symbol;

            if (status != ZYAN_STATUS_SUCCESS) {
                if(status == ZYDIS_STATUS_NO_MORE_DATA)
                {
                    std::cout << "No more data..." << "\n";
	                break;
                }

                std::cerr << "Error decoding instruction at offset " << offset << "\n";
                //std::cerr << "Status: " <<  FormatZyanStatus(status) << "\n";
                printf("%016" PRIX64 "  \n", runtime_address);
                offset++;
                continue;
            }

            char buffer[512];

            CustomUserData userData = {&customFormatter};

            ZydisFormatterFormatInstruction(formatter, &instruction, operands, instruction.operand_count_visible, buffer, sizeof(buffer), runtime_address, (void*)&userData);
            offset += instruction.length;
            runtime_address += instruction.length;
        }

        // ----------------------------------------------------

        offset = 0;
        runtime_address = m_executable_file->getImageBase() + section.getVirtualAddress();

    	Function currentFunction {"start_unk", 0};
        //size_t currentFunctionOffset = 0;

        // Run again with our new labels
        while (offset < data.size()) {
            uint32_t remaining_size = data.size() - offset;

            ZyanStatus status = ZydisDecoderDecodeFull(&decoder, data.data() + offset, remaining_size, &instruction, operands);

            Symbol symbol;
            if(pdbFile->getSymbol(runtime_address - m_executable_file->getImageBase(), &symbol))
            {
                //temp
                //m_output << symbol.getName() << ":\n";

                functions.push_back(currentFunction);
                currentFunction = Function(symbol);
			}

            if (status != ZYAN_STATUS_SUCCESS) {
                if(status == ZYDIS_STATUS_NO_MORE_DATA)
                {
                    std::cout << "No more data..." << "\n";
	                break;
                }

                std::cerr << "Error decoding instruction at offset " << offset << "\n";
                //std::cerr << "Status: " <<  FormatZyanStatus(status) << "\n";
                printf("%016" PRIX64 "  \n", runtime_address);
                offset++;
                continue;
            }

            char buffer[512];

            CustomUserData userData = {&customFormatter};
            ZydisFormatterFormatInstruction(formatter, &instruction, operands, instruction.operand_count_visible, buffer, sizeof(buffer), runtime_address, (void*)&userData);



            //m_output << "\t" << buffer << " ; 0x" <<  std::hex << runtime_address << std::dec << "\n";

            //std::stringstream instr;
            //instr << "\t" << buffer << " ; 0x" <<  std::hex << runtime_address << std::dec << "\n";

            Instruction instr(buffer, runtime_address);
            Instruction* lastInstruction = nullptr;
            bool addInstruction = true;

            if (currentFunction.getLastInstruction(&lastInstruction)) {
                if(lastInstruction->getInstruction() == instr.getInstruction())
                {
	                lastInstruction->setRepeats(lastInstruction->getRepeats() + 1);
					addInstruction = false;
				}
            }

            if(addInstruction)
				currentFunction.addInstruction(instr);

            offset += instruction.length;
            runtime_address += instruction.length;


            //m_instructions.push_back(instruction);// TODO: Transform into own representation with symbols, also instruction is just info about the decoded instruction
        }
        std::cout << "===================================================" << "\n";
    
    }
}


void Disassembler::write_globals()
{

    m_output << "; Globals\n";

    for(const auto& [address, symbol] : m_executable_file->getPDBFile()->getSymbols())
    {
	    if(symbol.getType() == SymbolType::Data || symbol.getType() == SymbolType::Function || symbol.getType() == SymbolType::Label)
	    {
	    	m_output << "global " << symbol.getName() << " ; " << address << "\n";
		}
	}


    /*
    m_output << "; Data\n";
	for(const auto& [address, data] : m_data)
	{
		m_output << "global " << data.name << "\n";
	}

    m_output << "\n\n; Functions\n";

    for(const auto& function : m_functions)
	{
		//m_output << "; " << function.first << "\n";
        for (const auto& f : function.second)
        {
	        m_output << "global " << f.getName() << "\n";
		}
	}
    */
    m_output << "\n";
}

void Disassembler::write_section(const ImageSection* section)
{
    uint64_t sectionCharacteristics = section->getCharacteristics();

    std::string flags;

    if(sectionCharacteristics & IMAGE_SCN_CNT_CODE || sectionCharacteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
    {
        flags += " alloc";
	}
    
    if (sectionCharacteristics & IMAGE_SCN_MEM_EXECUTE)
    {
        flags += " exec";
    }
    if (sectionCharacteristics & IMAGE_SCN_MEM_WRITE)
    {
        flags += " write";
    }


    // Get section name, remove null bytes
    std::string sectionName = section->getName().data();
    std::erase(sectionName, '\0');

    m_output << "section " << sectionName << flags << "\n";
}

void Disassembler::write_data()
{

    m_output << "section .data\n"; // todo: split data up properly

	for (const auto& [address, data] : m_data)
	{
        m_output << '\t' << data.name << "\tdb\t";

        for (size_t i = 0; i < data.data.size(); i++) {
            m_output << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data.data[i]);
            if (i < data.data.size() - 1) {
                m_output << ", ";
            }
        }

        m_output << "\n";
	}
}


void Disassembler::write_functions()
{
    for(const auto& functionSection : m_functions)
    {
	    write_section(m_executable_file->getSection(functionSection.first));

		for (const auto& f : functionSection.second)
		{
			m_output << f.getName() << ":\n";
			m_output << f.getInstructionString();
		}
	}
}