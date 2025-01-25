export module deckard.sbo;

import std;
import deckard.types;
import deckard.as;
import deckard.assert;
import deckard.debug;
import deckard.math.utils;

namespace deckard
{
	namespace detail
	{
		template<int N>
		bool msb(unsigned char byte)
		{
			return byte & (1u << (8 - N - 1));
		}

		template<int N>
		bool lsb(unsigned char byte)
		{
			return byte & (1u << N);
		}

		template<int N>
		void set_msb(unsigned char& byte, bool bit)
		{
			if (bit)
				byte |= 1u << (8 - N - 1);
			else
				byte &= ~(1u << (8 - N - 1));
		}

		template<typename T>
		unsigned char& most_sig_byte(T& obj)
		{
			return *(reinterpret_cast<unsigned char*>(&obj) + sizeof(obj) - 1);
		}

	} // namespace detail

	struct fbsbo
	{
		using type            = u8;
		using pointer         = type*;
		using reference       = type&;
		using const_reference = const type&;
		using size_type       = u32;

		struct normal
		{
			size_type large : 1;
			size_type capacity : sizeof(size_type) * 8 - 1;
			size_type size;
			pointer   data;
		};

		struct sbo
		{
			type large : 1;
			type size : (sizeof(type) * 8) - 1;
			type padding[sizeof(size_type) - sizeof(type)];
			type data[sizeof(normal) - (sizeof(type)) * 3 - 1];
		};

		static_assert(sizeof(normal) == sizeof(sbo));

		union
		{
			sbo    small;
			normal large;
		} packed;

		bool is_large() const { return true; }

	public:
	};

	constexpr auto fbnormalsize = sizeof(fbsbo::normal);
	constexpr auto fbsbosize    = sizeof(fbsbo::sbo);

	export template<size_t SBO_CAPACITY = 32>
	requires(SBO_CAPACITY >= 16)
	class alignas(16) smallbuffer
	{
	public:
		using type            = u8;
		using pointer         = type*;
		using reference       = type&;
		using const_reference = const type&;
		using size_type       = u32;

		smallbuffer()
		{
			m_data.non_sbo.ptr      = nullptr;
			m_data.non_sbo.size     = 0;
			m_data.non_sbo.capacity = sbo_capacity;
		}

		std::size_t size() const noexcept
		{
			if (sbo())
				return sbo_size();
			else
				return read_non_sbo_data().first;
		}

		std::size_t capacity() const noexcept
		{
			if (sbo())
				return sizeof(m_data) - 1;
			else
				return read_non_sbo_data().second;
		}


	private:
		std::pair<std::size_t, std::size_t> read_non_sbo_data() const
		{
			auto size     = m_data.non_sbo.size;
			auto capacity = m_data.non_sbo.capacity;

			auto& size_hsb = detail::most_sig_byte(size);
			auto& cap_hsb  = detail::most_sig_byte(capacity);

			// Remember to negate the high bits
			auto const cap_high_bit     = detail::lsb<0>(cap_hsb);
			auto const size_high_bit    = !detail::lsb<1>(cap_hsb);
			auto const cap_sec_high_bit = detail::msb<0>(size_hsb);

			detail::set_msb<0>(size_hsb, size_high_bit);

			cap_hsb >>= 2;
			detail::set_msb<0>(cap_hsb, cap_high_bit);
			detail::set_msb<1>(cap_hsb, cap_sec_high_bit);

			return std::make_pair(size, capacity);
		}

		std::size_t sbo_size() const noexcept { return sbo_capacity - (sbo_capacity - ((m_data.sbo.size >> 2) & 63u)); }

		bool sbo() const noexcept { return not detail::lsb<0>(m_data.sbo.size) && not detail::lsb<1>(m_data.sbo.size); }

		union Data
		{
			struct NonSBO
			{
				pointer   ptr;
				size_type size;
				size_type capacity;
			} non_sbo;

			struct SBO
			{
				type data[sizeof(NonSBO) - (sizeof(NonSBO) - SBO_CAPACITY) - 1];
				type size;
			} sbo;
		} m_data;

		static std::size_t const sbo_capacity = sizeof(typename Data::SBO) - 1;

