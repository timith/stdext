#pragma once

#include <type_traits>

namespace stdext
{
	template <typename value_type>
	struct intrusive_list_enabled
	{
		intrusive_list_enabled<value_type>* prev = nullptr;
		intrusive_list_enabled<value_type>* next = nullptr;
	};

	/**
	 * @brief Intrusive list. This list is nonowning, it just arranges a collection of pointers to T.
	 * @tparam T the type which the list manages
	*/
	template <typename T>
	class intrusive_list
	{
		static_assert(std::is_base_of<intrusive_list_enabled<T>, T>::value, "value_type must extend intrusive_list_enabled<value_type>");

	public:

		using value_type = T;
		using size_type = std::size_t;
		using reference = value_type&;
		using const_reference = const value_type&;

		void clear()
		{
			head = nullptr;
			tail = nullptr;
		}

		class iterator
		{
		public:

			friend class intrusive_list<value_type>;

			iterator(intrusive_list_enabled<value_type>* node_)
				: node(node_)
			{
			}

			iterator()
			{
			}

			explicit operator bool() const
			{
				return node != nullptr;
			}

			bool operator==(const iterator& other) const
			{
				return node == other.node;
			}

			bool operator!=(const iterator& other) const
			{
				return node != other.node;
			}

			value_type& operator*()
			{
				return *static_cast<value_type*>(node);
			}

			const value_type& operator*() const
			{
				return *static_cast<value_type*>(node);
			}

			value_type* get()
			{
				return static_cast<value_type*>(node);
			}

			const value_type* get() const
			{
				return static_cast<const value_type*>(node);
			}

			value_type* operator->()
			{
				return static_cast<value_type*>(node);
			}

			const value_type* operator->() const
			{
				return static_cast<T*>(node);
			}

			iterator& operator++()
			{
				node = node->next;
				return *this;
			}

			iterator& operator--()
			{
				node = node->prev;
				return *this;
			}

		private:

			intrusive_list_enabled<value_type>* node = nullptr;

		};

		iterator begin()
		{
			return iterator(head);
		}

		iterator end()
		{
			return iterator(tail);
		}

		iterator erase(iterator itr)
		{
			auto* node = itr.get();
			auto* next = node->next;
			auto* prev = node->prev;

			if (prev)
				prev->next = next;
			else
				head = next;

			if (next)
				next->prev = prev;
			else
				tail = prev;

			return next;
		}

		void insert_front(iterator itr)
		{
			auto* node = itr.get();
			if (head)
				head->prev = node;
			else
				tail = node;

			node->next = head;
			node->prev = nullptr;
			head = node;
		}

		void insert_back(iterator itr)
		{
			auto* node = itr.get();
			if (tail)
				tail->next = node;
			else
				head = node;

			node->prev = tail;
			node->next = nullptr;
			tail = node;
		}

		void move_to_front(intrusive_list<T>& other, iterator itr)
		{
			other.erase(itr);
			insert_front(itr);
		}

		void move_to_back(intrusive_list<T>& other, iterator itr)
		{
			other.erase(itr);
			insert_back(itr);
		}

		bool empty() const
		{
			return head == nullptr;
		}

	private:
		intrusive_list_enabled<value_type>* head = nullptr;
		intrusive_list_enabled<value_type>* tail = nullptr;
	};

}