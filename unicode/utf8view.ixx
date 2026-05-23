export module deckard.utf8:view;

import :codepoints;
import :decode;
import :utf8_span;


import std;
import deckard.types;
import deckard.utils.hash;
import deckard.assert;
import deckard.as;

namespace deckard::utf8
{
	export class string;

	export class view
	{
	public:
		static constexpr size_t npos = std::string_view::npos;

	private:
		std::span<const u8> m_data;
		size_t              byte_index{0};

		void advance_to_next_codepoint(size_t& idx) const
		{
			if (idx >= m_data.size_bytes())
				return;

			auto [codepoint, bytes] = utf8::decode_unchecked(m_data, idx);
			idx += bytes;
		}

		void reverse_to_last_codepoint(size_t& idx) const
		{

			if (idx > 0)
			{
				idx -= 1;

				while (idx > 0 and utf8::is_continuation_byte(m_data[idx]))
				{
					idx -= 1;
					if (idx == 0)
						break;
				}
			}
		}

		char32 decode_codepoint_at(size_t at) const
		{
			assert::check(at < m_data.size_bytes(), "Index out-of-bounds");
			auto [codepoint, bytes] = utf8::decode_unchecked(m_data, at);
			return codepoint;
		}

		char32 decode_current_codepoint() const { return decode_codepoint_at(byte_index); }

	public:
		view() = default;

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

		view(std::span<const u8> data)
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
			: m_data(data.data(), N)
			, byte_index(0uz)
		{
		}

		view(const string&) noexcept; // in utf8.ixx

		view(std::string_view data)
			: m_data{reinterpret_cast<const u8*>(data.data()), data.size()}
			, byte_index(0uz)
		{
		}

		view(const char* data, size_t size)
			: m_data{reinterpret_cast<const u8*>(data), size}
			, byte_index(0uz)
		{
		}

		view(const u8* ptr, size_t size)
			: m_data{ptr, size}
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
			while (new_byte_index < byte_index)
			{
				advance_to_next_codepoint(new_byte_index);
				count++;
			}

			return count;
		}

		constexpr size_t size_in_bytes() const { return as<size_t>(m_data.size_bytes()); }

		[[nodiscard]] std::string_view as_string_view() const
		{
			return {reinterpret_cast<const char*>(m_data.data()), m_data.size_bytes()};
		}

		size_t length() const
		{
			auto ret = utf8::length(m_data);
			return ret ? as<size_t>(*ret) : 0;
		}

		size_t size() const { return length(); }

		constexpr bool empty() const noexcept { return m_data.empty(); }

		bool is_valid() const
		{
			if (empty())
				return true;

			auto ret = utf8::length(m_data);
			return ret ? true : false;
		}

		explicit operator bool() const { return has_next(); }

		bool operator==(const view& other) const { return compare(other) == 0; }

		bool operator==(std::string_view other) const { return compare(view(other)) == 0; }

		std::strong_ordering operator<=>(const view& other) const
		{
			int r = compare(other);
			if (r < 0)
				return std::strong_ordering::less;
			if (r > 0)
				return std::strong_ordering::greater;
			return std::strong_ordering::equal;
		}

		std::strong_ordering operator<=>(std::string_view other) const { return *this <=> view(other); }

		auto& operator=(const view& input)
		{
			if (this != &input)
			{
				m_data     = input.m_data;
				byte_index = input.byte_index;
			}
			return *this;
		}

		int compare(view other) const
		{
			auto ord =
			  std::lexicographical_compare_three_way(m_data.begin(), m_data.end(), other.m_data.begin(), other.m_data.end());
			if (ord < 0)
				return -1;
			if (ord > 0)
				return 1;
			return 0;
		}

		int compare(std::string_view other) const { return compare(view(other)); }

		// starts_with
		bool starts_with(char32 c) const
		{
			if (not has_next())
				return false;
			return decode_current_codepoint() == c;
		}

		bool starts_with(const view prefix) const
		{
			if (prefix.m_data.empty())
				return true;

			if (prefix.m_data.size_bytes() > m_data.size_bytes() - byte_index)
				return false;

			return std::string_view(as<const char*>(m_data.data() + byte_index), prefix.m_data.size_bytes()) ==
				   std::string_view(as<const char*>(prefix.m_data.data()), prefix.m_data.size_bytes());
		}

		bool starts_with(std::string_view prefix) const { return starts_with(view(prefix)); }

