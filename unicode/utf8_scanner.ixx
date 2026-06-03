export module deckard.utf8:scanner;

import :codepoints;
import :decode;
import :view;

import std;
import deckard.types;
import deckard.assert;
import deckard.helpers;

namespace deckard::utf8
{
	export class scanner
	{
	private:
		std::span<const u8> m_data;
		size_t              m_pos{0};

	public:
		scanner() = default;

		explicit scanner(view v) noexcept
			: m_data{v.data()}
			, m_pos{0}
		{
		}

		explicit scanner(std::span<const u8> s) noexcept
			: m_data{s}
			, m_pos{0}
		{
		}

		explicit scanner(std::string_view sv) noexcept
			: m_data{reinterpret_cast<const u8*>(sv.data()), sv.size()}
			, m_pos{0}
		{
		}

		[[nodiscard]] char32 decode_at(size_t byte_pos) const noexcept
		{
			if (byte_pos >= m_data.size())
				return REPLACEMENT_CHARACTER;
			auto [cp, _] = utf8::decode_unchecked(m_data, byte_pos);
			return cp;
		}

		[[nodiscard]] u32 width_at(size_t byte_pos) const noexcept
		{
			if (byte_pos >= m_data.size())
				return 0;
			auto [_, bytes] = utf8::decode_unchecked(m_data, byte_pos);

			assert::check(bytes > 0, "Should never be zero");

			return bytes;
		}

		// positions

		constexpr void seek_to_byte_position(size_t pos) noexcept
		{
			assert::check(pos <= m_data.size(), "scanner::seek_to_byte_position out of bounds");
			m_pos = pos;
		}

		constexpr scanner& operator--() noexcept
		{
			if (m_pos > 0)
			{
				--m_pos;
				while (m_pos > 0 and utf8::is_continuation_byte(m_data[m_pos]))
					--m_pos;
			}
			return *this;
		}

		constexpr scanner operator--(int) noexcept
		{
			scanner temp = *this;
			--(*this);
			return temp;
		}

		constexpr scanner& operator-=(ptrdiff_t n) noexcept
		{
			assert::check(n >= 0, "scanner::operator-= does not support negative step");

			for (ptrdiff_t i = 0; i < n and m_pos > 0; ++i)
				--(*this);

			return *this;
		}

		constexpr auto operator+=(size_t n) noexcept
		{
			skip(n);
			return *this;
		}

		[[nodiscard]] constexpr scanner operator+(size_t n) const noexcept
		{
			scanner temp = *this;
			temp += n;
			return temp;
		}

		[[nodiscard]] constexpr scanner operator-(ptrdiff_t n) const noexcept
		{
			scanner temp = *this;
			temp -= n;
			return temp;
		}

		[[nodiscard]] constexpr std::ptrdiff_t operator-(const scanner& other) const noexcept
		{
			assert::check(m_data.data() == other.m_data.data(), "Scanners must share the same underlying buffer");
			return static_cast<std::ptrdiff_t>(m_pos) - static_cast<std::ptrdiff_t>(other.m_pos);
		}

		[[nodiscard]] constexpr size_t operator-(const view& other) const noexcept
		{
			const u8* current_ptr = m_data.data() + m_pos;
			assert::check(
			  current_ptr >= other.data().data() and current_ptr <= other.data().data() + other.data().size_bytes(),
			  "scanner::operator-(view): scanner position does not point into the view's span");
			return static_cast<size_t>(current_ptr - other.data().data());
		}

		[[nodiscard]] friend constexpr scanner operator+(size_t n, const scanner& s) noexcept { return s + n; }

		[[nodiscard]] bool has_next() const noexcept { return m_pos < m_data.size(); }

		[[nodiscard]] explicit operator bool() const noexcept { return has_next(); }

		[[nodiscard]] size_t byte_position() const noexcept { return m_pos; }

		[[nodiscard]] size_t remaining_bytes() const noexcept { return m_data.size() - m_pos; }

		// total codepoints in the whole buffer
		[[nodiscard]] size_t size() const noexcept
		{
			size_t count{0};
			for (size_t i = 0; i < m_data.size(); ++i)
				count += utf8::is_start_byte(m_data[i]) ? 1 : 0;
			return count;
		}

