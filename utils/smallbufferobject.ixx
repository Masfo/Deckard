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
		using const_pointer   = const value_type*;
		using const_reference = const value_type&;
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

		// ##############################################################
		//

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
			if (is_large() and packed.large.ptr)
				delete[] packed.large.ptr;

			std::memset(&packed, 0, sizeof(packed));
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
				packed.large.ptr      = other.packed.large.ptr;
				packed.large.size     = other.packed.large.size;
				packed.large.capacity = other.packed.large.capacity;
				packed.large.is_large = 1;

				other.packed.large.ptr      = nullptr;
				other.packed.large.is_large = 0;
			}
			other.reset();
		}

		size_t newcapacity_size(size_t size) const
		{
			return math::align_integer(as<size_t>(SCALE_FACTOR * size), ALIGNMENT);
		}

	public:
		// iterator
		using iterator               = pointer;
		using const_iterator         = const_pointer;
		using reverse_iterator       = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		constexpr iterator begin() noexcept { return rawptr(); }

		constexpr const_iterator begin() const noexcept { return rawptr(); }

		constexpr const_iterator cbegin() const noexcept { return rawptr(); }

		constexpr iterator end() noexcept { return rawptr() + size(); }

		constexpr const_iterator end() const noexcept { return rawptr() + size(); }

		constexpr const_iterator cend() const noexcept { return rawptr() + size(); }

		// reverse iterator
		reverse_iterator rbegin() { return std::reverse_iterator(end()); }

		reverse_iterator rend() { return std::reverse_iterator(begin()); }

		const_reverse_iterator rbegin() const { return std::reverse_iterator(end()); }

		const_reverse_iterator rend() const { return std::reverse_iterator(begin()); }

		const_reverse_iterator crbegin() const { return std::reverse_iterator(cend()); }

		const_reverse_iterator crend() const { return std::reverse_iterator(cbegin()); }

	public:
		sbo()
		{
			packed.small.size     = 0;
			packed.small.is_large = 0;
			reset();
		}

		explicit sbo(std::span<const value_type> buffer)
		{
			packed.small.size     = 0;
			packed.small.is_large = 0;
			reset();
			if (buffer.size() <= small_capacity())
			{
				std::copy(buffer.begin(), buffer.end(), packed.small.data.begin());
				packed.small.size = as<value_type>(buffer.size());
			}
			else
			{
				set_large(true);
				set_size(buffer.size());
				set_capacity(newcapacity_size(buffer.size()));
				packed.large.ptr = new value_type[packed.large.capacity];
				std::copy(buffer.begin(), buffer.end(), packed.large.ptr);
			}
		}

		sbo(const std::initializer_list<value_type>& il)
		{
			packed.small.size     = 0;
			packed.small.is_large = 0;
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
		sbo(sbo const& other)
		{
			packed.small.size     = 0;
			packed.small.is_large = 0;
			clone(other);
		}

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
		sbo(sbo&& other) noexcept
		{
			packed.small.size     = 0;
			packed.small.is_large = false;
			move(other);
		}

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
				return std::equal(
				  packed.small.data.begin(), packed.small.data.begin() + small_size(), other.packed.small.data.begin());
			else
				return std::equal(packed.large.ptr, packed.large.ptr + large_size(), other.packed.large.ptr);
		}

#if __cpp_deleted_function
#error "Delete reason (C++26) is supported, use it"
		bool operator<(const sbo&)  = delete ("Less-than compare doesn't make sense");
		bool operator<=(const sbo&) = delete ("Less-than compare doesn't make sense");
		bool operator>(const sbo&)  = delete ("Greater-than compare doesn't make sense");
		bool operator>=(const sbo&) = delete ("Greater-than compare doesn't make sense");
