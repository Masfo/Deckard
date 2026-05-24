export module deckard.utf8;
export import :ascii;
export import :codepoints;
export import :xid;
export import :xid_tables;
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
		
	{ 
		*this = u8str.subview(0, length);
	}

	[[nodiscard]] string v1::view::sub_str(size_t codepoint_offset, size_t codepoint_count) const
	{
		const size_t total_bytes = m_data.size_bytes();
		if (total_bytes == 0 or codepoint_count == 0)
			return string{};

		size_t byte_start        = 0;
		size_t current_codepoint = 0;

		while (current_codepoint < codepoint_offset and byte_start < total_bytes)
		{
			auto [_, bytes] = utf8::decode_unchecked(m_data, byte_start);
			byte_start += bytes;
			current_codepoint++;
		}

		if (byte_start >= total_bytes)
			return string{};

		size_t byte_end        = byte_start;
		size_t count_extracted = 0;
		while (count_extracted < codepoint_count and byte_end < total_bytes)
		{
			auto [_, bytes] = utf8::decode_unchecked(m_data, byte_end);
			byte_end += bytes;
			count_extracted++;
		}

		std::span<const u8> slice{m_data.data() + byte_start, byte_end - byte_start};

		return string{std::string_view{reinterpret_cast<const char*>(slice.data()), slice.size()}};
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
		mode             m{mode::character};
		std::string_view null_str{"<nullopt>"};

		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto it = ctx.begin();
			if (it == ctx.end() || *it == '}')
				return it;

			// Optional null override: {|none} {|?} {|-}
			if (*it == '|')
			{
				++it;
				auto start = it;
				while (it != ctx.end() && *it != '}')
					++it;
				null_str = std::string_view{start, it};
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

			// Allow {x|?} — mode then null override
			if (it != ctx.end() && *it == '|')
			{
				++it;
				auto start = it;
				while (it != ctx.end() && *it != '}')
					++it;
				null_str = std::string_view{start, it};
			}

			return it;
		}

		auto format(const std::optional<char32_t>& opt, std::format_context& ctx) const
		{
			using namespace deckard::utf8;

			if (!opt.has_value())
				return std::format_to(ctx.out(), "{}", null_str);

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
