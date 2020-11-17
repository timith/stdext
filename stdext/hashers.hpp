#pragma once

#include <cstdint>

#include <type_traits>

namespace stdext
{

	template<typename id_type>
	class hasher
	{
		static_assert(std::is_arithmetic<id_type>::value, "hasher id type must be an arithmatical type");

	public:

		using hash_type = id_type;

		inline hash_type value() { return h; }
		operator hash_type() const { return h; }

	protected:

		hash_type h;
	};

	template<typename>
	struct fnv_hash_traits;

	template<>
	struct fnv_hash_traits<std::uint32_t> {
		using type = std::uint32_t;
		static constexpr std::uint32_t offset = 2166136261;
		static constexpr std::uint32_t prime = 16777619;
	};


	template<>
	struct fnv_hash_traits<std::uint64_t> {
		using type = std::uint64_t;
		static constexpr std::uint64_t offset = 14695981039346656037ull;
		static constexpr std::uint64_t prime = 1099511628211ull;
	};

	template<typename id_type>
	class fnv1_hasher : public hasher<id_type>
	{
		using traits_type = fnv_hash_traits<id_type>;

	public:

		using hash_type = id_type;

		explicit constexpr fnv1_hasher(hash_type h_) noexcept
			: h(h_)
		{
		}

		constexpr fnv1_hasher() noexcept
			: h(traits_type::offset)
		{
		}

		template<typename value_type_>
		constexpr typename std::enable_if<std::is_arithmetic<value_type_>::value, void>::type hash(value_type_ value) noexcept
		{
			h = (h * traits_type::prime) ^ static_cast<hash_type>(value);
		}

	};

	template<typename id_type>
	class fnv1a_hasher : public hasher<id_type>
	{
		using traits_type = fnv_hash_traits<id_type>;

	public:

		using hash_type = id_type;

		explicit constexpr fnv1a_hasher(hash_type h_) noexcept
			: h(h_)
		{
		}

		constexpr fnv1a_hasher() noexcept
			: h(traits_type::offset)
		{
		}

		template<typename value_type_>
		constexpr typename std::enable_if<std::is_arithmetic<value_type_>::value, void>::type hash(value_type_ value) noexcept
		{
			h = (h ^ static_cast<hash_type>(value)) * traits_type::prime;
		}

	};



}