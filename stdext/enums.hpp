#pragma once

#include <cstdint>
#include <type_traits>

namespace stdext
{
	template <typename T>
	constexpr typename std::underlying_type<T>::type ecast(T x)
	{
		return static_cast<typename std::underlying_type<T>::type>(x);
	}

	template<typename Enum>
	struct enable_bitmask_operators
	{
		static const bool enable = false;
	};

	template<typename Enum>
	typename std::enable_if<enable_bitmask_operators<Enum>::enable, Enum>::type 
		operator |(Enum lhs, Enum rhs)
	{
		return static_cast<Enum>(ecast(lhs) | ecast(rhs));
	}

	template<typename Enum>
	typename std::enable_if<enable_bitmask_operators<Enum>::enable, Enum>::type
		operator |=(Enum& lhs, Enum rhs)
	{
		lhs = static_cast<Enum>(ecast(lhs) | ecast(rhs));
		return lhs;
	}

	template<typename Enum>
	typename std::enable_if<enable_bitmask_operators<Enum>::enable, Enum>::type
		operator &(Enum lhs, Enum rhs)
	{
		return static_cast<Enum>(ecast(lhs) & ecast(rhs));
	}

	template<typename Enum>
	typename std::enable_if<enable_bitmask_operators<Enum>::enable, Enum>::type
		operator &=(Enum& lhs, Enum rhs)
	{
		lhs = static_cast<Enum>(ecast(lhs) & ecast(rhs));
		return lhs;
	}
}

// #define STDEXT_ENABLE_BITMASK_OPERATORS(e) template<> struct stdext::enable_bitmask_operators<e> { static const bool enable = true; };
