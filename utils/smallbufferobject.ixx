export module deckard.sbo;

import std;
import deckard.types;
import deckard.as;
import deckard.assert;
import deckard.debug;
import deckard.math.utils;
import deckard.utils.hash;

namespace deckard
{


	export template<size_t SIZE = 32>
	class sbo
	{
		static_assert(SIZE >= 24, "Minimum size is 24 bytes");
		static constexpr f32    SCALE_FACTOR = 1.5f;
		static constexpr size_t ALIGNMENT    = 8;

	private:
		using value_type      = u8;
		using pointer         = value_type*;
		using reference       = value_type&;
		using const_pointer   = const pointer;
		using const_reference = const reference;
		using difference_type = std::ptrdiff_t;
		using size_type       = u32;

		struct small
		{
			std::array<value_type, SIZE - 1> data;

			value_type is_large : 1;
			value_type size : 7;
		};

		static_assert(sizeof(small) == SIZE);

		struct large
		{
			pointer    ptr;
			size_type  size;
			size_type  capacity;
			value_type padding[sizeof(small) - sizeof(ptr) - sizeof(size) - sizeof(capacity) - 1];

			value_type is_large : 1;
			value_type unused : 7;
		};

		static_assert(sizeof(large) == sizeof(small));

		union
		{
			small small;
			large large;
		} packed;

		class const_iterator;

		class iterator
		{
		private:
			pointer ptr;

		public:
			friend class const_iterator;
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type   = std::ptrdiff_t;
			using value_type        = value_type;

			iterator(const_iterator& ci)
				: ptr(ci.ptr)
			{
			}

			iterator(pointer p)
				: ptr(p)
			{
			}

			reference operator*() const { return *ptr; }

			pointer operator->() const { return ptr; }

			iterator& operator++()
			{
				ptr++;
				return *this;
			}

			iterator operator++(int)
			{
				iterator tmp = *this;
				ptr++;
				return tmp;
			}

			iterator& operator--()
			{
				ptr--;
				return *this;
			}

			iterator operator--(int)
			{
				iterator tmp = *this;
				ptr--;
				return tmp;
			}

			iterator operator+=(int v)
			{
				ptr += v;
				return *this;
			}

			iterator operator-=(int v)
			{
				ptr -= v;
				return *this;
			}

			iterator operator+(difference_type n) const { return iterator(ptr + n); }

			iterator operator-(difference_type n) const { return iterator(ptr - n); }

			difference_type operator-(const iterator& other) const { return ptr - other.ptr; }

			reference& operator[](difference_type n) const { return ptr[n]; }

			auto operator<=>(const iterator&) const = default;

			bool operator==(const iterator& other) const { return ptr == other.ptr; }
		};

		class const_iterator
		{
		private:
			pointer ptr;

		public:
			friend class iterator;
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type   = std::ptrdiff_t;
			using value_type        = value_type;

			const_iterator(pointer p)
				: ptr(p)
			{
			}

			const_reference operator*() const { return *ptr; }

			const_pointer operator->() const { return ptr; }

			const_iterator& operator++()
			{
				ptr++;
				return *this;
			}

			const_iterator operator++(int)
			{
				const_iterator tmp = *this;
				ptr++;
				return tmp;
			}

			const_iterator& operator--()
			{
				ptr--;
				return *this;
			}

			const_iterator operator--(int)
			{
				const_iterator tmp = *this;
				ptr--;
				return tmp;
			}

			const_iterator operator+(difference_type n) const { return const_iterator(ptr + n); }

			const_iterator operator-(difference_type n) const { return const_iterator(ptr - n); }

			difference_type operator-(const const_iterator& other) const { return ptr - other.ptr; }

			const_reference operator[](difference_type n) const
			{
				assert::check(ptr != nullptr, "Ptr is null");
				return ptr[n];
			}

			auto operator<=>(const const_iterator&) const = default;

			bool operator==(const const_iterator& other) const { return ptr == other.ptr; }
		};

		void set_size(size_t newsize)
		{
			if (is_large())
				packed.large.size = as<size_type>(newsize);
			else
				packed.small.size = as<value_type>(newsize);
		}

		void set_capacity(size_t newcapacity)
		{
			if (is_large())
				packed.large.capacity = as<size_type>(newcapacity);
		}

		void set_large(bool value) { packed.small.is_large = value ? 1 : 0; }

		bool is_large() const { return packed.small.is_large == 1 ? true : false; }

		bool is_small() const { return not is_large(); }

		// size
		size_t large_size() const { return packed.large.size; }

