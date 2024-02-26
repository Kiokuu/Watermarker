#pragma once


class Label
{
public:
	Label(uint64_t rva, std::string_view name) : m_rva(rva), m_name(name) {}

private:
	uint64_t m_rva;
	std::string m_name;
};