#pragma once

#include "list.hpp"
#include "../object_pool.hpp"
#include <vector>

namespace stdext
{

	template<typename hash_type, typename T>
	class intrusive_hashmap_enabled : public intrusive_list_enabled<T>
	{
		static_assert(std::is_arithmetic<hash_type>::value, "hash id type must be an arithmatical type");

	public:

		intrusive_hashmap_enabled()
		{
		}

		intrusive_hashmap_enabled(hash_type hash)
			: intrusive_hashmap_key(hash)
		{
		}

		void set_hash(hash_type hash)
		{
			intrusive_hashmap_key = hash;
		}

		hash_type get_hash() const
		{
			return intrusive_hashmap_key;
		}

	private:

		hash_type intrusive_hashmap_key = 0;
	};

	template <typename hash_type, typename T>
	struct intrusive_pod_wrapper : public intrusive_hashmap_enabled<hash_type, T>
	{
		template <typename U>
		explicit intrusive_pod_wrapper(U&& value_)
			: value(std::forward<U>(value_))
		{
		}

		intrusive_pod_wrapper() = default;

		T& get()
		{
			return value;
		}

		const T& get() const
		{
			return value;
		}

		T value = {};
	};

	// This HashMap is non-owning. It just arranges a list of pointers.
	// It's kind of special purpose container used by the Vulkan backend.
	// Dealing with memory ownership is done through composition by a different class.
	// T must inherit from IntrusiveHashMapEnabled<T>.
	// Each instance of T can only be part of one hashmap.

	//Same as list to intrusive list, this can be thought of as an intrusive std::unordered_map.

	template <typename Hash, typename T>
	class instrusive_hashmap_holder
	{
		static_assert(std::is_arithmetic<Hash>::value, "hash id type must be an arithmatical type");
		static_assert(std::is_base_of<intrusive_hashmap_enabled<Hash, T>, T>::value, "value_type must extend intrusive_hashmap_enabled");

	public:

		T* find(Hash hash) const
		{
			if (values.empty())
				return nullptr;

			hash_type hash_mask = values.size() - 1;
			auto masked = hash & hash_mask;
			for (unsigned i = 0; i < load_count; i++)
			{
				if (values[masked] && get_hash(values[masked]) == hash)
					return values[masked];
				masked = (masked + 1) & hash_mask;
			}

			return nullptr;
		}

		/*template <typename P>
		bool find_and_consume_pod(Hash hash, P& p) const
		{
			T* t = find(hash);
			if (t)
			{
				p = t->get();
				return true;
			}
			else
				return false;
		}*/

		// Inserts, if value already exists, insertion does not happen.
		// Return value is the data which is not part of the hashmap.
		// It should be deleted or similar.
		// Returns nullptr if nothing was in the hashmap for this key.
		T* insert_yield(T*& value)
		{
			if (values.empty())
				grow();

			Hash hash_mask = values.size() - 1;
			auto hash = get_hash(value);
			auto masked = hash & hash_mask;

			for (unsigned i = 0; i < load_count; i++)
			{
				if (values[masked] && get_hash(values[masked]) == hash)
				{
					T* ret = value;
					value = values[masked];
					return ret;
				}
				else if (!values[masked])
				{
					values[masked] = value;
					list.insert_front(value);
					return nullptr;
				}
				masked = (masked + 1) & hash_mask;
			}

			grow();
			return insert_yield(value);
		}

		T* insert_replace(T* value)
		{
			if (values.empty())
				grow();

			Hash hash_mask = values.size() - 1;
			auto hash = get_hash(value);
			auto masked = hash & hash_mask;

			for (unsigned i = 0; i < load_count; i++)
			{
				if (values[masked] && get_hash(values[masked]) == hash)
				{
					std::swap(values[masked], value);
					list.erase(value);
					list.insert_front(values[masked]);
					return value;
				}
				else if (!values[masked])
				{
					assert(!values[masked]);
					values[masked] = value;
					list.insert_front(value);
					return nullptr;
				}
				masked = (masked + 1) & hash_mask;
			}

			grow();
			return insert_replace(value);
		}

		T* erase(Hash hash)
		{
			Hash hash_mask = values.size() - 1;
			auto masked = hash & hash_mask;

			for (unsigned i = 0; i < load_count; i++)
			{
				if (values[masked] && get_hash(values[masked]) == hash)
				{
					auto* value = values[masked];
					list.erase(value);
					values[masked] = nullptr;
					return value;
				}
				masked = (masked + 1) & hash_mask;
			}
			return nullptr;
		}