		size_t small_size() const { return packed.small.size; }

		// capacity
		size_t small_capacity() const { return packed.small.data.size(); }

		size_t large_capacity() const { return packed.large.capacity; }

		// available_space
		size_t large_available_size() const { return large_capacity() - large_size(); }

		size_t small_available_size() const { return small_capacity() - small_size(); }

		size_t available_size() const
		{
			if (is_small())
				return small_available_size();
			else
				return large_available_size();
		}

		pointer large_rawptr() const
		{
			assert::check(packed.large.ptr != nullptr, "Something went wrong, large ptr not allocated");
			return packed.large.ptr;
		}

		pointer small_rawptr() const { return as<pointer>(packed.small.data.data()); }

		pointer rawptr() const
		{
			if (is_large())
				return large_rawptr();
			else
				return small_rawptr();
		}

		pointer end_rawptr() const
		{
			if (is_large())
				return large_rawptr() + large_size();
			else
				return small_rawptr() + small_size();
		}

		std::span<value_type> large_data() const { return {packed.large.ptr, packed.large.size}; }

		std::span<value_type> small_data() const { return {(pointer)packed.small.data.data(), packed.small.size}; }

		void reset() noexcept
		{
			if (is_large())
			{
				std::fill_n(large_rawptr(), large_size(), 0);

				delete[] packed.large.ptr;
			}

			std::ranges::fill_n(packed.small.data.data(), packed.small.data.size(), 0u);
			packed.small.size     = 0;
			packed.small.is_large = 0;
		}

		void clone(const sbo& from)
		{
			reset();
			if (from.is_small())
			{
				set_size(from.small_size());
				std::memcpy(&packed.small.data[0], &from.packed.small.data[0], from.small_size());
			}
			else
			{
				set_large(true);
				set_size(from.large_size());
				set_capacity(from.large_capacity());

				delete[] packed.large.ptr;
				packed.large.ptr = new value_type[packed.large.capacity];

				std::copy_n(from.packed.large.ptr, large_size(), packed.large.ptr);
			}
		}

		void move(sbo& other)
		{
			reset();
			if (other.is_small())
			{
				set_size(other.small_size());
				std::memcpy(&packed.small.data[0], &other.packed.small.data[0], other.small_size());
			}
			else
			{
				packed.large.ptr      = std::move(other.packed.large.ptr);
				packed.large.size     = other.packed.large.size;
				packed.large.capacity = other.packed.large.capacity;
				packed.large.is_large = 1;

				other.packed.large.is_large = 0;
			}
			other.reset();
		}

		size_t newcapacity_size(size_t size) const { return math::align_integer(as<size_t>(SCALE_FACTOR * size), ALIGNMENT); }

	public:
		sbo() { reset(); }

		sbo(const std::initializer_list<value_type>& il)
		{
			reset();
			if (il.size() <= small_capacity())
			{

				std::copy(il.begin(), il.end(), packed.small.data.begin());
				packed.small.size = as<value_type>(il.size());
			}
			else
			{
				set_large(true);
				set_size(il.size());
				set_capacity(newcapacity_size(il.size()));

				packed.large.ptr = new value_type[packed.large.capacity];
				std::copy(il.begin(), il.end(), packed.large.ptr);
			}
		}

		// Copy
		sbo(sbo const& other) { clone(other); }

		sbo& operator=(sbo const& other)
		{
			if (this != &other)
			{
				clear();
				clone(other);
			}

			return *this;
		}

		// Move
		sbo(sbo&& other) noexcept { move(other); }

		sbo& operator=(sbo&& other) noexcept
		{
			if (this != &other)
			{
				clear();
				move(other);
			}

			return *this;
		}

		~sbo() noexcept { reset(); }

		[[nodiscard("Use the empty check")]] bool empty() const { return size() == 0; }

		bool operator==(const sbo& other) const
		{
			if (size() != other.size())
				return false;

			if (is_small())
				return std::equal(packed.small.data.begin(), packed.small.data.begin() + small_size(), other.packed.small.data.begin());
			else
				return std::equal(packed.large.ptr, packed.large.ptr + large_size(), other.packed.large.ptr);
		}

#if __cpp_deleted_function
#error "Delete reason (C++26) is supported, use it"
		bool operator<(const sbo&)  = delete("Less-than compare doesn't make sense");
		bool operator<=(const sbo&) = delete("Less-than compare doesn't make sense");
		bool operator>(const sbo&)  = delete("Greater-than compare doesn't make sense");
		bool operator>=(const sbo&) = delete("Greater-than compare doesn't make sense");
#endif


