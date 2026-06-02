export module deckard.utf8;
export import :ascii;
export import :codepoints;
export import :xid;
export import :tables;
export import :string;
export import :view;
export import :decode;
export import :utf8_span;
export import :basic_characters;
export import :names;
export import :scanner;

export using namespace deckard::utf8::literals;

namespace deckard::utf8
{
	// to avoid circular modules


	constexpr v1::view::view(const string& u8str) noexcept
		: m_data{u8str.data()}
	{
	}

	constexpr v1::view::view(const string& u8str, size_t length) noexcept
		: view(u8str, 0, length)
	{
	}

	constexpr v1::view::view(const string& u8str, size_t offset, size_t length) noexcept
	{
		m_data = u8str.subspan(offset, length);
	}


	[[nodiscard]] string v1::view::sub_str(size_t codepoint_offset, size_t codepoint_count) const
	{
		return string{subview(codepoint_offset, codepoint_count)};
	}


	size_t v1::view::find_last_of(const string& view, size_t pos = npos) const
	{
		if (empty() or view.empty())
			return npos;

		if (pos >= size())
			pos = size() - 1;

		auto it = begin() + pos;

		while (true)
		{
			for (auto view_it = view.begin(); view_it != view.end(); ++view_it)
			{
				if (*it == *view_it)
					return std::distance(begin(), it);
			}

			if (it == begin())
				break;

			--it;
		}
		return npos;
	}

	export struct as_utf8
	{
		char32 cp;

		explicit as_utf8(char32 c)
			: cp{c}
		{
		}
	};


} // namespace deckard::utf8

namespace std
{
	template<>
	struct formatter<deckard::utf8::as_utf8>
	{
		enum class mode
		{
			character,
			hex,
			decimal
		};
		mode m{mode::character};

		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto it = ctx.begin();
			if (it == ctx.end() || *it == '}')
				return it;
			switch (*it)
			{
				case 'x':
					m = mode::hex;
					++it;
					break;
				case 'd':
					m = mode::decimal;
					++it;
					break;
				case 'c':
					m = mode::character;
					++it;
					break;
				default: break;
			}
			return it;
		}

		auto format(deckard::utf8::as_utf8 v, std::format_context& ctx) const
		{
			using namespace deckard::utf8;

			if (m == mode::hex)
				return std::format_to(ctx.out(), "U+{:04X}", static_cast<u32>(v.cp));

			if (m == mode::decimal)
				return std::format_to(ctx.out(), "{}", static_cast<u32>(v.cp));

			auto encoded = encode_codepoint(v.cp);
			return std::format_to(
			  ctx.out(), "{}", std::string_view{reinterpret_cast<const char*>(encoded.bytes.data()), encoded.count});
		}
	};

template<>
	struct formatter<std::optional<char32_t>>
	{
		enum class mode
		{
			character,
			hex,
			decimal
		};

		static constexpr std::string_view default_null_str{"<nullopt>"};
		static constexpr size_t           null_buf_size = 16;

		mode                 m{mode::character};
		std::array<char, 16> null_buf{};
		size_t               null_len{};

		constexpr formatter()
		{
			std::ranges::copy(default_null_str, null_buf.begin());
			null_len = default_null_str.size();
		}

		constexpr std::string_view null_str() const noexcept { return {null_buf.data(), null_len}; }

		constexpr void set_null_str(auto& it, auto end)
		{
			null_len = 0;
			while (it != end && *it != '}' && null_len < null_buf.size() - 1)
				null_buf[null_len++] = *it++;
			null_buf[null_len] = '\0';

			while (it != end && *it != '}')
				++it;
		}

		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto it = ctx.begin();
			if (it == ctx.end() || *it == '}')
				return it;

			// {|text} — null override only
			if (*it == '|')
			{
				++it;
				set_null_str(it, ctx.end());
				return it;
			}

			switch (*it)
			{
				case 'x':
					m = mode::hex;
					++it;
					break;
				case 'd':
					m = mode::decimal;
					++it;
					break;
				case 'c':
					m = mode::character;
					++it;
					break;
				default: break;
			}

			// {x|text} — mode then null override
			if (it != ctx.end() && *it == '|')
			{
				++it;
				set_null_str(it, ctx.end());
			}

			return it;
		}

		auto format(const std::optional<char32_t>& opt, std::format_context& ctx) const
		{
			using namespace deckard::utf8;

			if (!opt.has_value())
				return std::format_to(ctx.out(), "{}", null_str());

			const char32_t cp = *opt;

			if (m == mode::hex)
				return std::format_to(ctx.out(), "U+{:04X}", static_cast<u32>(cp));

			if (m == mode::decimal)
				return std::format_to(ctx.out(), "{}", static_cast<u32>(cp));

			auto encoded = encode_codepoint(cp);
			return std::format_to(
			  ctx.out(), "{}", std::string_view{reinterpret_cast<const char*>(encoded.bytes.data()), encoded.count});
		}
	};
} // namespace std