	public:
	};

	static_assert(sizeof(smallbuffer<16>) == 16);
	static_assert(sizeof(smallbuffer<32>) == 32);

	export template<size_t SBO_CAPACITY = 32>
	requires(SBO_CAPACITY >= 16)
	union alignas(16) basic_smallbuffer
	{
		using type            = u8;
		using pointer         = type*;
		using reference       = type&;
		using const_reference = const type&;
		using size_type       = u32;

		struct SBO
		{
			type buffer[SBO_CAPACITY - 2]{0}; // data() bytes
			type len{sizeof(buffer)};         //
			type is_sbo{0};                   // SBO = 0, Heap = 1
		} sbo{};

		struct NonSBO
		{
			size_type capacity;
			size_type size;

			union
			{
				pointer ptr;

				struct
				{
					type padding[sizeof(sbo.buffer) - sizeof(pointer) - 2]; // never accessed via this reference
					type len;
					type is_sbo;                                            // Heap = 1
				} ptrbytes;
			} buffer;


		} nonsbo;


	public:
		basic_smallbuffer() = default;

		~basic_smallbuffer()
		{
			if (not is_sbo())
			{

				// std::destroy_n(nonsbo.buffer.ptr, nonsbo.size);
				std::allocator<u8> allocator;
				auto               ptr_int = reinterpret_cast<u64>(nonsbo.buffer.ptr);

				dbg::println("dealloc nonsbo ptr: {:X}", address(nonsbo.buffer.ptr));


				u64 mask   = ((1ULL << 48) - 1);
				u64 masked = ptr_int & mask; // TODO: is is safe???

				auto ptr = as<pointer>(masked);

				dbg::println("dealloc masked ptr: {:X}", masked);


				allocator.deallocate(ptr, nonsbo.size);
				nonsbo.buffer.ptr = nullptr;
				nonsbo.size = nonsbo.capacity = 0;
			}
		}

		[[nodiscard("")]] reference operator[](size_t index)
		{
			assert::check(index < capacity(), "Indexing out-of-bounds");

			if (is_sbo())
				return sbo.buffer[index];
			else
			{
				assert::check(nonsbo.buffer.ptr != nullptr, "sbo buffer is null");
				return nonsbo.buffer.ptr[index];
			}
		}

		[[nodiscard("")]] const_reference operator[](size_t index) const
		{
			assert::check(index > capacity(), "Indexing out-of-bounds");

			if (is_sbo())
				return sbo.buffer[index];
			else
			{
				assert::check(nonsbo.buffer.ptr != nullptr, "sbo buffer is null");
				return nonsbo.buffer.ptr[index];
			}
		}

		void append(const std::span<type> buffer)
		{
			const size_type bufferlen = as<size_type>(buffer.size_bytes());

			size_type old_size = size();
			if (is_sbo() and (old_size + bufferlen) < as<type>(capacity()))
			{
				sbo.buffer[old_size] = buffer[0];
				set_size(old_size + 1);
			}
		}

		void push_back(const type value)
		{
			size_type old_size = size();
			if (is_sbo() and (old_size + 1) <= capacity())
			{
				sbo.buffer[old_size] = value;
				set_size(old_size + 1);
			}

			if (is_sbo() and (old_size + 1) > capacity())
			{
				resize(old_size * 2);
			}
		}

		void resize(size_type newsize)
		{
			size_type old_size = size();

			if (old_size == newsize)
				return;

			if (is_sbo() and newsize <= old_size)
			{
				set_size(newsize);
				std::fill_n(data() + newsize, capacity() - newsize, 0);
				return;
			}

			if (is_sbo() and newsize > old_size)
			{
				std::allocator<u8> allocator;

				auto newptr = allocator.allocate(newsize);

				assert::check(newptr != nullptr, "Could not allocate new ptr");

				std::uninitialized_fill(newptr, newptr + newsize, 0);
				std::copy(sbo_data(), sbo_data() + nonsbo_size(), newptr);

				dbg::println("newptr: {:X}", address(newptr));

				nonsbo.buffer.ptr = newptr;
				dbg::println("nonsbo ptr: {:X}", address(nonsbo.buffer.ptr));

				set_sbo(false);
				set_capacity(newsize);
				set_size(old_size);
			}
		}