		// ends_with
		bool ends_with(char32 c) const
		{
			if (empty())
				return false;

			size_t idx = m_data.size_bytes();
			reverse_to_last_codepoint(idx);
			return decode_codepoint_at(idx) == c;
		}

		bool ends_with(const view suffix) const
		{
			if (suffix.m_data.empty())
				return true;
			if (suffix.m_data.size_bytes() > m_data.size_bytes() - byte_index)
				return false;

			return std::string_view(as<const char*>(m_data.data() + m_data.size_bytes() - suffix.m_data.size_bytes()),
									suffix.m_data.size_bytes()) ==
				   std::string_view(as<const char*>(suffix.m_data.data()), suffix.m_data.size_bytes());
		}

		bool ends_with(std::string_view suffix) const { return ends_with(view(suffix)); }

		//
		bool has_next() const { return byte_index < m_data.size_bytes(); }

		bool has_next(size_t codepoints) const
		{
			size_t idx = byte_index;
			for (size_t i = 0; i < codepoints; ++i)
			{
				if (idx >= m_data.size_bytes())
					return false;
				advance_to_next_codepoint(idx);
			}
			return true;
		}

		size_t remaining() const
		{
			size_t count = 0;
			size_t idx   = byte_index;
			while (idx < m_data.size_bytes())
			{
				advance_to_next_codepoint(idx);
				count++;
			}
			return count;
		}

