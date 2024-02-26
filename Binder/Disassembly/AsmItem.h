#pragma once
#include <cstdint>

enum class AsmItemType {
	LABEL,
	INSTRUCTION,
	DATA,
};

class AsmItem
{
public:
	AsmItem(AsmItemType type, uint64_t va) : m_type(type), m_va(va) {}

private:
	AsmItemType m_type;
	uint64_t m_va;
};