export module deckard.sbo;

import std;
import deckard.types;
import deckard.as;
import deckard.assert;
import deckard.debug;
import deckard.math.utils;

namespace deckard
{


	export template<size_t SIZE = 32>
	class sbo
	{
		static_assert(SIZE >= 24, "Minimum size is 24 bytes");
		static constexpr f32    SCALE_FACTOR = 1.5f;
		static constexpr size_t ALIGNMENT    = 8;

	private:
		using type            = u8;
		using pointer         = type*;
		using reference       = type&;
		using const_pointer   = const pointer;
		using const_reference = const reference;
		using size_type       = u32;

		struct small
		{
			std::array<type, SIZE - 1> data;

			type is_large : 1;
			type size : 7;
		};

		static_assert(sizeof(small) == SIZE);

		struct large
		{
			pointer   ptr;
			size_type size;
			size_type capacity;
			type      padding[sizeof(small) - sizeof(ptr) - sizeof(size) - sizeof(capacity) - 1];

			type is_large : 1;
			type unused : 7;
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
			using iterator_category = std::random_access_iterator_tag;
			using value_type        = type;
			using difference_type   = std::ptrdiff_t;

			iterator(const_iterator ci)
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

			//
			// bool operator!=(const const_iterator& other) const { return ptr != other.ptr; }
			//
			// bool operator<(const const_iterator& other) const { return ptr < other.ptr; }
			//
			// bool operator>(const const_iterator& other) const { return ptr > other.ptr; }
			//
			// bool operator<=(const const_iterator& other) const { return ptr <= other.ptr; }
			//
			// bool operator>=(const const_iterator& other) const { return ptr >= other.ptr; }
		};

		class const_iterator
		{
		private:
			pointer ptr;

		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type        = type;
			using difference_type   = std::ptrdiff_t;

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

			//
			// bool operator!=(const const_iterator& other) const { return ptr != other.ptr; }
			//
			// bool operator<(const const_iterator& other) const { return ptr < other.ptr; }
			//
			// bool operator>(const const_iterator& other) const { return ptr > other.ptr; }
			//
			// bool operator<=(const const_iterator& other) const { return ptr <= other.ptr; }
			//
			// bool operator>=(const const_iterator& other) const { return ptr >= other.ptr; }
		};

		void set_size(size_t newsize)
		{
			if (is_large())
				packed.large.size = as<size_type>(newsize);
			else
				packed.small.size = as<type>(newsize);
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

		std::span<type> large_data() const { return {packed.large.ptr, packed.large.size}; }

		std::span<type> small_data() const { return {(pointer)packed.small.data.data(), packed.small.size}; }

		void reset()
		{
			if (is_large())
			{
				pointer ptr = large_rawptr();
				std::fill(ptr, ptr + large_size(), 0);

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

				pointer newptr = new type[packed.large.capacity];

				if (newptr != nullptr)
				{
					std::copy(from.packed.large.ptr, from.packed.large.ptr + large_size(), newptr);
					packed.large.ptr = newptr;
				}
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

				other.packed.large.is_large = 0;
			}
			other.reset();
		}

		size_t newcapacity_size(size_t size) const { return math::align_integer(as<size_t>(SCALE_FACTOR * size), ALIGNMENT); }

	public:
		sbo() { reset(); }

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

		void swap(sbo& other) noexcept
		{
			std::swap(packed, other.packed);
		}

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

		iterator insert(const_iterator pos, const std::span<type> buffer)
		{
			//
			todo();

			return pos;
		}

		iterator insert(const_iterator pos, const type& v) { return insert(pos, {as<pointer>(&v), 1}); }

		iterator insert(iterator pos, const std::span<type> buffer)
		{

			if (buffer.empty())
				return pos;

			//
			const auto pivot   = std::distance(begin(), pos);
			size_t     newsize = size() + buffer.size();
			if (newsize > capacity())
				resize(newsize);

			const pointer ptr = rawptr();
			pos               = begin() + pivot;

			std::copy_backward(ptr + pivot, ptr + size(), ptr + newsize);
			std::copy(buffer.data(), buffer.data() + buffer.size(), ptr + pivot);

			set_size(newsize);

			return pos;
		}

		iterator insert(iterator pos, const type& v) { return insert(pos, {as<pointer>(&v), 1}); }

		void append(const sbo<SIZE>& other) { append(other.data()); }

		void append(const std::span<type> buffer)
		{
			if (buffer.empty())
				return;

			// TODO: resize + copy

			if (available_size() < buffer.size())
				resize(size() + buffer.size());

			pointer ptr = rawptr();

			std::copy(buffer.data(), buffer.data() + buffer.size(), ptr + size());

			if (is_small())
				packed.small.size += as<type>(buffer.size());
			else
				packed.large.size += as<size_type>(buffer.size());
		}

		[[nodiscard("Use the front value")]] type front() const
		{
			assert::check(not empty(), "Front called on empty SBO");

			return rawptr()[0];
		}

		[[nodiscard("Use the back value")]] type back() const
		{
			assert::check(not empty(), "Back called on empty SBO");

			return rawptr()[size() - 1];
		}

		size_t size() const { return is_large() ? large_size() : small_size(); }

		size_t capacity() const { return is_large() ? large_capacity() : small_capacity(); }

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
				return packed.small.data[index];
			}
		}

