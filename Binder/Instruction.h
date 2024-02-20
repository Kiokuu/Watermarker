#pragma once
#include <string>

class Instruction
{
public:
	Instruction(std::string instruction, uint64_t runtime_address) : m_instruction(std::move(instruction)), m_runtime_address(runtime_address) {}

	std::string_view getInstruction() const { return m_instruction; }
	uint64_t getRuntimeAddress() const { return m_runtime_address; }

private:
	std::string m_instruction;
	uint64_t m_runtime_address;
};