#pragma once

#include <cstddef>
#include <utility>
#include <memory>
#include <atomic>
#include <type_traits>

namespace stdext
{
	class intrusive_ref_enabled
	{
	public:

		intrusive_ref_enabled()
		{
			count.store(0, std::memory_order_relaxed);
		}

		void inc_ref()
		{
			count.fetch_add(1, std::memory_order_relaxed);
		}

		bool dec_ref()
		{
			return count.fetch_sub(1, std::memory_order_acq_rel) == 1;
		}

		size_t ref_count()
		{
			return count.load(std::memory_order_relaxed);
		}

	private:

		std::atomic_size_t count;

	};

	template<typename T, typename Deleter = std::default_delete<T>>
	class intrusive_ref
	{
		static_assert(std::is_base_of<intrusive_ref_enabled, T>::value, "ref_type must extend intrsuive_ref_enabled to be used by intrsive_ref");

	public:

		using ref_type = T;
		using deleter_type = Deleter;

		template<typename T2, typename Deleter2>
		friend class intrusive_ref;

		intrusive_ref()
			: data(nullptr)
		{
		}

		intrusive_ref(std::nullptr_t n)
			: data(nullptr)
		{
		}

		intrusive_ref(T* handle)
			: data(handle)
		{
			if (data)
				data->inc_ref();
		}

		template<typename T2, typename Deleter2 = std::default_delete<Deleter2>>
		intrusive_ref(const intrusive_ref<T2, Deleter2>& other)
		{
			data = (T*)other.data;
			inc_ref();
		}

		template<typename T2, typename Deleter2 = std::default_delete<Deleter2>>
		intrusive_ref(intrusive_ref<T2, Deleter2>&& other)
		{
			data = (T*)other.data;
			other.data = nullptr;
		}

		~intrusive_ref()
		{
			dec_ref();
		}

		template<typename Deleter2 = std::default_delete<Deleter2>>
		intrusive_ref(const intrusive_ref<T, Deleter2>& other)
			: data(other.data)
		{
			inc_ref();
		}

		intrusive_ref& operator=(std::nullptr_t)
		{
			dec_ref();
			data = nullptr;
			return *this;
		}

		template<typename Deleter2 = std::default_delete<Deleter2>>
		intrusive_ref& operator=(const intrusive_ref<T, Deleter2>& other)
		{
			other.inc_ref();
			dec_ref();

			data = other.data;
			return *this;
		}

		template<typename T2, typename Deleter2 = std::default_delete<Deleter2>>
		intrusive_ref& operator=(const intrusive_ref<T2, Deleter2>& other)
		{
			other.inc_ref();
			dec_ref();

			data = other.data;
			return *this;
		}

		template<typename T2, typename Deleter2 = std::default_delete<Deleter2>>
		intrusive_ref& operator=(intrusive_ref<T2, Deleter2>&& other)
		{
			dec_ref();

			data = other.data;
			other.data = nullptr;
			return *this;
		}

		template<typename T2, typename Deleter2 = std::default_delete<Deleter2>>
		intrusive_ref<T2, Deleter2> as()
		{
			return intrusive_ref<T2, Deleter2>(*this);
		}

		T* raw() const
		{
			return data;
		}

		T& operator*()
		{
			return *data;
		}

		const T& operator*() const
		{
			return *data;
		}

		T* operator->()
		{
			return data;
		}

		const T* operator->() const
		{
			return data;
		}

		operator bool() 
		{
			return data != nullptr; 
		}

		operator bool() const 
		{
			return data != nullptr; 
		}

	private:

		void inc_ref() const
		{
			if (data)
				data->inc_ref();
		}

		void dec_ref() const
		{
			if (data)
			{
				if (data->dec_ref())
				{
					Deleter()(data);
					data = nullptr;
				}
			}

			std::shared_ptr<int>
		}

		T* data;

	};

	

}