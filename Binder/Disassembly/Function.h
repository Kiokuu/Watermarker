#pragma once
#include <vector>


class Function : public AsmItem
{
public:
private:
	std::vector<Instruction> m_instructions;
};