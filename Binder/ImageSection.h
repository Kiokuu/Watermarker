#pragma once
#include <span>
#include <stdexcept>
#include <string_view>

/**
 * @brief Represents a section within an executable image.
 */
class ImageSection
{
public:
	/**
	 * @brief Default constructor.
	 */
	explicit ImageSection() = default;

	/**
	 * @brief Constructor with section details.
	 * @param name Name of the section.
	 * @param virtualAddress Virtual address of the section.
	 * @param virtualSize Virtual size of the section.
	 * @param rawAddress Raw address of the section.
	 * @param rawSize Raw size of the section.
	 * @param characteristics Characteristics of the section.
	 * @param data Span representing the section data.
	 */
	explicit ImageSection(std::string_view name, uint32_t virtualAddress, uint32_t virtualSize, uint32_t rawAddress,
	                      uint32_t rawSize, uint32_t characteristics, const std::span<uint8_t>& data) :
		m_name(name),
		m_virtualAddress(virtualAddress),
		m_virtualSize(virtualSize),
		m_rawAddress(rawAddress),
		m_rawSize(rawSize),
		m_characteristics(characteristics),
		m_data(data)
	{
	}

	/**
	 * @brief Gets the name of the section.
	 * @return The name of the section.
	 */
	std::string_view getName() const { return m_name; }

	/**
	 * @brief Gets the virtual address of the section.
	 * @return The virtual address of the section.
	 */
	uint32_t getVirtualAddress() const { return m_virtualAddress; }

	/**
	 * @brief Gets the virtual size of the section.
	 * @return The virtual size of the section.
	 */
	uint32_t getVirtualSize() const { return m_virtualSize; }

	/**
	 * @brief Gets the raw address of the section.
	 * @return The raw address of the section.
	 */
	uint32_t getRawAddress() const { return m_rawAddress; }

	/**
	 * @brief Gets the raw size of the section.
	 * @return The raw size of the section.
	 */
	uint32_t getRawSize() const { return m_rawSize; }

	/**
	 * @brief Gets the characteristics of the section.
	 * @return The characteristics of the section.
	 */
	uint32_t getCharacteristics() const { return m_characteristics; }

	/**
	 * @brief Gets the data of the section as a span.
	 * @return The data of the section as a span.
	 */
	const std::span<uint8_t>& getData() const { return m_data; }

	/**
	 * @brief Sets the virtual address of the section.
	 * @param virtualAddress The virtual address to set.
	 */
	void setVirtualAddress(uint32_t virtualAddress) { m_virtualAddress = virtualAddress; }

	/**
	 * @brief Sets the virtual size of the section.
	 * @param virtualSize The virtual size to set.
	 */
	void setVirtualSize(uint32_t virtualSize) { m_virtualSize = virtualSize; }

	/**
	 * @brief Sets the raw address of the section.
	 * @param rawAddress The raw address to set.
	 */
	void setRawAddress(uint32_t rawAddress) { m_rawAddress = rawAddress; }

	/**
	 * @brief Sets the raw size of the section.
	 * @param rawSize The raw size to set.
	 */
	void setRawSize(uint32_t rawSize) { m_rawSize = rawSize; }

	/**
	 * @brief Sets the characteristics of the section.
	 * @param characteristics The characteristics to set.
	 */
	void setCharacteristics(uint32_t characteristics) { m_characteristics = characteristics; }

	/**
	 * @brief Sets the data of the section.
	 * @param data The data to set.
	 */
	void setData(const std::span<uint8_t>& data) { m_data = data; }

	/**
	 * @brief Converts a virtual address to a file offset within the section.
	 * @param va The virtual address to convert.
	 * @return The corresponding file offset.
	 * @throws std::runtime_error if the virtual address is invalid.
	 */
	uint32_t vaToFo(uint32_t va) const
	{
		if (va < m_virtualAddress || va >= m_virtualAddress + m_virtualSize)
		{
			throw std::runtime_error("Invalid virtual address");
		}
		return va - m_virtualAddress + m_rawAddress;
	}

	/**
	 * @brief Converts a file offset to a virtual address within the section.
	 * @param fo The file offset to convert.
	 * @return The corresponding virtual address.
	 * @throws std::runtime_error if the file offset is invalid.
	 */
	uint32_t foToVa(uint32_t fo) const
	{
		if (fo < m_rawAddress || fo >= m_rawAddress + m_rawSize)
		{
			throw std::runtime_error("Invalid file offset");
		}
		return fo - m_rawAddress + m_virtualAddress;
	}

private:
	std::string m_name; /**< Name of the section. */
	uint32_t m_virtualAddress; /**< Virtual address of the section. */
	uint32_t m_virtualSize; /**< Virtual size of the section. */
	uint32_t m_rawAddress; /**< Raw address of the section. */
	uint32_t m_rawSize; /**< Raw size of the section. */
	uint32_t m_characteristics; /**< Characteristics of the section. */
	std::span<uint8_t> m_data; /**< Span representing the section data. */
};
