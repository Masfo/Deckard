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
		std::span<u8> m_data;
		size_t        index{0};

		void advance_to_next_codepoint() { }

		void reverse_to_last_codepoint() { }

		char32 decode_current_index_to_codepoint() { return REPLACEMENT_CHARACTER; }
	public:
		view(std::span<u8> data)
			: m_data(data), index(0)
		{
		}

		view(std::string_view data)
			: m_data(as<u8*>(data.data()), as<u32>(data.size())), index(0)
		{
		}

		view(const char* data, u32 len)
			: m_data(as<u8*>(data), len), index(0)
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
