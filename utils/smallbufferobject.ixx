export module deckard.sbo;

import std;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard
{
	export template<size_t SBO_CAPACITY = 32>
	union basic_smallbuffer
	{
		using type = u8;
		using pointer   = type*;
		using size_type = u32;

		struct SBO
		{
			type buffer[SBO_CAPACITY - 1]{0}; // data() bytes
			type len{sizeof(buffer)};         // (1, 31) when Small
		} sbo{};

		struct NonSBO
		{
			size_type size;
			size_type capacity;

			union
			{
				pointer ptr;

				struct
				{
					type padding[sizeof(SBO) - sizeof(pointer) - 1]; // never accessed via this reference
					type len;                                        // 0x00 when Big
				} ptrbytes;
			} buffer;
		} nonsbo;

		size_type insert(std::span<type> buffer)
		{
			const size_type len = as<size_type>(buffer.size_bytes());

			if (len <= sizeof(sbo.buffer))
			{
				std::memcpy(sbo.buffer, buffer.data(), len);
				sbo.len = len;
			}
			else
			{
				// TODO: reallocate just allocates, copy is another function
			}


			return len;
		}

		void resize(i32 newsize) { resize(as<size_type>(newsize)); }

		void resize(size_type newsize)
		{
		
			std::allocator<type> allocator{};
			// newsize *= 2;

			if (is_sbo() and newsize > sbo.len)
			{
				// reallocate to heap
				pointer newptr = allocator.allocate(newsize);
				if (newptr != nullptr)
				{
					size_type oldsize = size();
					std::memcpy(newptr, data(), oldsize);
					//
					nonsbo.size     = oldsize;
					nonsbo.capacity = newsize;

					sbo.len     = 0;
					pointer old = std::exchange(nonsbo.buffer.ptr, newptr);
					if (old != nullptr)
						allocator.deallocate(old, nonsbo.size);
				}
				return;
			}

			if (is_sbo() and newsize <= sbo.len)
			{
				sbo.len = as<type>(newsize);
				return;
			}

			if (not is_sbo() and newsize > nonsbo.size)
			{
				// heap to larger heap
				pointer newptr = allocator.allocate(newsize);
				if (newptr != nullptr)
				{
					//
					size_type oldsize = size();
					std::memcpy(newptr, data(), oldsize);
					nonsbo.size     = newsize;
					nonsbo.capacity = newsize;
					sbo.len         = 0;

					pointer old = std::exchange(nonsbo.buffer.ptr, newptr);
					if (old != nullptr)
						allocator.deallocate(old, oldsize);
				}
				return;
			}

			if (not is_sbo() and newsize <= nonsbo.size)
			{
				// heap to small buffer
				std::memcpy(sbo.buffer, nonsbo.buffer.ptr, newsize);

				allocator.deallocate(nonsbo.buffer.ptr, 0);
				nonsbo.size = nonsbo.capacity = 0;
				sbo.len                       = as<type>(newsize);
				return;
			}
		}

		void fill(char c, size_type count)
		{
			std::fill_n(data(), count, c);
			if (is_sbo())
				sbo.len = as<type>(count);
			else
			{
				sbo.len     = 0;
				nonsbo.size = count;
			}
		}

	
		bool is_sbo() const { return sbo.len > 0; }

		pointer data() { return is_sbo() ? sbo.buffer : nonsbo.buffer.ptr; }

		size_type size() const { return is_sbo() ? sbo.len : nonsbo.size; }

		size_type capacity() const { return is_sbo() ? sizeof(sbo.buffer) : nonsbo.capacity; }




	private:
	};
}
