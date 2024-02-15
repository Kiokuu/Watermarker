#pragma once

namespace Binder
{
	template <typename T>
	inline static T alignTo(T value, T alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	inline std::wstring convertToWChar(const char* src)
	{
		const size_t size = strlen(src) + 1;

		auto dest = new wchar_t[size];
		size_t outSize;
		errno_t err = mbstowcs_s(&outSize, dest, size, src, size - 1);
		std::wstring result(dest);
		delete[] dest;

		return result;
	}

	inline std::string convertToChar(const wchar_t* src)
	{
		const size_t size = wcslen(src) + 1;

		auto dest = new char[size];
		size_t outSize;
		errno_t err = wcstombs_s(&outSize, dest, size, src, size - 1);
		std::string result(dest);
		delete[] dest;

		return result;
	}
}