		void swap(sbo& other) noexcept { std::swap(packed, other.packed); }

		size_t max_size() const
		{
			if (is_small())
				return packed.small.data.size();
			else
				return limits::max<size_type>;
		}

		void clear()
		{
			if (is_small())
			{
				std::ranges::fill_n(packed.small.data.data(), packed.small.data.size(), 0u);
				packed.small.size = 0;
			}
			else
			{
				std::ranges::fill_n(packed.large.ptr, packed.large.size, 0u);
				packed.large.size = 0;
			}
		}

		iterator insert(const_iterator pos, const std::span<value_type> buffer)
		{
			if (buffer.empty())
				return iterator(pos.ptr);

			const auto pivot   = std::distance(begin(), pos);
			size_t     newsize = size() + buffer.size();
			if (newsize > capacity())
				resize(newsize);

			const pointer ptr = rawptr();
			pos               = cbegin() + pivot;

			std::copy_backward(ptr + pivot, ptr + size(), ptr + newsize);
			std::copy(buffer.data(), buffer.data() + buffer.size(), ptr + pivot);

			set_size(newsize);

			return iterator(ptr + pivot);
		}

		iterator insert(const_iterator pos, const value_type& v) { return insert(pos, {&v, 1}); }

		iterator insert(iterator pos, std::span<value_type> buffer)
		{
			if (buffer.empty())
				return pos;

			const size_t pivot   = std::distance(begin(), pos);
			size_t       newsize = size() + buffer.size();
			if (newsize > capacity())
				resize(newsize);

			const pointer ptr   = rawptr();
			size_t        width = size();
			if (pos != end())
				std::copy_backward(ptr + pivot, ptr + width, ptr + newsize);

			std::copy_n(buffer.data(), buffer.size(), ptr + pivot);

			set_size(newsize);

			return iterator(ptr + pivot);
		}

		iterator insert(iterator pos, const value_type& v) { return insert(pos, std::span<value_type>{(pointer)&v, 1}); }

		void assign(const std::span<value_type> buffer)
		{
			clear();
			insert(begin(), buffer);
		}

		void assign(const sbo<SIZE>& other) { assign(other.data()); }

		void assign(const value_type& v) { assign(std::span<value_type>{(pointer)&v, 1}); }

		void assign(const std::initializer_list<value_type>& il)
		{
			clear();
			for (const auto& i : il)
				push_back(i);
		}

		auto operator+(const sbo<SIZE>& other)
		{
			auto ret(*this);
			ret.append(other);
			return ret;
		}

		auto operator+=(const sbo<SIZE>& other)
		{
			append(other);
			return *this;
		}

		sbo<SIZE> sub_sbo(size_t start, size_t end) const
		{
			assert::check(start < size(), "Index out-of-bounds");
			assert::check(end < size(), "Index out-of-bounds");

			sbo<SIZE> result;
			for (size_t i = start; i < end; ++i)
				result.push_back(at(i));
			return result;
		}

		void append(const std::span<value_type> buffer) { insert(end(), buffer); }

		void append(const sbo<SIZE>& other) { append(other.data()); }

		void append(const value_type& v) { append(std::span<value_type>{(pointer)&v, 1}); }

		void append(const std::initializer_list<value_type>& il)
		{
			//
			for (const auto& i : il)
				append(i);
		}

		[[nodiscard("Use the front value")]] value_type front() const
		{
			assert::check(not empty(), "Front called on empty SBO");

			return rawptr()[0];
		}

		[[nodiscard("Use the back value")]] value_type back() const
		{
			assert::check(not empty(), "Back called on empty SBO");

			return rawptr()[size() - 1];
		}

		size_t size() const { return is_large() ? large_size() : small_size(); }

		size_t capacity() const { return is_large() ? large_capacity() : small_capacity(); }

		[[nodiscard("Use result on index operator")]] reference at(size_t index) { return operator[](index); }

		[[nodiscard("Use result on index operator")]] const_reference at(size_t index) const { return operator[](index); }

		[[nodiscard("Use result on index operator")]] reference operator[](size_t index)
		{
			assert::check(index < capacity(), "Indexing out-of-bounds");


			if (is_large())
			{
				assert::check(index < size(), "Indexing out-of-bounds");
				assert::check(packed.large.ptr != nullptr, "sbo buffer is null");
				return packed.large.ptr[index];
			}
			else
			{
				return packed.small.data[index];
			}
		}

