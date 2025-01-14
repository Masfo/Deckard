export module deckard.sbo;

import std;
import deckard.types;
import deckard.as;
import deckard.assert;
import deckard.debug;

namespace deckard
{
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
					type padding[sizeof(sbo.buffer) - sizeof(pointer)-2]; // never accessed via this reference
					type len;
					type is_sbo;                                        // Heap = 1
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

				u64 mask   = ((1ULL << 48) - 1);
				u64 masked = ptr_int & mask; // TODO: is is safe???

				auto ptr = as<pointer>(masked);

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

} // namespace deckard
