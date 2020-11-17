#pragma once

#include <stddef.h>
#include <cstdlib>
#include <utility>
#include <exception>
#include <algorithm>
#include <initializer_list>
#include <type_traits>

namespace stdext
{
	// std::aligned_storage does not support size == 0, so roll our own.
	template <typename T, size_t N>
	class aligned_buffer
	{
	public:
		T* data()
		{
			return reinterpret_cast<T*>(aligned_char);
		}

	private:
		alignas(T) char aligned_char[sizeof(T) * N];
	};

	template <typename T>
	class aligned_buffer<T, 0>
	{
	public:
		T* data()
		{
			return nullptr;
		}
	};

	// Simple vector which supports up to N elements inline, without malloc/free.
	// We use a lot of throwaway vectors all over the place which triggers allocations.
	// It is *NOT* a drop-in replacement in general projects.
	template <typename T, size_t N = 8>
	class small_vector
	{
	public:

		class iterator
		{
		public:

			using iterator_category = std::random_access_iterator_tag;
			using difference_type = int64_t;
			using value_type = T;
			using reference = T&;
			using pointer = T*;

			iterator(T* ptr)
				: ptr(ptr)
			{
			}

			iterator()
			{
			}

			explicit operator bool() const
			{
				return ptr != nullptr;
			}

			bool operator==(const iterator& other) const
			{
				return ptr == other.ptr;
			}

			bool operator!=(const iterator& other) const
			{
				return ptr != other.ptr;
			}

			reference operator*()
			{
				return *ptr;
			}

			pointer get()
			{
				return ptr;
			}

			pointer operator->()
			{
				return ptr;
			}

			iterator& operator++()
			{
				++ptr;
				return *this;
			}

			iterator operator++(int)
			{
				iterator tmp = *this;
				++ptr;
				return tmp;
			}

			iterator& operator--()
			{
				--ptr;
				return *this;
			}

			iterator operator--(int)
			{
				iterator tmp = *this;
				--ptr;
				return tmp;
			}

			iterator& operator+=(const difference_type off) {
				ptr += off;
				return *this;
			}

			iterator operator+(const difference_type off) const {
				iterator tmp = *this;
				return tmp += off;
			}

			iterator& operator-=(const difference_type off) {
				return *this += -off;
			}

			iterator operator-(const difference_type off) const {
				iterator tmp = *this;
				return tmp -= off;
			}

			difference_type operator-(const iterator& rhs) const {
				return ptr - rhs.ptr;
			}

		private:

			T* ptr = nullptr;

		};

		using const_iterator = const iterator;
		using reference = T&;
		using const_reference = const T&;

		small_vector()
		{
			ptr = stack_storage.data();
			buffer_capacity = N;
		}

		template<typename InputIt>
		small_vector(InputIt arg_list_begin, InputIt arg_list_end)
			: small_vector()
		{
			auto count = std::distance(arg_list_end - arg_list_begin);
			reserve(count);
			for (size_t i = 0; i < count; i++, arg_list_begin++)
				new (&ptr[i]) T(*arg_list_begin);
			buffer_size = count;
		}

		small_vector(small_vector&& other) noexcept : small_vector()
		{
			*this = std::move(other);
		}

		small_vector(const std::initializer_list<T>& init_list) : small_vector()
		{
			insert(end(), init_list.begin(), init_list.end());
		}

		small_vector& operator=(small_vector&& other) noexcept
		{
			clear();
			if (other.ptr != other.stack_storage.data())
			{
				// Pilfer allocated pointer.
				if (ptr != stack_storage.data())
					std::free(ptr);
				ptr = other.ptr;
				buffer_size = other.buffer_size;
				buffer_capacity = other.buffer_capacity;
				other.ptr = nullptr;
				other.buffer_size = 0;
				other.buffer_capacity = 0;
			}
			else
			{
				// Need to move the stack contents individually.
				reserve(other.buffer_size);
				for (size_t i = 0; i < other.buffer_size; i++)
				{
					new (&ptr[i]) T(std::move(other.ptr[i]));
					other.ptr[i].~T();
				}
				buffer_size = other.buffer_size;
				other.buffer_size = 0;
			}
			return *this;
		}

		small_vector(const small_vector& other)
			: small_vector()
		{
			*this = other;
		}

		small_vector& operator=(const small_vector& other)
		{
			clear();
			reserve(other.buffer_size);
			for (size_t i = 0; i < other.buffer_size; i++)
				new (&ptr[i]) T(other.ptr[i]);
			buffer_size = other.buffer_size;
			return *this;
		}

		explicit small_vector(size_t count)
			: small_vector()
		{
			resize(count);
		}

		~small_vector()
		{
			clear();
			if (ptr != stack_storage.data())
				std::free(ptr);
		}

		reference operator[](size_t i)
		{
			return ptr[i];
		}

		const_reference operator[](size_t i) const
		{
			return ptr[i];
		}

		bool empty() const
		{
			return buffer_size == 0;
		}

		size_t size() const
		{
			return buffer_size;
		}

		T* data()
		{
			return ptr;
		}

		const T* data() const
		{
			return ptr;
		}

		iterator begin()
		{
			return iterator{ ptr };
		}

		iterator end()
		{
			return iterator{ ptr + buffer_size };
		}

		const_iterator begin() const
		{
			return iterator{ ptr };
		}

		const_iterator end() const
		{
			return iterator{ ptr + buffer_size };
		}

		reference front()
		{
			return ptr[0];
		}

		const_reference front() const
		{
			return ptr[0];
		}

		reference back()
		{
			return ptr[buffer_size - 1];
		}

		const_reference back() const
		{
			return ptr[buffer_size - 1];
		}

