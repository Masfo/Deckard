export module deckard.utf8:scanner;

import :codepoints;
import :decode;
import :utf8_span;
import :view;

import std;

namespace deckard::utf8
{
	export class scanner
	{
	private:
		std::span<const u8> m_data;
		size_t              m_pos{0};

	public:
		scanner() = default;

		explicit scanner(v2::view v) noexcept
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
		[[nodiscard]] bool has_next() const noexcept { return m_pos < m_data.size(); }

		[[nodiscard]] explicit operator bool() const noexcept { return has_next(); }

		[[nodiscard]] size_t byte_position() const noexcept { return m_pos; }

		[[nodiscard]] size_t remaining_bytes() const noexcept { return m_data.size() - m_pos; }

		[[nodiscard]] size_t remaining() const noexcept // codepoint count to end
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

		[[nodiscard]] std::optional<char32> peek(size_t n = 1) const noexcept
		{
			size_t idx = m_pos;
			for (size_t i = 0; i < n; ++i)
			{
				if (idx >= m_data.size())
					return std::nullopt;
				idx += width_at(idx);
			}
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

		void skip()
		{
			assert::check(has_next(), "scanner::skip() called at end of input");
			m_pos += width_at(m_pos);
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

		[[nodiscard]] view take_line() noexcept
		{
			const size_t start = m_pos;
			while (has_next())
			{
				const auto [codepoint, width] = utf8::decode_unchecked(m_data, m_pos);

				assert::check(width > 0, "Should never be zero");

				m_pos += width;
				if (codepoint == U'\n')
					return view{m_data.subspan(start, m_pos - width - start)};
				if (codepoint == U'\r')
				{

					const size_t line_end = m_pos - width;
					if (has_next() and decode_at(m_pos) == U'\n')
						m_pos += width_at(m_pos);

					return view{m_data.subspan(start, line_end - start)};
				}
			}
			return view{m_data.subspan(start, m_pos - start)};
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

		[[nodiscard]] bool expect(v2::view v) noexcept
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
							  "expect(v2::view) advanced m_pos to mid-codepoint");
			}
			return match;
		}
	};
} // namespace deckard::utf8