#else
		bool operator<(const sbo&)  = delete;
		bool operator<=(const sbo&) = delete;
		bool operator>(const sbo&)  = delete;
		bool operator>=(const sbo&) = delete;
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
				std::ranges::fill_n(packed.small.data.data(), packed.small.data.size(), 0_u8);
				packed.small.size = 0;
			}
			else
			{
				std::ranges::fill_n(packed.large.ptr, packed.large.size, 0_u8);
				packed.large.size = 0;
			}
		}

		// insert

		iterator insert(const_iterator pos, std::span<const value_type> buffer)
		{
			iterator mutable_pos = begin() + std::distance(cbegin(), pos);
			return insert(mutable_pos, buffer);
		}

		iterator insert(const_iterator pos, const value_type& v) { return insert(pos, {&v, 1}); }

		iterator insert(iterator pos, std::span<const value_type> buffer)
		{
			if (buffer.empty())
				return pos;

			const size_t pivot   = std::distance(begin(), pos);
			const size_t oldsize = size();
			size_t       newsize = oldsize + buffer.size();

			bool self_insert = buffer.data() >= rawptr() and buffer.data() < end_rawptr();
			if (self_insert)
			{
				sbo<SIZE> tmp(buffer);
				if (newsize > capacity())
					resize(newsize);
				const pointer ptr = rawptr();
				if (pivot < oldsize)
					std::copy_backward(ptr + pivot, ptr + oldsize, ptr + newsize);
				std::copy_n(tmp.rawptr(), buffer.size(), ptr + pivot);
				set_size(newsize);
				return iterator(ptr + pivot);
			}

			if (newsize > capacity())
				resize(newsize);

			const pointer ptr = rawptr();
			if (pivot < oldsize)
				std::copy_backward(ptr + pivot, ptr + oldsize, ptr + newsize);

			std::copy_n(buffer.data(), buffer.size(), ptr + pivot);
			set_size(newsize);
			return iterator(ptr + pivot);
		}

		iterator insert(iterator pos, const value_type& v) { return insert(pos, std::span<value_type>{(pointer)&v, 1}); }

		// assign
		void assign(std::span<const value_type> buffer)
		{
			const bool self_assign = (buffer.data() >= rawptr() and buffer.data() < end_rawptr());

			if (self_assign)
			{
				const size_t source_size = buffer.size();

				assert::check(source_size <= capacity(), "Self-assign subspan exceeds capacity invariants");

				std::array<value_type, SIZE> temp_buffer;
				std::ranges::copy_n(buffer.data(), source_size, temp_buffer.begin());

				clear();

				std::ranges::copy_n(temp_buffer.data(), source_size, rawptr());
				set_size(source_size);
				return;
			}

			clear();
			insert(cbegin(), buffer);
		}

		void assign(const sbo<SIZE>& other)
		{
			if (this != &other)
				assign(other.data());
		}

		void assign(const value_type& v) { assign(std::span<const value_type>{(pointer)&v, 1}); }

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

		void append(const std::span<const value_type> buffer) { insert(end(), buffer); }

		void append(const sbo<SIZE>& other) { append(other.data()); }

		void append(const value_type& v) { append(std::span<value_type>{(pointer)&v, 1}); }

		void append(const std::initializer_list<value_type>& il)
		{
			for (const auto& i : il)
				append(i);
		}

		// prepend
		void prepend(const std::span<const value_type> buffer) { insert(begin(), buffer); }

		void prepend(const sbo<SIZE>& other) { prepend(other.data()); }

		void prepend(const value_type& v) { prepend(std::span<value_type>{(pointer)&v, 1}); }

		void prepend(const std::initializer_list<value_type>& il)
		{
			//
			for (const auto& i : il)
				prepend(i);
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
			if (is_large())
			{
				assert::check(index < large_size(), "Indexing out-of-bounds");
				assert::check(packed.large.ptr != nullptr, "sbo buffer is null");
				return packed.large.ptr[index];
			}
			else
			{
				assert::check(index < small_capacity(), "Indexing out-of-bounds");
				return packed.small.data[index];
			}
		}

		[[nodiscard("Use result on index operator")]] const_reference operator[](size_t index) const
		{
			assert::check(index < size(), "Indexing out-of-bounds");


			if (is_large())
			{
				assert::check(packed.large.ptr != nullptr, "sbo buffer is null");
				return packed.large.ptr[index];
			}
			else
			{
				return packed.small.data[index];
			}
		}

		void fill(const value_type& v)
		{
			const pointer ptr = rawptr();
			const auto    max = capacity();

			for (auto i = 0ull; i < max; ++i)
				ptr[i] = v;

			set_size(max);
		}

		void push_back(const value_type& v)
		{
			const size_t current_size = size();

			if (current_size >= capacity())
				reserve(newcapacity_size(current_size + 1));

			pointer ptr       = rawptr();
			ptr[current_size] = v;

			set_size(current_size + 1);
		}

		void pop_back()
		{
			assert::check(not empty(), "Cannot pop back from empty sbo");

			const size_t current_size = size();

			pointer ptr           = rawptr();
			ptr[current_size - 1] = 0_u8;

			set_size(current_size - 1);
		}

		// reserve
		void reserve(size_t new_capacity)
		{
			if (new_capacity <= capacity())
				return;

			const size_t oldsize         = size();
			const size_t actual_capacity = std::max(new_capacity, newcapacity_size(capacity()));
			auto         newptr          = new value_type[actual_capacity];

			std::ranges::fill_n(newptr, actual_capacity, 0_u8);

			std::ranges::copy_n(rawptr(), oldsize, newptr);

			if (is_large())
				delete[] packed.large.ptr;

			packed.large.ptr = newptr;
			set_large(true);
			set_capacity(actual_capacity);
			set_size(oldsize);
		}

		// resize
		void resize(size_t newsize)
		{
			const size_t oldsize = size();

			if (newsize > capacity())
				reserve(newsize);

			const pointer ptr = rawptr();
			if (newsize > oldsize)
			{
				std::ranges::fill_n(ptr + oldsize, newsize - oldsize, 0_u8);
			}
			else if (newsize < oldsize)
			{
				std::ranges::fill_n(ptr + newsize, oldsize - newsize, 0_u8);
			}

			set_size(newsize);
		}

		void shrink_to_fit()
		{
			if (not is_large())
				return;

			const size_t current_size = size();

			if (current_size <= small_capacity())
			{
				const pointer old_heap_ptr = packed.large.ptr;

				std::array<value_type, SIZE - 1> tmp;
				if (current_size > 0)
					std::ranges::copy_n(old_heap_ptr, current_size, tmp.begin());

				delete[] old_heap_ptr;
				std::memset(&packed, 0, sizeof(packed));

				std::ranges::copy_n(tmp.begin(), current_size, packed.small.data.begin());
				if (current_size > 0)
					std::ranges::copy_n(tmp.begin(), current_size, packed.small.data.begin());

				set_large(false);
				set_size(current_size);
			}
			else if (current_size < large_capacity())
			{
				pointer old_heap_ptr = packed.large.ptr;

				pointer new_heap_ptr = new (std::nothrow) value_type[current_size];
				if (not new_heap_ptr)
					return;

				std::ranges::copy_n(old_heap_ptr, current_size, new_heap_ptr);
				delete[] old_heap_ptr;

				packed.large.ptr = new_heap_ptr;
				set_capacity(current_size);
			}
		}

		[[nodiscard("Use the data span")]] std::span<value_type> data() const
		{
			return is_large() ? large_data() : small_data();
		}

		// contains
		bool contains(const value_type& value) const { return find(value) != cend(); }


		// find
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

		iterator find(std::span<const value_type>& buffer)
		{
			const size_t container_size = size();
			const size_t search_size    = buffer.size();

			if (search_size == 0)
				return end();

			if (container_size < search_size)
				return end();

			const size_t  max_offset     = container_size - search_size;
			const pointer start_ptr      = begin();
			const pointer end_search_ptr = start_ptr + max_offset;

			for (auto it = start_ptr; it <= end_search_ptr; ++it)
			{
				if (std::ranges::equal(buffer, it))
					return it;
			}

			return end();
		}

		const_iterator find(const std::span<const value_type>& buffer) const
		{
			const size_t container_size = size();
			const size_t search_size    = buffer.size();

			if (search_size == 0)
				return cend();

			if (container_size < search_size)
				return cend();

			const size_t        max_offset     = container_size - search_size;
			const const_pointer start_ptr      = cbegin();
			const const_pointer end_search_ptr = start_ptr + max_offset;

			for (auto it = start_ptr; it <= end_search_ptr; ++it)
			{
				if (std::equal(buffer.begin(), buffer.end(), it))
					return it;
			}

			return cend();
		}

		friend void swap(sbo& lhs, sbo& rhs) noexcept { lhs.swap(rhs); }

		iterator erase(size_t pos, size_t count)
		{
			if (count == 0)
				return begin() + pos;

			const size_t old_size = size();

			assert::check(pos <= old_size && count <= (old_size - pos), "Indexing outside buffer limits");

			const size_t  new_size     = old_size - count;
			const pointer ptr          = rawptr();
			const size_t  rem_elements = old_size - (pos + count);

			if (rem_elements > 0)
				std::memmove(ptr + pos, ptr + pos + count, rem_elements);

			std::ranges::fill_n(ptr + new_size, count, 0_u8);
			set_size(new_size);

			return ptr + pos;
		}

		iterator erase(iterator first, iterator last)
		{
			if (first == last)
				return first;

			const auto pivot = std::distance(begin(), first);
			const auto count = std::distance(first, last);
			return erase(pivot, count);
		}

		iterator erase(iterator pos) { return erase(pos, pos + 1); }

		iterator erase(const_iterator first, const_iterator last) { return erase(iterator(first), iterator(last)); }

		iterator erase(const_iterator pos) { return erase(pos, pos + 1); }

		// replace
		iterator replace(iterator pos, const value_type& v)
		{
			return replace(pos, pos + 1, std::span<const value_type>{&v, 1});
		}

		iterator replace(iterator first, iterator last, std::span<const value_type> buffer)
		{
			if (first == last)
				return insert(first, buffer);

			auto next_it = erase(first, last);

			return insert(next_it, buffer);
		}

		iterator replace(size_t pos, size_t count, std::span<const value_type> buffer)
		{
			return replace(begin() + pos, begin() + pos + count, buffer);
		}

		iterator replace(iterator pos, std::span<const value_type> buffer)
		{
			if (pos == end())
			{
				auto pivot = erase(pos - 1);
				return insert(pivot, buffer);
			}
			return replace(pos, pos + 1, buffer);
		}

		//

		iterator find_first_of(const std::span<value_type>& buffer)
		{
			if (buffer.empty() or empty())
				return end();

			for (auto it = begin(); it != end(); ++it)
			{
				if (std::find(buffer.begin(), buffer.end(), *it) != buffer.end())
					return it;
			}

			return end();
		}

		const_iterator find_first_of(std::string_view view) const
		{
			return find_first_of({as<const value_type*>(view.data()), view.size()});
		}

		iterator find_first_of(std::string_view view)
		{
			return find_first_of({as<const value_type*>(view.data()), view.size()});
		}

		auto subspan(size_t start, size_t count) const -> std::span<value_type>
		{
			assert::check(start < size(), "Index out-of-bounds");

			if (count == 0)
				return {};

			size_t N = std::min(count, size() - start);
			return std::span<value_type>(rawptr() + start, N);
		}

		auto subspan(size_t start) const -> std::span<value_type> { return subspan(start, size() - start); }

		auto subsbo(size_t start, size_t count) const -> sbo<SIZE> { return {sbo<SIZE>(subspan(start, count))}; }
	};

	// TODO: self insert check


	static_assert(sizeof(sbo<24>) == 24);
	static_assert(sizeof(sbo<32>) == 32);


} // namespace deckard

export namespace std
{
	using namespace deckard;

	template<size_t SIZE>
	struct hash<sbo<SIZE>>
	{
		size_t operator()(const sbo<SIZE>& obj) const { return utils::hash_values<u8>({obj.data().data(), obj.size()}); }
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
