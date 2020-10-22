#pragma once

#ifdef _MSC_VER
	#include <intrin.h>
#endif

#include <cstdint>

namespace stdext
{
#ifdef __GNUC__
	#define _leading_zeroes(x) ((x) == 0 ? 32 : __builtin_clz(x))
	#define _trailing_zeroes(x) ((x) == 0 ? 32 : __builtin_ctz(x))
	#define _trailing_ones(x) __builtin_ctz(~uint32_t(x))
#elif defined(_MSC_VER)
	namespace _intern
	{
		static inline uint32_t _clz(uint32_t x)
		{
			unsigned long result;
			if (_BitScanReverse(&result, x))
				return 31 - result;
			else
				return 32;
		}

		static inline uint32_t _ctz(uint32_t x)
		{
			unsigned long result;
			if (_BitScanForward(&result, x))
				return result;
			else
				return 32;
		}
	}

	#define _leading_zeroes(x) ::stdext::_intern::_clz(x)
	#define _trailing_zeroes(x) ::stdext::_intern::_ctz(x)
	#define _trailing_ones(x) ::stdext::_intern::_ctz(~uint32_t(x))
	#else
#error "Implement me."
#endif

	// Searches through the bitmask "value", from least signifigant to most and for every 1 set in the bitmask, calls func on the position of that 1.
	template <typename T>
	inline void for_each_bit(uint32_t value, const T& func)
	{
		while (value)
		{
			uint32_t bit = _trailing_zeroes(value);
			func(bit);
			value &= ~(1u << bit);
		}
	}

	template <typename T>
	inline void for_each_bit_range(uint32_t value, const T& func)
	{
		if (value == ~0u)
		{
			func(0, 32);
			return;
		}

		uint32_t bit_offset = 0;
		while (value)
		{
			uint32_t bit = _trailing_zeroes(value);
			bit_offset += bit;
			value >>= bit;
			uint32_t range = _trailing_ones(value);
			func(bit_offset, range);
			value &= ~((1u << range) - 1);
		}
	}

	inline uint32_t most_signifigant_bit_set(uint32_t value)
	{
		if (!value)
			return 32;

		return 31 - _leading_zeroes(value);
	}

#undef _leading_zeroes
#undef _trailing_zeroes
#undef _trailing_ones
}