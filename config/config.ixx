export module deckard.config;

import deckard.utf8;
import deckard.types;
import deckard.file;
import deckard.debug;
import deckard.helpers;
import deckard.utils.hash;

namespace fs = std::filesystem;

namespace deckard::config
{


	// Newline means LF (0x0A) or CRLF (0x0D 0x0A).
	//
	// if [, consume_section, until ], if \n or EOF fail error

	// Section and keys ascii-only.
	// strings utf8

	// section .dot. key
	// token_index_map = map<section.key, token_index>

	// no duplicate keys

	// config["section.key"] = 123;		-> return index, tokens[index] = newvalue
	// std::vector<ConfigToken>
	//
	//
	// vector<token> tokens
	// keys = map<keys_hash, token_index>;


	// Example 1:
	//
	// [section]
	// key = "value"
	// count = 1

	// tokens:			token saved as index to data? or just as raw bytes?
	//  0: section = "section"		# missing newline tokens
	//  1: key = "section.key"
	//  2: value = "value"
	//  3: key = "section.count"
	//  4: value = "1"
	//  5: EOF

	// get value:
	// get("section", "count")
	//  map["section.count"] -> 4 ; index to token
	//  tokens[4] = "1" ; convert to whatever

	// new value:
	// set("section", "count", 2)
	//  map["section", "count"] -> value at index 4
	// tokens[4] = "2"


	// Example 2:
	// globalcount = 1
	//
	// [section]
	// count = 2

	// tokens:
	// 0: key = "globalcount"
	// 1: value = "1"
	// 2: section: "section"
	// 3: key = "section.count"
	// 4: value = "2"

	// map["globalcount") -> 0
	// tokens[0] -> tokens[0+1] -> value = "1"
	// key token is followed by value
	// [section]
	//  = 3.14		# not allowed, no key
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

	class config3
	{
	private:
		std::vector<u8>              data;
		std::vector<TokenType>       tokens;
		std::unordered_map<u64, u64> key_hash_to_token_index; // use _siphash

		void parse() 
		{
			//
		}

	public:
		explicit config3(fs::path file)
			: data(file::read_text_file(file))
		{
		}

		explicit config3(std::string_view input) { }
	};

#pragma region !Old config stuff
#if 0
	struct ConfigToken
	{
		TokenType   type;
		std::string value;
	};

	struct SectionKey
	{
		std::string section;
		std::string key;
	};

	using Value = std::variant<std::monostate, bool, i64, u64, f64, utf8::string>;

	struct TokenIndexValuePair
	{
		Value v;
		u32   token_index;
	};

	export struct Token
	{
		TokenType type{};

		std::vector<u8> data;

		Token(TokenType t)
			: type(t)
		{
		}

		bool operator==(const Token& other) const
		{
			if (type != other.type)
				return false;
			if (data != other.data)
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

	export class config2
	{
	private:

	private:
		std::string        data;
		std::vector<Token> tokens;

		void parse(std::span<u8> buffer)
		{
			using namespace utf8::basic_characters;


			for (size_t index = 0; index < buffer.size_bytes(); index++)
			{
				u8 c = buffer[index];

				if (c == LINE_FEED) // posix
				{
					tokens.push_back(TokenType::NEWLINE_POSIX);
					continue;
				}
			}
		}


	public:
		config2(std::span<u8> input) { parse(input); }

		config2(std::string_view input)
			: config2(to_span(input))
		{
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
						//result.append(token.value);
						result.append("]");
						break;
					case TokenType::KEY:
						//result.append(token.value);
						result.append(" = ");

						break;
					case TokenType::VALUE:
					{

						//result.append(token.value);
						break;
					}
					case TokenType::COMMENT:
						result.append(" # ");
						//result.append(token.value);
						break;
					default: dbg::panic("no handler for this token type: {}", static_cast<u8>(token.type));
				}
			}
			return result;
		}
	};

#if 0
	export class config
	{
	private:
		utf8::string       data;
		std::vector<Token> tokens;

		// std::unordered_map<


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
						tokens.push_back({.type = TokenType::NEWLINE_POSIX, .value = ""});
						start += 2;
					}
					else
					{
						tokens.push_back({.type = TokenType::NEWLINE_WINDOWS, .value = ""});
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
						tokens.push_back({.type = TokenType::SECTION, .value = utf8::string(section.value())});

					start += section.has_value() ? section.value().size() + 2ull : 0;
					continue;
				}
				else if (*start == '#')
				{
					auto comment = consume_comment(start);
					if (comment.has_value())
						tokens.push_back({.type = TokenType::COMMENT, .value = utf8::string(comment.value())});

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
					auto key = utf8::string{key_start, start};


					tokens.push_back({TokenType::KEY, key.trim()});
					dbg::println("key '{}'", key);

					if (start != end && *start == '=')
					{
						++start; // consume '='
						auto value_start = start;
						while (start != end && *start != '\n' && *start != '\r' && *start != '#')
							++start;
						auto value = utf8::string{value_start, start};
						_          = 0;
						tokens.push_back({TokenType::VALUE, value.trim()});
						dbg::println("value: '{}'", value);
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
						result.append(" = ");
						dbg::println("key token value '{} /#'", token.value);

						break;
					case TokenType::VALUE:
					{

						result.append(token.value);
						dbg::println("value token value'{}' /#", token.value);
						break;
					}
					case TokenType::COMMENT:
						result.append(" # ");
						result.append(token.value);
						break;
					default: dbg::panic("no handler for this token type: {}", static_cast<u8>(token.type));
				}
			}
			return result;
		}
	};
#endif
} // namespace deckard::config

export namespace std
{

	template<>
	struct formatter<deckard::config::Token>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const deckard::config::Token& v, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "{}", "");
		}
	};

	template<>
	struct formatter<deckard::config::config2>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const deckard::config::config2& v, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "{}", v.to_string());
		}
	};

	template<>
	struct formatter<deckard::config::SectionKey>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const deckard::config::SectionKey& v, std::format_context& ctx) const
		{
			if (v.section.empty())
				return std::format_to(ctx.out(), "{}", v.key);
			else
				return std::format_to(ctx.out(), "{}.{}", v.section, v.key);
		}

		int  parsed_base = 10;
		bool uppercase   = false;
	};
#endif
#pragma endregion
} // namespace deckard::config