		std::optional<char32> peek(size_t offset = 1) const
		{

			size_t idx = byte_index;
			for (size_t i = 0; i < offset; ++i)
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

		auto operator+=(size_t v)
		{
			while (v--)
				advance_to_next_codepoint(byte_index);

			return *this;
		}

		auto operator-=(size_t v)
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

		char32 front() const
		{
			assert::check(not empty(), "front() on empty view");
			return decode_codepoint_at(0);
		}

		char32 back() const
		{
			assert::check(not empty(), "back() on empty view");
			size_t idx = m_data.size_bytes();
			reverse_to_last_codepoint(idx);
			return decode_codepoint_at(idx);
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

			if (byte_length == 0)
				return view();

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

		view subview(size_t codepoints) const { return subview(*this, codepoints); }

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

		view subspan(size_t start_codepoint, size_t count) const { return subview(start_codepoint, count); }

		view substr(size_t pos = 0, size_t count = npos) const { return subview(pos, count); }

		view substr(size_t count) const { return subview(0, count); }

		void remove_prefix(size_t n)
		{
			size_t bytes = 0;
			for (size_t i = 0; i < n and bytes < m_data.size_bytes(); ++i)
				advance_to_next_codepoint(bytes);

			m_data = m_data.subspan(bytes);
			if (byte_index > bytes)
				byte_index -= bytes;
			else
				byte_index = 0;
		}

		void remove_suffix(size_t n)
		{
			size_t total_len = length();
			size_t to_keep   = (n >= total_len) ? 0 : (total_len - n);

			size_t bytes = 0;
			for (size_t i = 0; i < to_keep; ++i)
				advance_to_next_codepoint(bytes);

			m_data = m_data.subspan(0, bytes);
			if (byte_index > m_data.size_bytes())
				byte_index = m_data.size_bytes();
		}

		auto span() const { return m_data; }

		auto trim_left() const
		{
			size_t idx = 0;
			while (idx < m_data.size_bytes() and utf8::is_whitespace(m_data[idx]))
				idx++;
			return subview_bytes(idx, m_data.size_bytes() - idx);
		}

		auto trim_right() const
		{
			size_t idx = m_data.size_bytes();
			while (idx > 0 and std::isspace(m_data[idx - 1]))
				idx--;
			return subview_bytes(0, idx);
		}

		auto trim() const { return trim_left().trim_right(); }

		bool contains(char32 c) const { return find_first_of(c) != npos; }

		bool contains(view v) const { return find(v) != npos; }

		bool contains(std::string_view sv) const { return find(sv) != npos; }

		size_t find(char32 c, size_t pos = 0) const { return find_first_of(c, pos); }

		size_t find(view v, size_t pos = 0) const
		{
			if (v.empty())
				return pos <= length() ? pos : npos;

			size_t current_pos = 0;
			size_t idx         = 0;

			// Skip to pos
			while (current_pos < pos and idx < m_data.size_bytes())
			{
				advance_to_next_codepoint(idx);
				current_pos++;
			}

			std::string_view haystack{reinterpret_cast<const char*>(m_data.data()), m_data.size_bytes()};
			std::string_view needle{reinterpret_cast<const char*>(v.m_data.data()), v.m_data.size_bytes()};

			size_t found_byte = haystack.find(needle, idx);
			while (found_byte != std::string_view::npos)
			{
				if (utf8::is_start_of_codepoint(static_cast<u8>(m_data[found_byte])))
				{
					// Count codepoints from idx to found_byte
					while (idx < found_byte)
					{
						advance_to_next_codepoint(idx);
						current_pos++;
					}
					return current_pos;
				}
				found_byte = haystack.find(needle, found_byte + 1);
			}

			return npos;
		}

		size_t find(std::string_view sv, size_t pos = 0) const { return find(view(sv), pos); }

		size_t rfind(char32 c, size_t pos = npos) const { return find_last_of(c, pos); }

		size_t rfind(view v, size_t pos = npos) const
		{
			size_t len = length();
			if (v.empty())
				return std::min(pos, len);
			if (len == 0)
				return npos;

			size_t search_end_cp   = std::min(pos, len - 1);
			size_t search_end_byte = 0;
			for (size_t i = 0; i < search_end_cp and search_end_byte < m_data.size_bytes(); ++i)
				advance_to_next_codepoint(search_end_byte);

			std::string_view haystack{reinterpret_cast<const char*>(m_data.data()), m_data.size_bytes()};
			std::string_view needle{reinterpret_cast<const char*>(v.m_data.data()), v.m_data.size_bytes()};

			size_t found_byte = haystack.rfind(needle, search_end_byte);
			while (found_byte != std::string_view::npos)
			{
				if (utf8::is_start_of_codepoint(static_cast<u8>(m_data[found_byte])))
				{
					size_t cp_idx = 0;
					size_t b_idx  = 0;
					while (b_idx < found_byte)
					{
						advance_to_next_codepoint(b_idx);
						cp_idx++;
					}
					return cp_idx;
				}
				if (found_byte == 0)
					break;
				found_byte = haystack.rfind(needle, found_byte - 1);
			}

			return npos;
		}

		size_t rfind(std::string_view sv, size_t pos = npos) const { return rfind(view(sv), pos); }

		size_t find_first_of(char32 c, size_t pos = 0) const
		{
			size_t current_pos = 0;
			size_t idx         = 0;

			// Skip to pos
			while (current_pos < pos and idx < m_data.size_bytes())
			{
				advance_to_next_codepoint(idx);
				current_pos++;
			}

			while (idx < m_data.size_bytes())
			{
				auto [codepoint, bytes] = utf8::decode_unchecked(m_data, idx);
				if (codepoint == c)
					return current_pos;
				idx += bytes;
				current_pos++;
			}

			return npos;
		}

		size_t find_first_of_byte(char32 c, size_t byte_pos = 0) const
		{
			size_t idx = byte_pos;
			while (idx < m_data.size_bytes())
			{
				auto [codepoint, bytes] = utf8::decode_unchecked(m_data, idx);
				if (codepoint == c)
					return idx;
				idx += bytes;
			}
			return npos;
		}

		size_t find_first_of(view v, size_t pos = 0) const
		{
			size_t current_pos = 0;
			size_t idx         = 0;

			// Skip to pos
			while (current_pos < pos and idx < m_data.size_bytes())
			{
				advance_to_next_codepoint(idx);
				current_pos++;
			}

			while (idx < m_data.size_bytes())
			{
				auto [codepoint, bytes] = utf8::decode_unchecked(m_data, idx);
				if (v.contains(codepoint))
					return current_pos;
				idx += bytes;
				current_pos++;
			}

			return npos;
		}

		size_t find_last_of(char32 c, size_t pos = npos) const
		{
			size_t total_len = size();
			if (total_len == 0)
				return npos;

			size_t current_pos = std::min(pos, total_len - 1);
			size_t idx         = 0;

			// Go to the specified start position (pos)
			for (size_t i = 0; i < current_pos; ++i)
				advance_to_next_codepoint(idx);

			// Now search backwards
			while (true)
			{
				if (decode_codepoint_at(idx) == c)
					return current_pos;

				if (current_pos == 0)
					break;

				reverse_to_last_codepoint(idx);
				current_pos--;
			}

			return npos;
		}

		size_t find_last_of_byte(char32 c, size_t byte_pos = npos) const
		{
			if (m_data.empty())
				return npos;

			size_t idx = std::min(byte_pos, m_data.size_bytes() - 1);

			// Align to start of codepoint
			while (idx > 0 and not utf8::is_start_of_codepoint(static_cast<u8>(m_data[idx])))
				idx--;

			while (true)
			{
				if (decode_codepoint_at(idx) == c)
					return idx;

				if (idx == 0)
					break;

				reverse_to_last_codepoint(idx);
			}

			return npos;
		}

		size_t find_last_of(view v, size_t pos = npos) const
		{
			size_t total_len = size();
			if (total_len == 0)
				return npos;

			size_t current_pos = std::min(pos, total_len - 1);
			size_t idx         = 0;

			for (size_t i = 0; i < current_pos; ++i)
				advance_to_next_codepoint(idx);

			while (true)
			{
				if (v.contains(decode_codepoint_at(idx)))
					return current_pos;

				if (current_pos == 0)
					break;

				reverse_to_last_codepoint(idx);
				current_pos--;
			}

			return npos;
		}

		size_t find_first_not_of(char32 c, size_t pos = 0) const
		{
			size_t current_pos = 0;
			size_t idx         = 0;

			// Skip to pos
			while (current_pos < pos and idx < m_data.size_bytes())
			{
				advance_to_next_codepoint(idx);
				current_pos++;
			}

			while (idx < m_data.size_bytes())
			{
				auto [codepoint, bytes] = utf8::decode_unchecked(m_data, idx);
				if (codepoint != c)
					return current_pos;
				idx += bytes;
				current_pos++;
			}

			return npos;
		}

		size_t find_first_not_of(view v, size_t pos = 0) const
		{
			size_t current_pos = 0;
			size_t idx         = 0;

			// Skip to pos
			while (current_pos < pos and idx < m_data.size_bytes())
			{
				advance_to_next_codepoint(idx);
				current_pos++;
			}

			while (idx < m_data.size_bytes())
			{
				auto [codepoint, bytes] = utf8::decode_unchecked(m_data, idx);
				if (not v.contains(codepoint))
					return current_pos;
				idx += bytes;
				current_pos++;
			}

			return npos;
		}

		size_t find_first_not_of(std::string_view v, size_t pos = 0) const { return find_first_not_of(view(v), pos); }

		size_t find_last_not_of(char32 c, size_t pos = npos) const
		{
			size_t total_len = size();
			if (total_len == 0)
				return npos;

			size_t current_pos = std::min(pos, total_len - 1);
			size_t idx         = 0;

			// Go to the specified start position (pos)
			for (size_t i = 0; i < current_pos; ++i)
				advance_to_next_codepoint(idx);

			// Now search backwards
			while (true)
			{
				if (decode_codepoint_at(idx) != c)
					return current_pos;

				if (current_pos == 0)
					break;

				reverse_to_last_codepoint(idx);
				current_pos--;
			}

			return npos;
		}

		size_t find_last_not_of(view v, size_t pos = npos) const
		{
			size_t total_len = size();
			if (total_len == 0)
				return npos;

			size_t current_pos = std::min(pos, total_len - 1);
			size_t idx         = 0;

			for (size_t i = 0; i < current_pos; ++i)
				advance_to_next_codepoint(idx);

			while (true)
			{
				if (not v.contains(decode_codepoint_at(idx)))
					return current_pos;

				if (current_pos == 0)
					break;

				reverse_to_last_codepoint(idx);
				current_pos--;
			}

			return npos;
		}

		size_t find_last_not_of(std::string_view v, size_t pos = npos) const { return find_last_not_of(view(v), pos); }

		[[nodiscard]] std::string to_string() const
		{
			if (empty())
				return {};

			return std::string{reinterpret_cast<const char*>(m_data.data()), m_data.size_bytes()};
		}
	};

	export inline std::ostream& operator<<(std::ostream& os, const utf8::view& s) { return os << s.as_string_view(); }

	// ############################################################################################
	// ############################################################################################


	namespace v2
	{

		export class view
		{
		public:
			static constexpr size_t npos = static_cast<size_t>(-1);

		private:
			std::span<const u8> m_data{};

		public:
			constexpr view() = default;

			constexpr view(std::span<const u8> data)
				: m_data(data)
			{
			}

			constexpr view(std::string_view data)
				: m_data{reinterpret_cast<const u8*>(data.data()), data.size()}
			{
			}

			constexpr view(const char* data, size_t size)
				: m_data{reinterpret_cast<const u8*>(data), size}
			{
			}

			constexpr view(const u8* ptr, size_t size)
				: m_data{ptr, size}
			{
			}

			template<typename T>
			requires requires(T&& t) { t.subspan(); }
			constexpr view(T&& str)
				: m_data(str.subspan())
			{
			}

			[[nodiscard]] size_t graphemes() const { return grapheme_count(m_data); }

			[[nodiscard]] size_t size() const { return length(); }

			[[nodiscard]] constexpr size_t size_in_bytes() const { return m_data.size_bytes(); }

			[[nodiscard]] constexpr bool empty() const { return m_data.empty(); }

			[[nodiscard]] constexpr std::span<const u8> data() const { return m_data; }

			[[nodiscard]] constexpr size_t length() const
			{
				auto ret = utf8::length(m_data);
				return ret ? as<size_t>(*ret) : 0uz;
			}

			[[nodiscard]] constexpr bool is_valid() const
			{
				if (empty())
					return true;

				auto ret = utf8::length(m_data);
				return ret.has_value();
			}

			// Element access


			[[nodiscard]] char32 at(size_t index) const
			{
				assert::check(index < size(), "Index out-of-bounds");

				size_t byte_idx = 0;
				for (size_t i = 0; i < index; ++i)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
				}

				auto [codepoint, _] = utf8::decode_unchecked(m_data, byte_idx);
				return codepoint;
			}

			[[nodiscard]] char32 front() const
			{
				assert::check(not empty(), "front() on empty view");
				auto [cp, _] = utf8::decode_unchecked(m_data, 0uz);
				return cp;
			}

			[[nodiscard]] char32 back() const
			{
				assert::check(not empty(), "back() on empty view");
				size_t idx = m_data.size_bytes() - 1;
				while (idx > 0 and utf8::is_continuation_byte(m_data[idx]))
				{
					idx--;
				}
				auto [cp, _] = utf8::decode_unchecked(m_data, idx);
				return cp;
			}

			[[nodiscard]] char32 operator[](size_t idx) const
			{
				size_t byte_pos = 0;
				for (size_t i = 0; i < idx && byte_pos < m_data.size_bytes(); ++i)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_pos);
					byte_pos += bytes;
				}
				assert::check(byte_pos < m_data.size_bytes(), "Index out-of-bounds");
				auto [cp, _] = utf8::decode_unchecked(m_data, byte_pos);
				return cp;
			}

