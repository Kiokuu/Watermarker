#pragma once
#include <span>
#include <string>


// Stores name of data, type of data, original rva of data, data + size.

class DataEntry
{
public:
	DataEntry() = default;
	DataEntry(std::string_view name, std::span<uint8_t> data) : name(name), data(data) {}

	std::string name;
	std::span<uint8_t> data;

private:

};