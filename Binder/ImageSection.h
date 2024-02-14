#pragma once
#include <span>
#include <string_view>

class ImageSection
{
public:
	explicit ImageSection() = default;

	explicit ImageSection(std::string_view name, uint32_t virtualAddress, uint32_t virtualSize, uint32_t rawAddress,
	             uint32_t rawSize, uint32_t characteristics, const std::span<uint8_t>& data) :
		m_name(name),
		m_virtualAddress(virtualAddress),
		m_virtualSize(virtualSize),
		m_rawAddress(rawAddress),
		m_rawSize(rawSize),
		m_characteristics(characteristics),
		m_data(data)
	{}

private:
	std::string m_name;
	uint32_t m_virtualAddress;
	uint32_t m_virtualSize;
	uint32_t m_rawAddress;
	uint32_t m_rawSize;
	uint32_t m_characteristics;

	std::span<uint8_t> m_data;
};
