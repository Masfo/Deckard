export module deckard.utf8:view;

import :codepoints;
import :decode;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;

namespace deckard::utf8
{

	export class view
	{
	private:
		using type = u8;

		std::span<type> m_data;
		i64             index{0};

		void advance_to_next_codepoint()
		{

			if (index >= as<i64>(m_data.size_bytes()))
				return;

			auto next = index;
			next++;

			while (next < as<i64>(m_data.size_bytes()) and utf8::is_continuation_byte(m_data[next]))
				next++;

			while (next < as<i64>(m_data.size_bytes()))
			{
				u8 byte = m_data[next];

				if (not utf8::is_single_byte(byte))
					break;

				if (not utf8::is_start_of_codepoint(byte))
					break;

				next++;
			}

			index = next;
		}

		void reverse_to_last_codepoint()
		{

			if (index > 0)
			{
				index -= 1;

				assert::check(index < as<i64>(m_data.size_bytes()), "Index out-of-bounds");

				while (index > 0 and utf8::is_continuation_byte(m_data[index]))
					index -= 1;

				if (index < 0)
					index = 0;
			}
		}

		char32 decode_current_codepoint() const
		{
			assert::check(index < as<i64>(m_data.size_bytes()), "Index out-of-bounds");

			auto   current   = index;
			u8    state     = 0;
			char32 codepoint = 0;

			for (; current < as<i64>(m_data.size_bytes()); current++)
			{
				u8 byte = m_data[current];

				const u8 type = utf8_table[byte];
				codepoint     = state ? (byte & 0x3fu) | (codepoint << 6) : (0xffu >> type) & byte;
				state         = utf8_table[256 + state + type];

				if (state == 0)
					return codepoint;
				else if (state == UTF8_REJECT)
					return REPLACEMENT_CHARACTER;
			}
			return REPLACEMENT_CHARACTER;
		}

	public:
		view(std::span<u8> data)
			: m_data(data)
			, index(0)
		{
		}

		view(std::string_view data)
			: m_data(as<u8*>(data.data()), as<u32>(data.size()))
			, index(0)
		{
		}

		view(const char* data, u32 len)
			: m_data(as<u8*>(data), len)
			, index(0)
		{
		}

		size_t size_in_bytes() const { return m_data.size_bytes(); }

		size_t length() const
		{
			auto ret = utf8::length(m_data);
			return ret ? *ret : 0;

			size_t len = 0;
			for (size_t i = 0; i < m_data.size(); ++i)
			{
				if (not utf8::is_continuation_byte(m_data[i]))
					++len;
			}
			return len;
		}

		size_t size() const { return length(); }

		bool is_valid() const
		{
			auto ret = utf8::length(m_data);
			return ret ? true : false;
		}

		// operators ++,--, []
		//
	};

} // namespace deckard::utf8
