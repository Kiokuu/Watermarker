#pragma once

#include <string>

namespace Binder
{
	/**
	 * @brief Aligns a value to a specified alignment.
	 * @tparam T The type of the value.
	 * @param value The value to align.
	 * @param alignment The alignment value.
	 * @return The aligned value.
	 */
	template <typename T>
	inline static T alignTo(T value, T alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	/**
	 * @brief Converts a C-style string to a wide string.
	 * @param src Pointer to the null-terminated input string.
	 * @return The converted wide string.
	 */
	inline std::wstring convertToWChar(const char* src)
	{
		if (src == nullptr)
			return std::wstring();

		const size_t size = strlen(src) + 1;

		auto dest = new wchar_t[size];
		size_t outSize;
		errno_t err = mbstowcs_s(&outSize, dest, size, src, size - 1);
		std::wstring result(dest);
		delete[] dest;

		return result;
	}

	/**
	 * @brief Converts a wide string to a C-style string.
	 * @param src Pointer to the null-terminated input wide string.
	 * @return The converted C-style string.
	 */
	inline std::string convertToChar(const wchar_t* src)
	{
		if (src == nullptr)
			return std::string();

		const size_t size = wcslen(src) + 1;

		auto dest = new char[size];
		size_t outSize;
		errno_t err = wcstombs_s(&outSize, dest, size, src, size - 1);
		std::string result(dest);
		delete[] dest;

		return result;
	}
}