		void fill(const type& v)
		{
			const pointer   ptr = rawptr();
			const size_type max = capacity();

			for (size_type i = 0; i < max; ++i)
				ptr[i] = v;
		}

		void push_back(const type& v)
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

			if (newsize > capacity())
			{
				size_t  new_capacity = std::max(newsize, newcapacity_size(capacity()));
				pointer newptr       = new type[new_capacity];
				std::fill(newptr, newptr + new_capacity, 0);

				pointer oldptr = rawptr();
				if (newptr != nullptr)
				{
					std::copy(oldptr, oldptr + std::min(oldsize, newsize), newptr);
					if (is_large())
						delete[] packed.large.ptr;

					packed.large.ptr = newptr;
					set_large(true);
					set_capacity(new_capacity);
					set_size(oldsize);
				}
			}
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
					std::copy(ptr, ptr + oldsize, newptr);

					set_large(false);
					set_size(as<type>(oldsize));
					return;
				}

				if (oldsize < large_capacity())
				{
					pointer newptr = new type[oldsize];
					std::fill(newptr, newptr + oldsize, 0);

					pointer oldptr = large_rawptr();
					if (newptr != nullptr)
					{
						std::copy(oldptr, oldptr + oldsize, newptr);
						if (is_large())
							delete[] packed.large.ptr;

						packed.large.ptr = newptr;
						set_large(true);
						set_capacity(oldsize);
						set_size(oldsize);
					}
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

		[[nodiscard("Use the data span")]] std::span<type> data() const
		{
			assert::check(not empty(), "Data span cannot be empty");

			if (is_large())
			{
				return {packed.large.ptr, packed.large.size};
			}
			else
			{
				return {(pointer)packed.small.data.data(), packed.small.size};
			}
		}

		// iterator

		bool contains(const type& value) const { return find(value) != cend(); }

		iterator begin() { return iterator(rawptr()); }

		iterator end() { return iterator(end_rawptr()); }

		const_iterator begin() const { return const_iterator(rawptr()); }

		const_iterator end() const { return const_iterator(end_rawptr()); }

		const_iterator cbegin() const { return const_iterator(rawptr()); }

		const_iterator cend() const { return const_iterator(end_rawptr()); }

		iterator find(const type& value)
		{
			auto it = begin();
			for (; it != end(); ++it)
			{
				if (*it == value)
					return it;
			}
			return end();
		}

		const_iterator find(const type& value) const
		{
			auto it = cbegin();
			for (; it != cend(); ++it)
			{
				if (*it == value)
					return it;
			}
			return cend();
		}

		iterator find(const std::span<type>& buffer)
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

		const_iterator find(const std::span<type>& buffer) const
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
	};

	static_assert(sizeof(sbo<24>) == 24);
	static_assert(sizeof(sbo<32>) == 32);


} // namespace deckard