		[[nodiscard("Use result on index operator")]] const_reference operator[](size_t index) const
		{
			assert::check(index < capacity(), "Indexing out-of-bounds");


			if (is_large())
			{
				assert::check(index < size(), "Indexing out-of-bounds");
				assert::check(packed.large.ptr != nullptr, "sbo buffer is null");
				return packed.large.ptr[index];
			}
			else
			{
				return (const_reference)packed.small.data[index];
			}
		}

		void fill(const value_type& v)
		{
			const pointer   ptr = rawptr();
			const size_type max = capacity();

			for (size_type i = 0; i < max; ++i)
				ptr[i] = v;

			set_size(max);
		}

		void push_back(const value_type& v)
		{
			auto current_size     = size();
			auto current_capacity = capacity();
			if (current_size + 1 > current_capacity)
			{
				resize(newcapacity_size(current_capacity));
			}


			if (is_large() and current_size <= large_capacity())
			{
				packed.large.ptr[packed.large.size] = v;
				packed.large.size += 1;
			}
			else if (is_small() and current_size <= small_capacity())
			{
				packed.small.data[packed.small.size] = v;
				packed.small.size += 1;
			}
		}

		void pop_back()
		{
			if (is_large() and large_size() > 0)
			{
				packed.large.size -= 1;
			}
			else if (is_small() and small_size() > 0)
			{
				packed.small.size -= 1;
			}
		}

		// resize
		void resize(size_t newsize)
		{
			size_t oldsize = size();

			if (newsize <= capacity())
			{
				pointer ptr = rawptr();
				std::fill(ptr + newsize, ptr + capacity(), 0);
				set_size(newsize);
				return;
			}

			size_t new_capacity = std::max(newsize, newcapacity_size(capacity()));
			auto   newptr       = new value_type[new_capacity];
			std::fill_n(newptr, new_capacity, 0u);

			pointer oldptr = rawptr();
			std::copy_n(oldptr, std::min(oldsize, newsize), newptr);
			if (is_large())
				delete[] packed.large.ptr;

			packed.large.ptr = newptr;
			set_large(true);
			set_capacity(new_capacity);
			set_size(oldsize);
		}

		void shrink_to_fit()

		{
			if (is_large())
			{
				size_t oldsize = large_size();
				if (oldsize <= small_capacity())
				{

					pointer ptr    = large_rawptr();
					pointer newptr = small_rawptr();
					std::copy_n(ptr, oldsize, newptr);

					set_large(false);
					set_size(as<value_type>(oldsize));
					return;
				}

				if (oldsize < large_capacity())
				{
					auto newptr = new value_type[oldsize];
					std::fill_n(newptr, oldsize, 0u);

					pointer oldptr = large_rawptr();

					std::copy_n(oldptr, oldsize, newptr);

					delete[] packed.large.ptr;
					packed.large.ptr = newptr;

					set_large(true);
					set_capacity(oldsize);
					set_size(oldsize);
				}
			}
		}

		void reserve(size_t newsize)

		{
			if (newsize > capacity())
			{
				resize(newsize);
			}
		}

		[[nodiscard("Use the data span")]] std::span<value_type> data() const
		{
			assert::check(not empty(), "Data span cannot be empty");

			return is_large() ? large_data() : small_data();
		}

		// iterator

		bool contains(const value_type& value) const { return find(value) != cend(); }

		iterator begin() { return iterator(rawptr()); }

		iterator end() { return iterator(end_rawptr()); }

		const_iterator begin() const { return const_iterator(rawptr()); }

		const_iterator end() const { return const_iterator(end_rawptr()); }

		const_iterator cbegin() const { return const_iterator(rawptr()); }

		const_iterator cend() const { return const_iterator(end_rawptr()); }

		std::reverse_iterator<iterator> rbegin() { return std::reverse_iterator(end()); }

		std::reverse_iterator<iterator> rend() { return std::reverse_iterator(begin()); }

		std::reverse_iterator<const_iterator> rbegin() const { return std::reverse_iterator(end()); }

		std::reverse_iterator<const_iterator> rend() const { return std::reverse_iterator(begin()); }

		std::reverse_iterator<const_iterator> crbegin() const { return std::reverse_iterator(cend()); }

		std::reverse_iterator<const_iterator> crend() const { return std::reverse_iterator(cbegin()); }

		iterator find(const value_type& value)
		{
			auto it = begin();
			for (; it != end(); ++it)
			{
				if (*it == value)
					return it;
			}
			return end();
		}

		const_iterator find(const value_type& value) const
		{
			auto it = cbegin();
			for (; it != cend(); ++it)
			{
				if (*it == value)
					return it;
			}
			return cend();
		}

