#pragma once
#include <span>
#include <stdexcept>
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

	std::string_view getName() const { return m_name; }
	uint32_t getVirtualAddress() const { return m_virtualAddress; }
	uint32_t getVirtualSize() const { return m_virtualSize; }
	uint32_t getRawAddress() const { return m_rawAddress; }
	uint32_t getRawSize() const { return m_rawSize; }
	uint32_t getCharacteristics() const { return m_characteristics; }
	const std::span<uint8_t>& getData() const { return m_data; }

	void setVirtualAddress(uint32_t virtualAddress) { m_virtualAddress = virtualAddress; }
	void setVirtualSize(uint32_t virtualSize) { m_virtualSize = virtualSize; }
	void setRawAddress(uint32_t rawAddress) { m_rawAddress = rawAddress; }
	void setRawSize(uint32_t rawSize) { m_rawSize = rawSize; }
	void setCharacteristics(uint32_t characteristics) { m_characteristics = characteristics; }
	void setData(const std::span<uint8_t>& data) { m_data = data; }

	uint32_t vaToFo(uint32_t va) const
	{
		if (va < m_virtualAddress || va >= m_virtualAddress + m_virtualSize)
		{
			throw std::runtime_error("Invalid virtual address");
		}
		return va - m_virtualAddress + m_rawAddress;
	}

	uint32_t foToVa(uint32_t fo) const
	{
		if (fo < m_rawAddress || fo >= m_rawAddress + m_rawSize)
		{
			throw std::runtime_error("Invalid file offset");
		}
		return fo - m_rawAddress + m_virtualAddress;
	}

private:
	std::string m_name;
	uint32_t m_virtualAddress;
	uint32_t m_virtualSize;
	uint32_t m_rawAddress;
	uint32_t m_rawSize;
	uint32_t m_characteristics;

	std::span<uint8_t> m_data;
};
