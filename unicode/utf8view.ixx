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

	inline namespace v1
	{

		export class view
		{
		public:
			static constexpr size_t npos = static_cast<size_t>(-1);

		private:
			std::span<const u8> m_data{};

		public:
			class iterator
			{
			public:
				using iterator_concept  = std::random_access_iterator_tag;
				using iterator_category = std::random_access_iterator_tag;
				using value_type        = char32;
				using difference_type   = std::ptrdiff_t;
				using pointer           = void;
				using reference         = char32;

				constexpr iterator() = default;

				constexpr iterator(std::span<const u8> data, size_t byte_idx) noexcept
					: m_data(data)
					, m_byte_idx(byte_idx)
				{
					if (m_byte_idx < m_data.size())
					{
						fetch_current();
					}
				}

				[[nodiscard]] constexpr char32 operator*() const noexcept { return m_current_cp; }

				// Pre-increment
				constexpr iterator& operator++() noexcept
				{
					m_byte_idx += m_current_bytes;
					if (m_byte_idx < m_data.size())
					{
						fetch_current();
					}
					else
					{
						m_byte_idx = m_data.size();
					}
					return *this;
				}

				// Post-increment
				constexpr iterator operator++(int) noexcept
				{
					iterator temp = *this;
					++(*this);
					return temp;
				}

				// Pre-decrement
				constexpr iterator& operator--() noexcept
				{
					// Step back byte-by-byte until we hit a non-continuation byte
					if (m_byte_idx > 0)
					{
						--m_byte_idx;
						while (m_byte_idx > 0 && utf8::is_continuation_byte(m_data[m_byte_idx]))
						{
							--m_byte_idx;
						}
						fetch_current();
					}
					return *this;
				}

				// Post-decrement
				constexpr iterator operator--(int) noexcept
				{
					iterator temp = *this;
					--(*this);
					return temp;
				}

				// Compound addition assignment
				constexpr iterator& operator+=(difference_type offset) noexcept
				{
					if (offset > 0)
					{
						for (difference_type i = 0; i < offset && m_byte_idx < m_data.size(); ++i)
						{
							++(*this);
						}
					}
					else if (offset < 0)
					{
						for (difference_type i = 0; i < -offset && m_byte_idx > 0; ++i)
						{
							--(*this);
						}
					}
					return *this;
				}

				// Compound subtraction assignment
				constexpr iterator& operator-=(difference_type offset) noexcept { return *this += (-offset); }

				// Binary arithmetic operators
				[[nodiscard]] friend constexpr iterator operator+(iterator it, difference_type offset) noexcept
				{
					it += offset;
					return it;
				}

				[[nodiscard]] friend constexpr iterator operator+(difference_type offset, iterator it) noexcept
				{
					it += offset;
					return it;
				}

				[[nodiscard]] friend constexpr iterator operator-(iterator it, difference_type offset) noexcept
				{
					it -= offset;
					return it;
				}

				[[nodiscard]] friend constexpr difference_type operator-(const iterator& lhs, const iterator& rhs) noexcept
				{
					difference_type dist = 0;
					iterator        temp = rhs;
					if (rhs.m_byte_idx < lhs.m_byte_idx)
					{
						while (temp.m_byte_idx < lhs.m_byte_idx)
						{
							++temp;
							++dist;
						}
					}
					else
					{
						while (temp.m_byte_idx > lhs.m_byte_idx)
						{
							--temp;
							--dist;
						}
					}
					return dist;
				}

				// Structural comparisons
				[[nodiscard]] constexpr bool operator==(const iterator& other) const noexcept
				{
					return m_byte_idx == other.m_byte_idx;
				}

				[[nodiscard]] constexpr std::strong_ordering operator<=>(const iterator& other) const noexcept
				{
					return m_byte_idx <=> other.m_byte_idx;
				}

			private:
				constexpr void fetch_current() noexcept
				{
					auto [cp, bytes] = utf8::decode_unchecked(m_data, m_byte_idx);
					m_current_cp     = cp;
					m_current_bytes  = bytes > 0 ? bytes : 1uz;
				}

				std::span<const u8> m_data{};
				size_t              m_byte_idx{0};
				char32              m_current_cp{0};
				size_t              m_current_bytes{0};
			};

			using iterator               = iterator;
			using const_iterator         = iterator;
			using reverse_iterator       = std::reverse_iterator<iterator>;
			using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		public:
			constexpr view() = default;

			constexpr view(std::span<const u8> data)
				: m_data(data)
			{
			}

			template<size_t N>
			constexpr view(const std::array<u8, N>& arr) noexcept
				: m_data(arr.data(), N)
			{
			}

			template<size_t N>
			constexpr view(std::array<u8, N>& arr) noexcept
				: m_data(arr.data(), N)
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

			constexpr view(const string&) noexcept;
			constexpr view(const string&, size_t) noexcept;

			[[nodiscard]] constexpr iterator begin() const noexcept { return iterator(m_data, 0uz); }

			[[nodiscard]] constexpr iterator end() const noexcept { return iterator(m_data, m_data.size()); }

			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }

			[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

			[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }

			[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

			[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }

			[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

			// ##############
			// ##############


			[[nodiscard]] constexpr size_t operator-(const view& other) const
			{
				assert::check(m_data.data() >= other.m_data.data(), "Invalid view subtraction order or distinct buffers");

				size_t byte_offset = static_cast<size_t>(m_data.data() - other.m_data.data());

				view delta_view(other.m_data.data(), byte_offset);

				return delta_view.length();
			}

			constexpr view& operator++() noexcept
			{
				remove_prefix(1);
				return *this;
			}

			[[nodiscard]] constexpr view operator++(int) noexcept
			{
				view temp = *this;
				remove_prefix(1);
				return temp;
			}

			// ##############
			// ##############
			// ##############


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

			[[nodiscard]] constexpr bool valid() const
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

			[[nodiscard]] constexpr view subview(const view start, size_t codepoints) const noexcept
			{
				assert::check(
				  start.data().data() >= m_data.data() and start.data().data() <= m_data.data() + m_data.size_bytes(),
				  "The starting view anchor is outside the bounds of this view");

				const size_t byte_start  = static_cast<size_t>(start.data().data() - m_data.data());
				const size_t total_bytes = m_data.size_bytes();

				if (byte_start >= total_bytes or codepoints == 0)
				{
					return view{};
				}

				size_t byte_end        = byte_start;
				size_t count_extracted = 0;

				while (count_extracted < codepoints and byte_end < total_bytes)
				{
					auto [_, bytes] = utf8::decode_unchecked(m_data, byte_end);
					byte_end += bytes;
					count_extracted++;
				}

				return subview_bytes(byte_start, byte_end - byte_start);
			}

			[[nodiscard]] view subview(size_t codepoint_offset, size_t codepoint_count = npos) const
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

			[[nodiscard]] constexpr std::string_view as_string_view() const noexcept
			{
				return {reinterpret_cast<const char*>(m_data.data()), m_data.size_bytes()};
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

			[[nodiscard]] int compare(const view other) const
			{
				auto ord = std::lexicographical_compare_three_way(
				  m_data.begin(), m_data.end(), other.m_data.begin(), other.m_data.end());
				if (ord < 0)
					return -1;
				if (ord > 0)
					return 1;
				return 0;
			}

			[[nodiscard]] int compare(std::string_view other) const { return compare(view(other)); }

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

	} // namespace v1

	export inline std::ostream& operator<<(std::ostream& os, const utf8::view& s) { return os << s.to_string(); }


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
	struct formatter<utf8::v1::view>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const utf8::v1::view& v, std::format_context& ctx) const
		{
			std::string_view sv{reinterpret_cast<const char*>(v.data().data()), v.size_in_bytes()};
			return std::format_to(ctx.out(), "{}", sv);
		}
	};


} // namespace std
