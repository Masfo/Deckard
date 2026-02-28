export module deckard.utf8:view;

import :codepoints;
import :decode;

import std;
import deckard.types;
import deckard.utils.hash;
import deckard.assert;
import deckard.as;
import :utf8_span;

namespace deckard::utf8
{


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

			auto [codepoint, bytes] = utf8::decode_unchecked(utf8::as_ro_bytes(m_data), idx);
			idx += bytes;
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
			auto [codepoint, bytes] = utf8::decode_unchecked(utf8::as_ro_bytes(m_data), at);
			return codepoint;
		}

		char32 decode_current_codepoint() const { return decode_codepoint_at(byte_index); }

	public:
		view() = default;

		class string;

		view(const view begin, const view end)
			: m_data(begin.m_data.subspan(0, end - begin))
			, byte_index(0uz)
		{
		}

		view(subspannable auto&& str)
			: m_data(str.subspan())
			, byte_index(0uz)
		{
		}

		view(std::span<u8> data)
			: m_data(data)
			, byte_index(0uz)
		{
		}

		template<size_t N>
		view(std::array<u8, N>& data)
			: m_data(data)
			, byte_index(0uz)
		{
		}

		template<size_t N>
		view(const std::array<u8, N>& data)
			: m_data(as<u8*>(data.data()), N)
			, byte_index(0uz)
		{
		}

		view(std::string_view data)
			: m_data(as<u8*>(data.data()), data.size())
			, byte_index(0uz)
		{
		}

		size_t operator-(const view& other) const
		{
			assert::check(m_data.data() == other.m_data.data(), "Cannot subtract two different views");
			return byte_index - other.byte_index;
		}

		size_t index() const
		{
			size_t count{};

			size_t new_byte_index = 0;
			while(new_byte_index < byte_index)
			{
				advance_to_next_codepoint(new_byte_index);
				count++;
			}

			return count;
		}

		size_t size_in_bytes() const { return m_data.size_bytes(); }

		size_t length() const
		{
			auto ret = utf8::length(utf8::as_ro_bytes(m_data));
			return ret ? *ret : 0;
		}

		auto c_str() const { return as<const char*>(m_data.data()); }

		size_t size() const { return length(); }

		bool empty() const { return size() == 0uz; }

		bool is_valid() const
		{
			auto ret = utf8::length(utf8::as_ro_bytes(m_data));
			return ret ? true : false;
		}

		explicit operator bool() const { return has_next(); }

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

		auto& operator=(const view& input)
		{
			if (this != &input)
			{
				m_data     = input.m_data;
				byte_index = input.byte_index;
			}
			return *this;
		}

		bool has_next() const { return byte_index < m_data.size_bytes(); }

		u32 count_until_end() const
		{
			u32    count = 0;
			size_t idx   = byte_index;
			while (idx < m_data.size_bytes())
			{
				advance_to_next_codepoint(idx);
				count++;
			}
			return count;
		}

		std::optional<char32> peek(u32 offset = 1) const
		{

			size_t idx = byte_index;
			for (u32 i = 0; i < offset; ++i)
				advance_to_next_codepoint(idx);
			if (idx >= m_data.size_bytes())
				return std::nullopt;
			return decode_codepoint_at(idx);
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

		constexpr auto operator[](this const auto& self, size_t idx)
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

		view subview_bytes(size_t start_byte, size_t byte_length) const
		{
			assert::check(start_byte + byte_length <= m_data.size_bytes(), "Subview out-of-bounds");
			return view(m_data.subspan(start_byte, byte_length));
		}

		view subview_bytes(const view start, size_t byte_length) const
		{
			assert::check(m_data.data() == start.m_data.data(), "Cannot create subview from different data");
			assert::check(start.byte_index + byte_length <= m_data.size_bytes(), "Subview out-of-bounds");
			return view(m_data.subspan(start.byte_index, byte_length));
		}

		view subview(const view start, size_t codepoints) const
		{
			assert::check(m_data.data() == start.m_data.data(), "Cannot create subview from different data");

			size_t end_byte = start.byte_index;
			for (size_t i = 0; i < codepoints and end_byte < m_data.size_bytes(); ++i)
				advance_to_next_codepoint(end_byte);

			assert::check(end_byte <= m_data.size_bytes(), "Subview out-of-bounds");
			return view(m_data.subspan(start.byte_index, end_byte - start.byte_index));
		}

		view subview(size_t codepoints) const
		{
			return subview(*this, codepoints);
		}

		view subview(size_t start_codepoint, size_t count) const
		{
			size_t start_byte = 0;
			for (size_t i = 0; i < start_codepoint and start_byte < m_data.size_bytes(); ++i)
				advance_to_next_codepoint(start_byte);

			size_t end_byte = start_byte;
			for (size_t i = 0; i < count and end_byte < m_data.size_bytes(); ++i)
				advance_to_next_codepoint(end_byte);

			assert::check(end_byte <= m_data.size_bytes(), "Subview out-of-bounds");
			return view(m_data.subspan(start_byte, end_byte - start_byte));
		}


		auto span() const { return utf8::as_ro_bytes(m_data); }
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