		void erase(T* value)
		{
			erase(get_hash(value));
		}

		void clear()
		{
			list.clear();
			values.clear();
			load_count = 0;
		}

		typename intrusive_list<T>::iterator begin()
		{
			return list.begin();
		}

		typename intrusive_list<T>::iterator end()
		{
			return list.end();
		}

		intrusive_list<T>& inner_list()
		{
			return list;
		}

	private:

		inline bool compare_key(Hash masked, Hash hash) const
		{
			return get_key_for_index(masked) == hash;
		}

		inline Hash get_hash(const T* value) const
		{
			return static_cast<const intrusive_hashmap_enabled<Hash, T>*>(value)->get_hash();
		}

		inline Hash get_key_for_index(Hash masked) const
		{
			return get_hash(values[masked]);
		}

		bool insert_inner(T* value)
		{
			Hash hash_mask = values.size() - 1;
			auto hash = get_hash(value);
			auto masked = hash & hash_mask;

			for (unsigned i = 0; i < load_count; i++)
			{
				if (!values[masked])
				{
					values[masked] = value;
					return true;
				}
				masked = (masked + 1) & hash_mask;
			}
			return false;
		}

		void grow()
		{

			constexpr size_t initial_size = 16;
			constexpr size_t initial_load_count = 3;

			bool success;
			do
			{
				for (auto& v : values)
					v = nullptr;

				if (values.empty())
				{
					values.resize(initial_size);
					load_count = initial_load_count;
				}
				else
				{
					values.resize(values.size() * 2);
					load_count++;
				}

				// Re-insert.
				success = true;
				for (auto& t : list)
				{
					if (!insert_inner(&t))
					{
						success = false;
						break;
					}
				}
			} while (!success);
		}

		std::vector<T*> values;
		intrusive_list<T> list;
		size_t load_count = 0;
	};

	template <typename Hash, typename T>
	class intrusive_hashmap
	{
	public:

		intrusive_hashmap(const intrusive_hashmap&) = delete;
		void operator=(const intrusive_hashmap&) = delete;

		intrusive_hashmap()
		{
		}

		~intrusive_hashmap()
		{
			clear();
		}

		void clear()
		{
			auto& list = hashmap.inner_list();
			auto itr = list.begin();
			while (itr != list.end())
			{
				auto* to_free = itr.get();
				itr = list.erase(itr);
				pool.free(to_free);
			}

			hashmap.clear();
		}

		T* find(Hash hash) const
		{
			return hashmap.find(hash);
		}

		T& operator[](Hash hash)
		{
			auto* t = find(hash);
			if (!t)
				t = emplace_yield(hash);
			return *t;
		}

		/*template <typename P>
		bool find_and_consume_pod(Hash hash, P& p) const
		{
			return hashmap.find_and_consume_pod(hash, p);
		}*/

		void erase(T* value)
		{
			hashmap.erase(value);
			pool.free(value);
		}

		void erase(Hash hash)
		{
			auto* value = hashmap.erase(hash);
			if (value)
				pool.free(value);
		}

		template <typename... P>
		T* emplace_replace(Hash hash, P&&... p)
		{
			T* t = allocate(std::forward<P>(p)...);
			return insert_replace(hash, t);
		}

		template <typename... P>
		T* emplace_yield(Hash hash, P&&... p)
		{
			T* t = allocate(std::forward<P>(p)...);
			return insert_yield(hash, t);
		}

		template <typename... P>
		T* allocate(P&&... p)
		{
			return pool.allocate(std::forward<P>(p)...);
		}

		void free(T* value)
		{
			pool.free(value);
		}

		T* insert_replace(Hash hash, T* value)
		{
			static_cast<intrusive_hashmap_enabled<Hash, T>*>(value)->set_hash(hash);
			T* to_delete = hashmap.insert_replace(value);
			if (to_delete)
				pool.free(to_delete);
			return value;
		}

		T* insert_yield(Hash hash, T* value)
		{
			static_cast<intrusive_hashmap_enabled<Hash, T>*>(value)->set_hash(hash);
			T* to_delete = hashmap.insert_yield(value);
			if (to_delete)
				pool.free(to_delete);
			return value;
		}

		typename instrusive_hashmap_holder<Hash, T>::iterator begin()
		{
			return hashmap.begin();
		}

		typename instrusive_hashmap_holder<Hash, T>::iterator end()
		{
			return hashmap.end();
		}

		intrusive_hashmap& get_thread_unsafe()
		{
			return *this;
		}

	private:
		instrusive_hashmap_holder<Hash, T> hashmap;
		object_pool<T> pool;
	};

}