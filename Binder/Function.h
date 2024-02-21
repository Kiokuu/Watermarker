#pragma once
#include <sstream>

#include "PDBFile.h"
#include "Instruction.h"

class Function
{
public:
	Function() = default;


	Function (Symbol symbol)
	{
		m_name = symbol.getName();
		m_length = symbol.getLength();
	}

	Function (std::string_view name, size_t length) : m_name(name), m_length(length), m_instructions() {}


	void addInstruction(const Instruction& instruction)
	{
		m_instructions.push_back(instruction);
	}

    bool getLastInstruction(Instruction** outInstruction)
    {
        if (m_instructions.empty())
        {
            *outInstruction = nullptr;
            return false;
        }
        
        *outInstruction = &m_instructions.back();
        return true;
    }


	std::string getInstructionString() const
	{
		std::stringstream instructionString;

		for (const auto& instruction : m_instructions)
		{
			if(instruction.getRepeats()>1)
			{
				instructionString << "\t" << "times " << instruction.getRepeats() << " " << instruction.getInstruction() << std::hex << " ; 0x" << instruction.getRuntimeAddress() << std::dec << "\n";
			}

			instructionString << "\t" << instruction.getInstruction() << std::hex << " ; 0x" << instruction.getRuntimeAddress() << std::dec << "\n";
		}
		return instructionString.str();
	}

	const std::vector<Instruction>& getInstructions() const { return m_instructions; }
	std::string_view getName() const { return m_name; }
	size_t getLength() const { return m_length; }

private:
	std::string m_name;
	size_t m_length;
	std::vector<Instruction> m_instructions;

	// Name of function
	// Instructions
};