		// codepoints remaining from current position to end
		[[nodiscard]] size_t remaining() const noexcept
		{
			size_t count{0};
			for (size_t i = m_pos; i < m_data.size(); ++i)
				count += utf8::is_start_byte(m_data[i]) ? 1 : 0;

			return count;
		}

		[[nodiscard]] bool at_end() const noexcept { return not has_next(); }

		//
		[[nodiscard]] char32 current() const noexcept { return decode_at(m_pos); }

		[[nodiscard]] char32 operator*() const noexcept { return current(); }

		[[nodiscard]] constexpr char32 peek_back() const noexcept
		{
			if (m_pos == 0)
				return REPLACEMENT_CHARACTER;

			size_t back_pos = m_pos;

			do
			{
				--back_pos;
			} while (back_pos > 0 and not utf8::is_start_byte(m_data[back_pos]));

			auto [cp, _] = utf8::decode_unchecked(m_data, back_pos);
			return cp;
		}

		[[nodiscard]] std::optional<char32> peek(size_t n = 1) const noexcept
		{
			size_t idx = m_pos;
			for (size_t i = 0; i < n and idx < m_data.size(); ++i)
				idx += width_at(idx);

			if (idx >= m_data.size())
				return std::nullopt;

			return decode_at(idx);
		}

		[[nodiscard]] bool is(char32 c) const noexcept { return has_next() and current() == c; }

		[[nodiscard]] bool is(auto&& pred) const noexcept
		requires std::predicate<decltype(pred), char32>
		{
			return has_next() and pred(current());
		}

		//  moving positions

		char32 next()
		{
			assert::check(has_next(), "scanner::next() called at end of input");

			const auto [codepoint, width] = utf8::decode_unchecked(m_data, m_pos);
			assert::check(width > 0, "Should never be zero");

			m_pos += width;
			return codepoint;
		}

		[[nodiscard]] std::optional<char32> try_next() noexcept
		{
			if (not has_next())
				return std::nullopt;

			const auto [codepoint, width] = utf8::decode_unchecked(m_data, m_pos);

			assert::check(width > 0, "Should never be zero");

			m_pos += width;
			return codepoint;
		}

		void skip() noexcept
		{
			assert::check(has_next(), "scanner::skip() called at end of input");
			m_pos += width_at(m_pos);
		}

		template<character_type T>
		bool skip(T c) noexcept
		{
			if (is(c))
			{
				skip();
				return true;
			}
			return false;
		}

		void skip(size_t n) noexcept
		{
			while (n-- > 0 and has_next())
				m_pos += width_at(m_pos);
		}

		scanner& operator++()
		{
			skip();
			return *this;
		}

		char32 operator++(int)
		{
			assert::check(has_next(), "scanner::operator++(int) called at end of input");
			const char32 cp = current();
			skip();
			return cp;
		}

		//

		bool skip_if(char32 c) noexcept
		{
			if (not is(c))
				return false;
			m_pos += width_at(m_pos);
			return true;
		}

		bool skip_if(auto&& pred) noexcept
		requires std::predicate<decltype(pred), char32>
		{
			if (not is(pred))
				return false;
			m_pos += width_at(m_pos);
			return true;
		}

		size_t skip_while(auto&& pred) noexcept
		requires std::predicate<decltype(pred), char32>
		{
			size_t count = 0;
			while (has_next() and pred(current()))
			{
				m_pos += width_at(m_pos);
				++count;
			}
			return count;
		}

		size_t skip_whitespace() noexcept
		{
			return skip_while([](char32 cp) { return utf8::is_whitespace(cp); });
		}

		[[nodiscard]] view take(size_t n) noexcept
		{
			const size_t start = m_pos;
			skip(n);
			return view{m_data.subspan(start, m_pos - start)};
		}

		[[nodiscard]] view take_while(auto&& pred) noexcept
		requires std::predicate<decltype(pred), char32>
		{
			const size_t start = m_pos;
			skip_while(pred);
			return view{m_data.subspan(start, m_pos - start)};
		}

		[[nodiscard]] view take_until(char32 c) noexcept
		{
			return take_while([c](char32 cp) { return cp != c; });
		}

		[[nodiscard]] view take_until(auto&& pred) noexcept
		requires std::predicate<decltype(pred), char32>
		{
			return take_while([&pred](char32 cp) { return not pred(cp); });
		}

