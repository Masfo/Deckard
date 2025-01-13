export module deckard.sbo;

import std;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard
{
	export template<size_t SBO_CAPACITY = 32>
	requires(SBO_CAPACITY >= 16)
	union basic_smallbuffer
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
			size_type size;
			size_type capacity;

			union
			{
				pointer ptr;

				struct
				{
					type padding[sizeof(SBO) - sizeof(pointer) - 2]; // never accessed via this reference
					type len;
					type is_sbo;                                     // Heap = 1
				} ptrbytes;
			} buffer;
		} nonsbo;

	private:
		void set_size(size_type size)
		{
			if (is_sbo())
				sbo.len = as<type>(capacity() - size);
			else
				nonsbo.size = size;
		}

	public:
		[[nodiscard("")]] reference operator[](size_t index)
		{
			assert::check(index < capacity(), "Indexing out-of-bounds");

			if (is_sbo())
				return sbo.buffer[index];
			else
				return nonsbo.buffer.ptr[index];
		}

		[[nodiscard("")]] const_reference operator[](size_t index) const
		{
			assert::check(index > capacity(), "Indexing out-of-bounds");

			if (is_sbo())
				return sbo.buffer[index];
			else
				return nonsbo.buffer.ptr[index];
		}

		size_type insert_into(size_t index, std::span<type> buffer)
		{
			const size_type len = as<size_type>(buffer.size_bytes());

			return 0;
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
		}

		void resize(size_type newsize)
		{

			if (is_sbo() and newsize <= sbo_size())
			{
				set_size(newsize);
				std::fill_n(data()+newsize, capacity()-newsize, 0);
				return;
			}
		}

		void fill(type value)
		{
			std::fill_n(data(), capacity(), value);
			set_size(capacity());
		}

		bool is_sbo() const { return sbo.is_sbo == 0; }

		pointer data() const { return is_sbo() ? sbo_data() : nonsbo_data(); }



		size_type size() const { return is_sbo() ? sbo_size() : nonsbo_size(); }



		size_type capacity() const { return is_sbo() ? sbo_capacity() : nonsbo_capacity(); }


	private:
		pointer sbo_data() const { return as<pointer>(sbo.buffer); }
		pointer nonsbo_data() const { return nonsbo.buffer.ptr; }
		size_type sbo_size() const { return sizeof(sbo.buffer) - sbo.len; }
		size_type nonsbo_size() const { return nonsbo.size; }

		size_type sbo_capacity() const { return sizeof(sbo.buffer); }

		size_type nonsbo_capacity() const { return nonsbo.capacity; }
	};
} // namespace deckard
