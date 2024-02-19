#pragma once
#include "PDBFile.h"

class Function
{
public:
	Function (Symbol symbol)
	{
		m_name = symbol.getName();
		m_length = symbol.getLength();
	}

	Function (std::string_view name, size_t length) : m_name(name), m_length(length) {}

	std::string_view getName() const { return m_name; }
	size_t getLength() const { return m_length; }

private:
	std::string m_name;
	size_t m_length;
	

	// Name of function
	// Instructions
};