		// Takes line, skips newline characters
		[[nodiscard]] view take_line() noexcept
		{
			using namespace utf8::basic_characters;
			const size_t        start     = m_pos;
			std::span<const u8> remaining = m_data.subspan(m_pos);

			auto it = std::ranges::find_if(remaining, [](u8 b) { return b == u8'\n' || b == u8'\r'; });

			if (it == remaining.end())
			{
				m_pos = m_data.size();
				return view{m_data.subspan(start)};
			}

			size_t newline_pos = start + std::distance(remaining.begin(), it);
			view   result{m_data.subspan(start, newline_pos - start)};

			if (*it == LINE_FEED)
			{
				m_pos = newline_pos + 1; // \n
			}
			else                         // *it == u8'\r'
			{
				m_pos = newline_pos + 1; // \r (CARRIAGE_RETURN)
				if (m_pos < m_data.size() && m_data[m_pos] == LINE_FEED)
				{
					m_pos += 1;          // trailing \n
				}
			}

			return result;
		}

		// views
		[[nodiscard]] view remaining_view() const noexcept { return view{m_data.subspan(m_pos)}; }

		[[nodiscard]] view full_view() const noexcept { return view{m_data}; }

		// expects
		[[nodiscard]] bool expect(char32 c) noexcept { return skip_if(c); }

		[[nodiscard]] bool expect(std::string_view sv) noexcept
		{
			if (sv.empty())
				return true;

			if (remaining_bytes() < sv.size())
				return false;

			std::string_view target{reinterpret_cast<const char*>(m_data.data() + m_pos), sv.size()};
			if (target != sv)
				return false;

			m_pos += sv.size();

			assert::check(m_pos >= m_data.size() or utf8::is_start_byte(m_data[m_pos]),
						  "expect(string_view) advanced m_pos to mid-codepoint");
			return true;
		}

		[[nodiscard]] bool expect(view v) noexcept
		{
			if (v.empty())
				return true;

			if (remaining_bytes() < v.size_in_bytes())
				return false;

			auto       target = m_data.subspan(m_pos, v.size_in_bytes());
			const bool match  = std::ranges::equal(target, v.data());

			if (match)
			{
				m_pos += v.size_in_bytes();
				assert::check(m_pos >= m_data.size() or utf8::is_start_byte(m_data[m_pos]),
							  "expect(view) advanced m_pos to mid-codepoint");
			}
			return match;
		}

		[[nodiscard]] constexpr bool starts_with(std::string_view prefix) const noexcept
		{
			if (remaining_bytes() < prefix.size())
				return false;

			std::string_view window{reinterpret_cast<const char*>(m_data.data() + m_pos), remaining_bytes()};
			return window.starts_with(prefix);
		}

		[[nodiscard]] constexpr bool starts_with(view prefix) const noexcept
		{
			if (remaining_bytes() < prefix.size_in_bytes())
				return false;

			std::span<const u8> window = m_data.subspan(m_pos, prefix.size_in_bytes());
			return std::ranges::equal(window, prefix.data());
		}

		[[nodiscard]] constexpr bool starts_with(char32 codepoint) const noexcept
		{
			return has_next() and current() == codepoint;
		}

		[[nodiscard]] constexpr bool ends_with(std::string_view suffix) const noexcept
		{
			if (remaining_bytes() < suffix.size())
				return false;

			std::string_view window{reinterpret_cast<const char*>(m_data.data() + m_pos), remaining_bytes()};
			return window.ends_with(suffix);
		}

		[[nodiscard]] constexpr bool ends_with(view suffix) const noexcept
		{
			if (remaining_bytes() < suffix.size_in_bytes())
				return false;

			std::span<const u8> window =
			  m_data.subspan(m_pos + remaining_bytes() - suffix.size_in_bytes(), suffix.size_in_bytes());
			return std::ranges::equal(window, suffix.data());
		}

		[[nodiscard]] constexpr bool ends_with(char32 codepoint) const noexcept
		{
			if (not has_next())
				return false;

			size_t back_pos = m_data.size() - 1;
			while (back_pos > m_pos and utf8::is_continuation_byte(m_data[back_pos]))
				--back_pos;

			return decode_at(back_pos) == codepoint;
		}
	};
} // namespace deckard::utf8
