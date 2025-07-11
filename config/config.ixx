export module deckard.config;

import deckard.utf8;
import deckard.types;
import deckard.file;
import deckard.debug;

namespace fs = std::filesystem;

namespace deckard::config
{
	export enum struct TokenType : u8 {
		NEWLINE = 0x00,
		NEWLINE_POSIX,
		NEWLINE_WINDOWS,

		SECTION,
		KEY,
		VALUE,
		COMMENT,

		END_OF_FILE = 0xFF,
	};

	// if [, consume_section, until ], if \n or EOF fail error

	// Section and keys ascii-only.
	// strings utf8

	export struct Token
	{
		TokenType  type{};
		utf8::view value;

		bool operator==(const Token& other) const
		{
			if (type != other.type)
				return false;
			if (value != other.value)
				return false;
			return true;
		}
	};

	template<typename T>
	concept consume_predicate = requires(T&& v, char32 cp) {
		{ v(cp) } -> std::same_as<bool>;
	};

	struct ConsumeResult
	{
		utf8::view view;
		u64        consumed_chars;

		ConsumeResult(utf8::view v, u64 chars)
			: view(v)
			, consumed_chars(chars)
		{
		}
	};

	export class config
	{
	private:
		utf8::string       data;
		std::vector<Token> tokens;

		ConsumeResult consume_ascii_until(consume_predicate auto&& predicate, utf8::iterator start)
		{
			auto end = data.end();
			auto it  = start;
			while (it != end && !predicate(*it))
			{
				if (utf8::is_whitespace(*it))
					return {data.subview(start, it - start), it - start};
				++it;
			}
			if (it == end)
				return {data.subview(start, it - start), it - start};
			return {data.subview(start, it - start), it - start + 1};
		}

		std::expected<utf8::view, std::string> consume_section(utf8::iterator start)
		{
			auto section_start = start + 1; // skip '['
			auto end           = data.end();

			while (start != end && *start != ']')
			{
				if (utf8::is_whitespace(*start))
					return std::unexpected("Invalid section header");
				++start;
			}
			if (start == end)
				return std::unexpected("Invalid section header");
			auto section = data.subview(section_start, start - section_start);
			return section;
		}

		std::expected<utf8::view, std::string> consume_comment(utf8::iterator start)
		{
			start += 1; // skip #

			auto total_start   = start;
			auto comment_start = start;
			auto end           = data.end();

			while (comment_start != end && utf8::is_whitespace(*comment_start))
				++comment_start;

			while (start != end && *start != '\n' && *start != '\r')
				++start;

			if (start == end)
				return std::unexpected("Invalid comment header");

			auto comment = data.subview(comment_start, start - comment_start);

			size_t consumed_chars = comment_start - total_start;

			return comment;
		}

		void tokenize()
		{
			if (data.empty())
				return;

			auto start = data.begin();
			auto end   = data.end();
			while (start != end)
			{
				auto current = *start;

				if (*start == '\n' or *start == '\r')
				{
					if (start + 1 != end && *(start + 1) == '\n')
					{
						tokens.push_back({TokenType::NEWLINE_POSIX, {}});
						start += 2;
					}
					else
					{
						tokens.push_back({TokenType::NEWLINE_WINDOWS, {}});
						start += 1;
					}
				}
				else if (utf8::is_whitespace(*start))
				{
					start++;
					continue;
				}
				else if (*start == '[')
				{
					// TODO: consume_section should return how many characters it consumed
					auto section = consume_section(start);
					if (section.has_value())
						tokens.push_back({TokenType::SECTION, section.value()});

					start += section.has_value() ? section.value().size() + 2ull : 0;
					continue;
				}
				else if (*start == '#')
				{
					auto comment = consume_comment(start);
					if (comment.has_value())
						tokens.push_back({TokenType::COMMENT, comment.value()});

					// TODO: consume_comment should return how many characters it consumed
					// +2 here is wrong,

					size_t len = comment.has_value() ? comment.value().size() + 2ull : 0;

					start += len;
					continue;
				}
				else
				{
					auto key_start = start;
					while (start != end && *start != '=' && *start != '\n' && *start != '\r')
						++start;
					// auto key = utf8::view(key_start, start);
					if (start != end && *start == '=')
					{
						++start; // consume '='
						auto value_start = start;
						while (start != end && *start != '\n' && *start != '\r')
							++start;
						// auto value = utf8::view(value_start, start-value_start);
						// tokens.push_back({TokenType::KEY, key});
						// tokens.push_back({TokenType::VALUE, value});
					}
				}
			}
		}


	public:
		explicit config(utf8::view view)
			: data(view)
		{
			data = view;
			tokenize();
		}

		config(std::string_view input)
		{
			data = input;
			tokenize();
		}

		utf8::string to_string() const
		{
			utf8::string result;
			for (const auto& token : tokens)
			{
				switch (token.type)
				{
					case TokenType::NEWLINE: [[fallthrough]];
					case TokenType::NEWLINE_POSIX: result.append("\n"); break;
					case TokenType::NEWLINE_WINDOWS: result.append("\r\n"); break;
					case TokenType::SECTION:
						result.append("[");
						result.append(token.value);
						result.append("]");
						break;
					case TokenType::KEY:
						result.append(token.value);
						result.append("=");
						break;
					case TokenType::VALUE: result.append(token.value); break;
					case TokenType::COMMENT:
						result.append("#");
						result.append(token.value);
						break;
					default: dbg::panic("no handler for this token type: {}", static_cast<u8>(token.type));
				}
			}
			return result;
		}
	};

} // namespace deckard::config

export namespace std
{
	template<>
	struct formatter<deckard::config::config>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const deckard::config::config& v, std::format_context& ctx) const 
		{
			return std::format_to(ctx.out(), "{}", v.to_string()); 
		}

		int  parsed_base = 10;
		bool uppercase   = false;
	};


} // namespace std
