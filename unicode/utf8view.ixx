export module deckard.utf8:view;

import :codepoints;
import :decode;

import std;
import deckard.types;
import deckard.utils.hash;
import deckard.assert;
import deckard.as;

namespace deckard::utf8
{
	template<typename T>
	concept subspannable = requires(T&& v) {
		{ v.subspan() } -> std::same_as<std::span<u8>>;
	};

	export class view
	{
	private:
		using type = u8;

		std::span<type> m_data;
		size_t          byte_index{0};

		void advance_to_next_codepoint(size_t& idx) const
		{

			if (idx >= m_data.size_bytes())
				return;

			auto next = idx;
			next++;

			while (next < m_data.size_bytes() and utf8::is_continuation_byte(m_data[next]))
				next++;

			while (next < m_data.size_bytes())
			{
				u8 byte = m_data[next];

				if (not utf8::is_single_byte(byte))
					break;

				if (not utf8::is_start_of_codepoint(byte))
					break;

				next++;
			}

			idx = next;
		}

		void reverse_to_last_codepoint(size_t& idx) const
		{

			if (idx > 0)
			{
				idx -= 1;

				while (idx > 0 and utf8::is_continuation_byte(m_data[idx]))
					idx -= 1;

				if (idx < 0)
					idx = 0;
			}
		}

		char32 decode_codepoint_at(size_t at) const
		{
			assert::check(at < m_data.size_bytes(), "Index out-of-bounds");

			auto   current   = at;
			u8     state     = 0;
			char32 codepoint = 0;

			for (; current < as<size_t>(m_data.size_bytes()); current++)
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

		char32 decode_current_codepoint() const { return decode_codepoint_at(byte_index); }

	public:
		view() = default;

		class string;

		view(subspannable auto&& str)
			: m_data(str.subspan())
			, byte_index(0uz)
		{
		}

		view(const std::span<u8> data)
			: m_data(data)
			, byte_index(0uz)
		{
		}

		view(std::string_view data)
			: m_data(as<u8*>(data.data()), data.size())
			, byte_index(0uz)
		{
		}

		view(const char* data, u32 len)
			: m_data(as<u8*>(data), len)
			, byte_index(0uz)
		{
		}

		size_t size_in_bytes() const { return m_data.size_bytes(); }

		size_t length() const
		{
			auto ret = utf8::length(m_data);
			return ret ? *ret : 0;
		}

		auto c_str() const { return as<const char*>(m_data.data()); }


		size_t size() const { return length(); }

		bool empty() const { return size() == 0uz; }

		bool is_valid() const
		{
			auto ret = utf8::length(m_data);
			return ret ? true : false;
		}

		bool operator==(const view& other) const
		{
			if (m_data.size() != other.m_data.size())
				return false;

			for (size_t i = 0; i < m_data.size(); ++i)
			{
				if (m_data[i] != other.m_data[i])
					return false;
			}
			return true;
		}

		auto operator*() const { return decode_current_codepoint(); }

		auto operator++()
		{
			advance_to_next_codepoint(byte_index);
			return *this;
		}

		auto operator++(int)
		{
			auto tmp = *this;
			advance_to_next_codepoint(byte_index);
			return tmp;
		}

		auto operator--()
		{
			reverse_to_last_codepoint(byte_index);
			return *this;
		}

		auto operator--(int)
		{
			auto tmp = *this;
			reverse_to_last_codepoint(byte_index);
			return tmp;
		}

		auto operator+=(int v)
		{
			while (v--)
				advance_to_next_codepoint(byte_index);

			return *this;
		}

		auto operator-=(int v)
		{
			while (v--)
				reverse_to_last_codepoint(byte_index);

			return *this;
		}

		const char32 at(size_t newindex) const
		{
			assert::check(newindex < size(), "Index out-of-bounds");

			size_t tmp = 0;

			for (size_t i = 0; i < newindex; ++i)
				advance_to_next_codepoint(tmp);

			return decode_codepoint_at(tmp);
		}

		constexpr auto operator[](this auto&& self, size_t idx)
		{

			assert::check(idx < self.size(), "Index out-of-bounds");

			size_t tmp = 0;

			for (size_t i = 0; i < idx; ++i)
				self.advance_to_next_codepoint(tmp);

			return self.decode_codepoint_at(tmp);
		}

		auto begin() const { return m_data.begin(); }

		auto end() const { return m_data.end(); }

		auto data() const { return m_data; }

		// TODO: iterator
	};

} // namespace deckard::utf8

export namespace std

{
	using namespace deckard;

	 template<>
	 struct hash<utf8::view>
	{
		size_t operator()(const utf8::view& value) const { return utils::hash_values(value.data()); }
	 };

	template<>
	struct formatter<utf8::view>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const utf8::view& v, std::format_context& ctx) const
		{
			std::string_view view{v.c_str(), v.size_in_bytes()};
			return std::format_to(ctx.out(), "{}", view);
		}
	};


} // namespace std