		iterator find(const std::span<value_type>& buffer)
		{
			if (buffer.empty() or size() < buffer.size())
			{
				return end();
			}

			for (auto it = begin(); it != end() - buffer.size() + 1; ++it)
			{
				if (std::equal(buffer.begin(), buffer.end(), it))
				{
					return it;
				}
			}

			return end();
		}

		const_iterator find(const std::span<value_type>& buffer) const
		{
			if (buffer.empty() or size() < buffer.size())
			{
				return end();
			}

			for (auto it = cbegin(); it != cend() - buffer.size() + 1; ++it)
			{
				if (std::equal(buffer.begin(), buffer.end(), it))
				{
					return it;
				}
			}

			return cend();
		}

		friend void swap(sbo& lhs, sbo& rhs) noexcept { lhs.swap(rhs); }

		iterator erase(size_t pos, size_t count)
		{
			assert::check(count <= size(), "trying to erase too much");
			assert::check(pos <= size() - 1, "indexing outside buffer");
			assert::check(pos + count <= size(), "indexing outside buffer");

			size_t pivot    = pos + count;
			size_t new_size = size() - count;

			if (pivot < size()) // fill if erased to end of buffer
			{
				std::copy(begin() + pivot, end(), begin() + pos);
			}

			std::fill(begin() + new_size, end(), 0);
			set_size(new_size);
			return begin() + pos;
		}

		iterator erase(iterator first, iterator last)
		{
			if (first == last)
				return iterator(rawptr());


			const auto pivot = std::distance(begin(), first);
			const auto count = std::distance(first, last);
			return erase(pivot, count);
		}

		iterator erase(const_iterator first, const_iterator last) { return erase(iterator(first), iterator(last)); }

		iterator erase(iterator pos) { return erase(pos, pos + 1); }

		iterator erase(const_iterator pos) { return erase(pos, pos + 1); }

		// replace
		iterator replace(iterator pos, const std::span<value_type>& buffer)
		{
			if (pos == end())
			{
				auto pivot = erase(pos - 1);
				return insert(pivot, buffer);
			}
			return replace(pos, pos + 1, buffer);
		}

		iterator replace(iterator first, iterator end, const std::span<value_type>& buffer)
		{
			if (first == end)
			{
				auto pos = erase(first);
				return insert(pos, buffer);
			}

			auto pos = erase(first, end);
			return insert(first, buffer);
		}

		iterator replace(size_t pos, size_t count, const std::span<value_type>& buffer)
		{
			auto first = begin() + pos;
			auto end   = first + count;
			return replace(first, end, buffer);
		}

		iterator find_first_of(const std::span<value_type>& buffer)
		{
			if (buffer.empty() or empty())
			{
				return end();
			}

			for (auto it = begin(); it != end(); ++it)
			{
				if (std::find(buffer.begin(), buffer.end(), *it) != buffer.end())
				{
					return it;
				}
			}

			return end();
		}

		iterator find_first_of(std::string_view view) const { return find_first_of({as<value_type*>(view.data()), view.size()}); }

		// iterator find_first_of(const std::initializer_list<value_type>& il)
		//{
		//	return find_first_of(std::span<value_type>{il.begin() , il.size()});
		// }

		// iterator find_first_of(char c) const { return find_first_of(std::span<value_type>{&c, 1}); }

		size_t hash() const
		{
			if (is_small())
			{
				return utils::hash_values<u8>({small_data().data(), small_size()});
			}
			else
			{
				return utils::hash_values<u8>({large_data().data(), large_size()});
			}
		}
	};

	static_assert(sizeof(sbo<24>) == 24);
	static_assert(sizeof(sbo<32>) == 32);


} // namespace deckard

export namespace std
{
	using namespace deckard;

	template<size_t SIZE>
	struct hash<sbo<SIZE>>
	{
		size_t operator()(const sbo<SIZE>& obj) const { return obj.hash(); }
	};

	// template<size_t SIZE>
	// struct formatter<sbo<SIZE>>
	//{
	//	// TODO: Parse width
	//	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
	//
	//	auto format(const sbo<SIZE>& v, std::format_context& ctx) const
	//	{
	//		std::format_to(ctx.out(), "sbo: [");
	//
	//		for(auto it = v.begin(); it != v.end(); it++)
	//			std::format_to(ctx.out(), "{:#02X}{}"sv, it, it != v.end() ? ", "sv : ""sv);
	//
	//		return std::format_to(ctx.out(), "]");
	//	}
	// };
} // namespace std