		void clear()
		{
			if constexpr (!std::is_trivially_destructible<T>::value)
			{
				for (size_t i = 0; i < buffer_size; i++)
					ptr[i].~T();
			}
			buffer_size = 0;
		}

		void push_back(const T& t)
		{
			reserve(buffer_size + 1);
			new (&ptr[buffer_size]) T(t);
			buffer_size++;
		}

		void push_back(T&& t)
		{
			reserve(buffer_size + 1);
			new (&ptr[buffer_size]) T(std::move(t));
			buffer_size++;
		}

		void pop_back()
		{
			// Work around false positive warning on GCC 8.3.
			// Calling pop_back on empty vector is undefined.
			if (!empty())
				resize(buffer_size - 1);
		}

		//Constructs a new object at the back of the vector
		template <typename... Ts>
		void emplace_back(Ts&&... ts)
		{
			reserve(buffer_size + 1);
			new (&ptr[buffer_size]) T(std::forward<Ts>(ts)...);
			buffer_size++;
		}

		void reserve(size_t count)
		{
			if (count > buffer_capacity)
			{
				size_t target_capacity = buffer_capacity;
				if (target_capacity == 0)
					target_capacity = 1;
				if (target_capacity < N)
					target_capacity = N;

				while (target_capacity < count)
					target_capacity <<= 1u;

				T* new_buffer = target_capacity > N ? static_cast<T*>(std::malloc(target_capacity * sizeof(T))) : stack_storage.data();

				if (!new_buffer)
					std::terminate();

				// In case for some reason two allocations both come from same stack.
				if (new_buffer != ptr)
				{
					// We don't deal with types which can throw in move constructor.
					for (size_t i = 0; i < buffer_size; i++)
					{
						new (&new_buffer[i]) T(std::move(ptr[i]));
						ptr[i].~T();
					}
				}

				if (ptr != stack_storage.data())
					free(ptr);
				ptr = new_buffer;
				buffer_capacity = target_capacity;
			}
		}

		template<typename InputIt>
		void insert(iterator itr, const InputIt first, const InputIt last)
		{
			auto count = std::distance(first, last);

			if (itr == end())
			{
				reserve(buffer_size + count);
				for (size_t i = 0; i < count; i++, first++)
					new (&ptr[buffer_size + i]) T(*first);
				buffer_size += count;
			}
			else
			{
				if (buffer_size + count > buffer_capacity)
				{
					auto target_capacity = buffer_size + count;
					if (target_capacity == 0)
						target_capacity = 1;
					if (target_capacity < N)
						target_capacity = N;

					while (target_capacity < count)
						target_capacity <<= 1u;

					// Need to allocate new buffer. Move everything to a new buffer.
					T* new_buffer =
						target_capacity > N ? static_cast<T*>(std::malloc(target_capacity * sizeof(T))) : stack_storage.data();
					if (!new_buffer)
						std::terminate();

					// First, move elements from source buffer to new buffer.
					// We don't deal with types which can throw in move constructor.
					auto* target_itr = new_buffer;
					auto* original_source_itr = begin();

					if (new_buffer != ptr)
					{
						while (original_source_itr != itr)
						{
							new (target_itr) T(std::move(*original_source_itr));
							original_source_itr->~T();
							++original_source_itr;
							++target_itr;
						}
					}

					// Copy-construct new elements.
					for (auto* source_itr = first; source_itr != last; ++source_itr, ++target_itr)
						new (target_itr) T(*source_itr);

					// Move over the other half.
					if (new_buffer != ptr || first != last)
					{
						while (original_source_itr != end())
						{
							new (target_itr) T(std::move(*original_source_itr));
							original_source_itr->~T();
							++original_source_itr;
							++target_itr;
						}
					}

					if (ptr != stack_storage.data())
						std::free(ptr);
					ptr = new_buffer;
					buffer_capacity = target_capacity;
				}
				else
				{
					// Move in place, need to be a bit careful about which elements are constructed and which are not.
					// Move the end and construct the new elements.
					auto* target_itr = end() + count;
					auto* source_itr = end();
					while (target_itr != end() && source_itr != itr)
					{
						--target_itr;
						--source_itr;
						new (target_itr) T(std::move(*source_itr));
					}

					// For already constructed elements we can move-assign.
					std::move_backward(itr, source_itr, target_itr);

					// For the inserts which go to already constructed elements, we can do a plain copy.
					while (itr != end() && first != last)
						*itr++ = *first++;

					// For inserts into newly allocated memory, we must copy-construct instead.
					while (first != last)
					{
						new (itr) T(*first);
						++itr;
						++first;
					}
				}

				buffer_size += count;
			}
		}

		void insert(iterator itr, const T& value)
		{
			insert(itr, iterator{ &value }, iterator{ &value + 1 });
		}

		iterator erase(iterator itr)
		{
			std::move(itr + 1, end(), itr);
			ptr[--buffer_size].~T();
			return iterator{ itr };
		}

		iterator erase(iterator first, iterator last)
		{
			if (last == end())
			{
				resize(size_t(first - begin()));
			}
			else
			{
				auto new_size = buffer_size - (last - first);
				iterator itr = std::move(last, end(), first);
				resize(new_size);
			}
			return first;
		}

		void resize(size_t new_size)
		{
			if (new_size > buffer_size)
			{
				reserve(new_size);
				for (size_t i = buffer_size; i < new_size; i++)
					new (&ptr[i]) T();
			}
			else if(new_size < buffer_size)
			{
				if constexpr (!std::is_trivially_destructible<T>::value)
					for (size_t i = new_size; i < buffer_size; i++)
						ptr[i].~T();
			}

			buffer_size = new_size;
		}

	private:
		size_t buffer_capacity = 0;
		size_t buffer_size = 0;

		T* ptr = nullptr;
		aligned_buffer<T, N> stack_storage;
	};
}