		void fill(type value)
		{
			std::fill_n(data(), capacity(), value);
			set_size(capacity());
		}

		pointer data() const { return is_sbo() ? sbo_data() : nonsbo_data(); }

		size_type size() const { return is_sbo() ? sbo_size() : nonsbo_size(); }

		size_type capacity() const { return is_sbo() ? sbo_capacity() : nonsbo_capacity(); }


	private:
		bool is_sbo() const { return sbo.is_sbo == 0; }

		void set_sbo(bool state)
		{
			// SBO: 0, NonSBO: 1
			sbo.is_sbo = state ? 0 : 1;
		}

		void set_size(size_type size)
		{
			if (is_sbo())
				sbo.len = as<type>(capacity() - size);
			else
				nonsbo.size = size;
		}

		void set_capacity(size_type size)
		{
			if (not is_sbo())
			{
				nonsbo.capacity = size;
			}
		}

		pointer sbo_data() const { return as<pointer>(sbo.buffer); }

		pointer nonsbo_data() const { return nonsbo.buffer.ptr; }

		size_type sbo_size() const { return sizeof(sbo.buffer) - sbo.len; }

		size_type nonsbo_size() const { return nonsbo.size; }

		size_type sbo_capacity() const { return sizeof(sbo.buffer); }

		size_type nonsbo_capacity() const { return nonsbo.capacity; }
	};

	static_assert(sizeof(basic_smallbuffer<16>) == 16);
	static_assert(sizeof(basic_smallbuffer<32>) == 32);

	// ###########################################################################

	export namespace v2
	{


		export template<size_t SIZE = 32>
		requires(SIZE >= 24)
		class sbo
		{
			static_assert(SIZE >= 24, "Minimum size is 24 bytes");

		private:
			using type            = u8;
			using pointer         = type*;
			using reference       = type&;
			using const_reference = const type&;
			using size_type       = u32;

			struct small
			{
				std::array<type, SIZE - 1> data;

				type is_large : 1; // LSB
				type size : 7;     //
			};

			static_assert(sizeof(small) == SIZE);

			struct large
			{
				pointer   ptr;
				size_type size;
				size_type capacity;
				type      padding[sizeof(small) - sizeof(ptr) - sizeof(size) - sizeof(capacity) - 1];

				type is_large : 1; // LSB
				type unused : 7;   //
			};

			static_assert(sizeof(large) == sizeof(small));

			union
			{
				small small;
				large large;
			} packed;

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

		public:
			sbo() { reset(); }

			// Copy
			sbo(sbo const& other) { clone(other); }

			sbo& operator=(sbo const& other)
			{
				if (this == &other)
					return *this;

				clone(other);
				return *this;
			}

			// Move
			sbo(sbo&& other) { move(other); }

			sbo& operator=(sbo&& other)
			{
				if (this == &other)
					return *this;
				move(other);
				return *this;
			}

			~sbo() noexcept { reset(); }

			bool empty() const { return size() == 0; }

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

			type front() const
			{
				assert::check(empty(), "Front called on empty SBO");

				return rawptr()[0];
			}

			type back() const
			{
				assert::check(empty(), "Back called on empty SBO");

				return rawptr()[size() - 1];
			}

			size_t size() const { return is_large() ? large_size() : small_size(); }

			size_t capacity() const { return is_large() ? large_capacity() : small_capacity(); }

			[[nodiscard("")]] reference operator[](size_t index)
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

			[[nodiscard("")]] const_reference operator[](size_t index) const
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
					resize(math::ceil_pow2(current_capacity) * 2);
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
					size_t  new_capacity = std::max(newsize, math::ceil_pow2(capacity()) * 2);
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

			std::span<type> data() const
			{
				if (is_large())
				{
					return {packed.large.ptr, packed.large.size};
				}
				else
				{
					return {(pointer)packed.small.data.data(), packed.small.size};
				}
			}
		};

		static_assert(sizeof(sbo<24>) == 24);
		static_assert(sizeof(sbo<32>) == 32);

	}; // namespace v2

} // namespace deckard
