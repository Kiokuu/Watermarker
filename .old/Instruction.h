#pragma once
#include <string>

class Instruction
{
public:
	Instruction(std::string instruction, uint64_t runtime_address) : m_instruction(std::move(instruction)), m_runtime_address(runtime_address), m_repeats(1) {}

	void setInstruction(const std::string& instruction) { m_instruction = instruction; }
	void setRepeats(uint64_t repeats) { m_repeats = repeats; }

	std::string_view getInstruction() const { return m_instruction; }
	uint64_t getRuntimeAddress() const { return m_runtime_address; }
	uint64_t getRepeats() const { return m_repeats; }

private:
	std::string m_instruction;
	uint64_t m_runtime_address;
	uint64_t m_repeats;
};