#pragma once

namespace Binder
{
	template<typename T>
	inline static T alignTo(T value, T alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}
}