			// modifiers
			constexpr void remove_prefix(size_t n)
			{
				size_t byte_offset = 0;

				for (size_t i = 0; i < n && byte_offset < m_data.size_bytes(); ++i)
				{
					auto [_, bytes_consumed] = utf8::decode_unchecked(m_data, byte_offset);
					byte_offset += bytes_consumed;
				}

				m_data = m_data.subspan(byte_offset);
			}

			constexpr void remove_suffix(size_t n)
			{
				size_t total_len = length();
				size_t to_keep   = (n >= total_len) ? 0uz : (total_len - n);

				size_t byte_offset = 0;
				for (size_t i = 0; i < to_keep; ++i)
				{
					auto [_, bytes_consumed] = utf8::decode_unchecked(m_data, byte_offset);
					byte_offset += bytes_consumed;
				}

				m_data = m_data.subspan(0, byte_offset);
			}

			[[nodiscard]] view trim_left() const
			{
				size_t byte_idx = 0;
				while (byte_idx < m_data.size_bytes())
				{
					auto [codepoint, bytes] = utf8::decode_unchecked(m_data, byte_idx);

					if (not utf8::is_whitespace(codepoint))
						break;

					byte_idx += bytes;
				}

				return subview_bytes(byte_idx, m_data.size_bytes() - byte_idx);
			}

			[[nodiscard]] view trim_right() const
			{
				if (empty())
					return *this;

				size_t last_valid_byte_idx = m_data.size_bytes();

				while (last_valid_byte_idx > 0)
				{
					size_t prev_byte_idx = last_valid_byte_idx - 1;
					while (prev_byte_idx > 0 and utf8::is_continuation_byte(m_data[prev_byte_idx]))
					{
						prev_byte_idx--;
					}

					auto [codepoint, _] = utf8::decode_unchecked(m_data, prev_byte_idx);
					if (not utf8::is_whitespace(codepoint))
						break;

					last_valid_byte_idx = prev_byte_idx;
				}

				return subview_bytes(0, last_valid_byte_idx);
			}

			[[nodiscard]] view trim() const { return trim_left().trim_right(); }

			// Subviews
			[[nodiscard]] view subview_bytes(size_t start, size_t len) const
			{
				assert::check(start + len <= m_data.size_bytes(), "Subview out-of-bounds");
				return view(m_data.subspan(start, len));
			}

			[[nodiscard]] view sub_view(size_t codepoint_offset, size_t codepoint_count = npos) const
			{
				const size_t total_bytes = m_data.size_bytes();
				if (total_bytes == 0 or codepoint_count == 0)
					return view{};

				size_t byte_start        = 0;
				size_t current_codepoint = 0;

				while (current_codepoint < codepoint_offset and byte_start < total_bytes)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_start);
					byte_start += bytes;
					current_codepoint++;
				}

				if (byte_start >= total_bytes)
					return view{};

				size_t byte_end        = byte_start;
				size_t count_extracted = 0;
				while (count_extracted < codepoint_count and byte_end < total_bytes)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_end);
					byte_end += bytes;
					count_extracted++;
				}

				return subview_bytes(byte_start, byte_end - byte_start);
			}

			// implemented in utf8.ixx
			[[nodiscard]] string sub_str(size_t codepoint_offset, size_t codepoint_count = npos) const;

			// Comparison and search operators
			[[nodiscard]] bool operator==(const view& other) const { return std::ranges::equal(m_data, other.m_data); }

			[[nodiscard]] std::strong_ordering operator<=>(const view& other) const
			{
				return std::lexicographical_compare_three_way(
				  m_data.begin(), m_data.end(), other.m_data.begin(), other.m_data.end());
			}

			[[nodiscard]] bool contains(char32 c) const { return find(c) != npos; }

			[[nodiscard]] bool contains(view v) const { return find(v) != npos; }

			[[nodiscard]] bool contains(std::string_view sv) const { return contains(view(sv)); }

			[[nodiscard]] bool starts_with(char32 c) const
			{
				if (empty())
					return false;

				auto [codepoint, _] = utf8::decode_unchecked(m_data, 0uz);
				return codepoint == c;
			}

			[[nodiscard]] bool starts_with(const view prefix) const
			{
				if (prefix.m_data.empty())
					return true;
				if (prefix.m_data.size_bytes() > m_data.size_bytes())
					return false;

				return std::string_view(reinterpret_cast<const char*>(m_data.data()), prefix.m_data.size_bytes()) ==
					   std::string_view(reinterpret_cast<const char*>(prefix.m_data.data()), prefix.m_data.size_bytes());
			}

			[[nodiscard]] bool starts_with(std::string_view prefix) const { return starts_with(view(prefix)); }

			[[nodiscard]] bool ends_with(char32 c) const
			{
				if (empty())
					return false;

				size_t idx = m_data.size_bytes() - 1;
				while (idx > 0 and utf8::is_continuation_byte(m_data[idx]))
				{
					idx--;
				}

				auto [codepoint, _] = utf8::decode_unchecked(m_data, idx);
				return codepoint == c;
			}

			[[nodiscard]] bool ends_with(const view suffix) const
			{
				if (suffix.m_data.empty())
					return true;
				if (suffix.m_data.size_bytes() > m_data.size_bytes())
					return false;

				return std::string_view(
						 reinterpret_cast<const char*>(m_data.data() + m_data.size_bytes() - suffix.m_data.size_bytes()),
						 suffix.m_data.size_bytes()) ==
					   std::string_view(reinterpret_cast<const char*>(suffix.m_data.data()), suffix.m_data.size_bytes());
			}

			[[nodiscard]] bool ends_with(std::string_view suffix) const { return ends_with(view(suffix)); }

			// ###################################
			// ###################################

			[[nodiscard]] size_t find(char32 c, size_t pos = 0) const { return find_first_of(c, pos); }

			[[nodiscard]] size_t find(view v, size_t pos = 0) const
			{
				if (v.empty())
					return pos <= length() ? pos : npos;

				size_t current_cp = 0;
				size_t byte_idx   = 0;

				while (current_cp < pos and byte_idx < m_data.size_bytes())
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
					current_cp++;
				}

				std::string_view haystack{reinterpret_cast<const char*>(m_data.data()), m_data.size_bytes()};
				std::string_view needle{reinterpret_cast<const char*>(v.m_data.data()), v.m_data.size_bytes()};

				size_t found_byte = haystack.find(needle, byte_idx);
				while (found_byte != std::string_view::npos)
				{
					if (utf8::is_start_of_codepoint(m_data[found_byte]))
					{
						while (byte_idx < found_byte)
						{
							auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
							byte_idx += bytes;
							current_cp++;
						}
						return current_cp;
					}
					found_byte = haystack.find(needle, found_byte + 1);
				}
				return npos;
			}

			[[nodiscard]] size_t find(std::string_view sv, size_t pos = 0) const { return find(view(sv), pos); }

			[[nodiscard]] size_t rfind(char32 c, size_t pos = npos) const { return find_last_of(c, pos); }

			[[nodiscard]] size_t rfind(view v, size_t pos = npos) const
			{
				if (v.empty())
					return std::min(pos, length());
				if (empty())
					return npos;

				size_t search_end_cp   = std::min(pos, length() - 1);
				size_t search_end_byte = 0;
				for (size_t i = 0; i < search_end_cp and search_end_byte < m_data.size_bytes(); ++i)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, search_end_byte);
					search_end_byte += bytes;
				}

				std::string_view haystack{reinterpret_cast<const char*>(m_data.data()), m_data.size_bytes()};
				std::string_view needle{reinterpret_cast<const char*>(v.m_data.data()), v.m_data.size_bytes()};

				size_t found_byte = haystack.rfind(needle, search_end_byte);
				while (found_byte != std::string_view::npos)
				{
					if (utf8::is_start_of_codepoint(m_data[found_byte]))
					{
						size_t cp_idx = 0;
						size_t b_idx  = 0;
						while (b_idx < found_byte)
						{
							auto [_, bytes] = utf8::decode_unchecked(m_data, b_idx);
							b_idx += bytes;
							cp_idx++;
						}
						return cp_idx;
					}
					if (found_byte == 0)
						break;

					found_byte = haystack.rfind(needle, found_byte - 1);
				}
				return npos;
			}

			[[nodiscard]] size_t rfind(std::string_view sv, size_t pos = npos) const { return rfind(view(sv), pos); }

			[[nodiscard]] size_t find_first_of(char32 c, size_t pos = 0) const
			{
				size_t current_cp = 0;
				size_t byte_idx   = 0;

				while (current_cp < pos and byte_idx < m_data.size_bytes())
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
					current_cp++;
				}

				while (byte_idx < m_data.size_bytes())
				{
					auto [codepoint, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					if (codepoint == c)
						return current_cp;
					byte_idx += bytes;
					current_cp++;
				}
				return npos;
			}

			[[nodiscard]] size_t find_first_of(view v, size_t pos = 0) const
			{
				size_t current_cp = 0;
				size_t byte_idx   = 0;

				while (current_cp < pos and byte_idx < m_data.size_bytes())
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
					current_cp++;
				}

				while (byte_idx < m_data.size_bytes())
				{
					auto [codepoint, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					if (v.contains(codepoint))
						return current_cp;
					byte_idx += bytes;
					current_cp++;
				}
				return npos;
			}

			[[nodiscard]] size_t find_first_of(std::string_view sv, size_t pos = 0) const
			{
				return find_first_of(view(sv), pos);
			}

			// ###################################
			// ###################################

			[[nodiscard]] size_t find_first_not_of(char32 c, size_t pos = 0) const
			{
				size_t current_cp = 0;
				size_t byte_idx   = 0;

				while (current_cp < pos and byte_idx < m_data.size_bytes())
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
					current_cp++;
				}

				while (byte_idx < m_data.size_bytes())
				{
					auto [codepoint, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					if (codepoint != c)
						return current_cp;
					byte_idx += bytes;
					current_cp++;
				}
				return npos;
			}

			[[nodiscard]] size_t find_first_not_of(view v, size_t pos = 0) const
			{
				size_t current_cp = 0;
				size_t byte_idx   = 0;

				while (current_cp < pos and byte_idx < m_data.size_bytes())
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
					current_cp++;
				}

				while (byte_idx < m_data.size_bytes())
				{
					auto [codepoint, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					if (not v.contains(codepoint))
						return current_cp;
					byte_idx += bytes;
					current_cp++;
				}
				return npos;
			}

			[[nodiscard]] size_t find_first_not_of(std::string_view sv, size_t pos = 0) const
			{
				return find_first_not_of(view(sv), pos);
			}

			// ###################################
			// ###################################

			[[nodiscard]] size_t find_last_of(char32 c, size_t pos = npos) const
			{
				size_t total_cp = size();
				if (total_cp == 0)
					return npos;

				size_t current_cp = std::min(pos, total_cp - 1);
				size_t byte_idx   = 0;

				for (size_t i = 0; i < current_cp; ++i)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
				}

				while (true)
				{
					auto [codepoint, _] = utf8::decode_unchecked(m_data, byte_idx);
					if (codepoint == c)
						return current_cp;

					if (current_cp == 0)
						break;

					if (byte_idx > 0)
					{
						byte_idx--;
						while (byte_idx > 0 and utf8::is_continuation_byte(m_data[byte_idx]))
							byte_idx--;
					}
					current_cp--;
				}
				return npos;
			}

			[[nodiscard]] size_t find_last_of(view v, size_t pos = npos) const
			{
				size_t total_cp = size();
				if (total_cp == 0)
					return npos;

				size_t current_cp = std::min(pos, total_cp - 1);
				size_t byte_idx   = 0;

				for (size_t i = 0; i < current_cp; ++i)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
				}

				while (true)
				{
					auto [codepoint, _] = utf8::decode_unchecked(m_data, byte_idx);
					if (v.contains(codepoint))
						return current_cp;

					if (current_cp == 0)
						break;

					if (byte_idx > 0)
					{
						byte_idx--;
						while (byte_idx > 0 and utf8::is_continuation_byte(m_data[byte_idx]))
							byte_idx--;
					}
					current_cp--;
				}
				return npos;
			}

			[[nodiscard]] size_t find_last_of(std::string_view sv, size_t pos = npos) const
			{
				return find_last_of(view(sv), pos);
			}

			// ###################################
			// ###################################


			[[nodiscard]] size_t find_last_not_of(char32 c, size_t pos = npos) const
			{
				size_t total_cp = size();
				if (total_cp == 0)
					return npos;

				size_t current_cp = std::min(pos, total_cp - 1);
				size_t byte_idx   = 0;

				for (size_t i = 0; i < current_cp; ++i)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
				}

				while (true)
				{
					auto [codepoint, _] = utf8::decode_unchecked(m_data, byte_idx);
					if (codepoint != c)
						return current_cp;

					if (current_cp == 0)
						break;

					if (byte_idx > 0)
					{
						byte_idx--;
						while (byte_idx > 0 and utf8::is_continuation_byte(m_data[byte_idx]))
							byte_idx--;
					}
					current_cp--;
				}
				return npos;
			}

			[[nodiscard]] size_t find_last_not_of(view v, size_t pos = npos) const
			{
				size_t total_cp = size();
				if (total_cp == 0)
					return npos;

				size_t current_cp = std::min(pos, total_cp - 1);
				size_t byte_idx   = 0;

				for (size_t i = 0; i < current_cp; ++i)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_idx);
					byte_idx += bytes;
				}

				while (true)
				{
					auto [codepoint, _] = utf8::decode_unchecked(m_data, byte_idx);
					if (not v.contains(codepoint))
						return current_cp;

					if (current_cp == 0)
						break;

					if (byte_idx > 0)
					{
						byte_idx--;
						while (byte_idx > 0 and utf8::is_continuation_byte(m_data[byte_idx]))
							byte_idx--;
					}
					current_cp--;
				}
				return npos;
			}

			[[nodiscard]] size_t find_last_not_of(std::string_view sv, size_t pos = npos) const
			{
				return find_last_not_of(view(sv), pos);
			}

			[[nodiscard]] std::string to_string() const
			{
				if (empty())
					return {};

				return std::string{reinterpret_cast<const char*>(m_data.data()), m_data.size_bytes()};
			}

			// ###################################
			// ###################################

			//[[nodiscard]] string to_utf8_string() const;
		};

	} // namespace v2


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
			std::string_view sv{reinterpret_cast<const char*>(v.data().data()), v.size_in_bytes()};
			return std::format_to(ctx.out(), "{}", sv);
		}
	};

	// v2
	template<>
	struct formatter<deckard::utf8::v2::view>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const deckard::utf8::v2::view& v, std::format_context& ctx) const
		{
			std::string_view sv{reinterpret_cast<const char*>(v.data().data()), v.size_in_bytes()};
			return std::format_to(ctx.out(), "{}", sv);
		}
	};

} // namespace std
