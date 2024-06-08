export module deckard.lexer;
import std;
import deckard.utf8;
import deckard.as;
import deckard.debug;
import deckard.assert;
import deckard.helpers;
import deckard.types;

namespace fs = std::filesystem;
using namespace std::string_view_literals;
using namespace deckard::utf8;

namespace deckard::lexer
{
	/*
	* General tokenizer, with keyword support

		tokenizer.add_keyword("if") -> TOKEN_KEYWORD: "if"
									-> TOKEN_IDENTIFIER: "if"

		std::array keywords{"if"sv, "while"sv, "for"sv};

		tokenizer.add_keywords(keyword);
				add_keywords(std::span<std::string_view> words)


		tokenizer.comment("#");


	identifier				Names assigned by the programmer.						x, color, UP
	keyword					Reserved words of the language.							if, while, return
	separator/punctuator	Punctuation characters and paired delimiters.			}, (, ;
	operator				Symbols that operate on arguments and produce results.	+, <, =
	literal					Numeric, logical, textual, and reference literals.		true, 6.02e23, "music"
	comment					comment	Line or block comments. Usually discarded.

	 */

	export enum class Token : u8 {
		INTEGER,        // 1, -1
		FLOATING_POINT, // 3.14
		KEYWORD,        // if, else
		IDENTIFIER,     // a123, _123
		CHARACTER,      // 'a'
		STRING,         // "abc"

		// keywords
		KEYWORD_TRUE,   // true
		KEYWORD_FALSE,  // false
		KEYWORD_IF,     //
		KEYWORD_ELSE,   //
		KEYWORD_FN,     //
		KEYWORD_LET,    //
		KEYWORD_STRUCT, //
		KEYWORD_ENUM,   //
		KEYWORD_RETURN, //


		TYPE,           // builtin type: i32, f32
		USER_TYPE,      // struct <type>

		// Op
		PLUS,        // +
		MINUS,       // -
		STAR,        // *
		SLASH,       // /
		PERCENT,     // %
		QUESTION,    // ?
		BANG,        // !
		AT,          // @
		UNDERSCORE,  // _


		BACK_SLASH,  // '\'
		SLASH_SLASH, // //
		SLASH_STAR,  // /*
		STAR_SLASH,  // */

		EQUAL,       // =

		// Compare
		LESSER,         // <
		GREATER,        // >
		LESSER_EQUAL,   // <=
		GREATER_EQUAL,  // >=
		EQUAL_EQUAL,    // ==
		PLUS_EQUAL,     // +=
		MINUS_EQUAL,    // -=
		STAR_EQUAL,     // *=
		SLASH_EQUAL,    // /=
		PERCENT_EQUAL,  // %=
		BANG_EQUAL,     // !=
		XOR_EQUAL,      // ^=
		QUESTION_EQUAL, // ?=
		AND_EQUAL,      // &=
		OR_EQUAL,       // |=


		//
		DOT,        // .
		ELLIPSIS,   // ..
		COMMA,      // ,
		COLON,      // :
		SEMI_COLON, // ;
		HASH,       // #
		ARROW,      // ->

		OR,         // |
		AND,        // &
		XOR,        // ^
		TILDE,      // ~
		OR_OR,      // ||
		AND_AND,    // &&

		// Brackets
		LEFT_PAREN,    // (
		RIGHT_PAREN,   // )
		LEFT_BRACE,    // {
		RIGHT_BRACE,   // }
		LEFT_BRACKET,  // [
		RIGHT_BRACKET, // ]

		//
		UNKNOWN,
		INVALID,
		INVALID_CHAR,
		INVALID_HEX,
		INVALID_FLOATING_POINT,
		INVALID_BINARY,
		EOL,
		EOF,
	};

	struct keyword
	{
		std::wstring_view word;
		Token             token;

		bool operator<(const keyword& lhs) const noexcept { return word < lhs.word; }
	};

	using lexeme = std::vector<char32_t>;


	using number = std::variant<std::monostate, double, i64, u64>;

	export struct token
	{
		lexeme       lexeme;
		std::wstring str_literal;
		number       num;
		u32          line{0};
		u32          cursor{0}; // cursor pos in line
		Token        type;

		// in codepoints
		u32 lexeme_index{0};
		u32 lexeme_length{0};

		template<typename T>
		std::optional<T> get() const
		{
			if (std::holds_alternative<T>(num))
				return std::get<T>(num);
			return {};
		}

		auto as_i64() const { return get<i64>(); }

		auto as_u64() const { return get<u64>(); }

		auto as_double() const { return get<double>(); }
	};

	export struct tokenizer_config
	{
		std::string_view digit_separator{"\'"};
		std::string_view line_comment{"//"};
		bool             dot_identifier{false};
		bool             output_eol{false};
	};

	export class tokenizer
	{
	public:
		using tokens = std::vector<token>;

		tokenizer() = default;

		tokenizer(codepoints cp) noexcept { codepoints = cp.data(true); }

		explicit tokenizer(fs::path f) noexcept
			: input_file(f)
		{
			utf8::codepoints cps = input_file.data();
			codepoints           = cps.data(true);
			m_tokens.reserve(codepoints.size());
			filename = input_file.filename().string();
		}

		explicit tokenizer(std::string_view str) noexcept { *this = str; }

		void operator=(std::string_view input)
		{
			reset();
			utf8::codepoints cps(input.data());
			codepoints = cps.data(true);
			m_tokens.reserve(codepoints.size());
			filename = "";
		};

		void setconfig(tokenizer_config c) noexcept { config = c; }

		bool has_data(u32 offset = 0) const noexcept { return (index + offset) < codepoints.size(); }

		char32_t peek(u32 offset = 0) noexcept
		{
			if (has_data(offset))
			{
				return codepoints[index + offset];
			}
			return utf8::EOF_CHARACTER;
		}

		void reset() noexcept
		{
			m_tokens.clear();
			index  = 0;
			line   = 0;
			cursor = 0;
		}

		bool eof() noexcept { return peek() == utf8::EOF_CHARACTER; }

		char32_t next(u32 offset = 1) noexcept
		{
			if (has_data(offset))
			{
				cursor += offset;
				index += offset;
				return codepoints[index - offset];
			}
			return utf8::EOF_CHARACTER;
		}

		token create_token() const
		{
			token t;
			t.line         = line;
			t.cursor       = cursor;
			t.lexeme_index = index;

			return t;
		}

		tokens tokenize() noexcept { return tokenize(config); }

		tokens tokenize(const tokenizer_config& cfg) noexcept
		{
			setconfig(cfg);

			init_defaults();

			while (not eof())
			{
				const auto current_char = peek(0);
				const auto next_char    = peek(1);


				if (utf8::is_whitespace(current_char))
				{
					next();
					continue;
				}


				if ((current_char == '\\' and next_char == '\n'))
				{
					// TODO: any whitespace between slash and newline

					// Any sequence of backslash (\) immediately followed by a new line is deleted,
					// resulting in splicing lines together.
					// \\n
					next(2);
					continue;
				}

				if ((current_char == '\\' and next_char == '\r' and peek(2) == '\n'))
				{
					// \ \r\n
					next(3);
					continue;
				}

				if (current_char == '\n' or current_char == '\r') // linux/mac
				{
					if (config.output_eol)
						insert_token(Token::EOL, {}, cursor);

					next(1);
					cursor = 0;
					line += 1;
					continue;
				}
				if (current_char == '\r' and next_char == '\n') // windows
				{
					if (config.output_eol)
						insert_token(Token::EOL, {}, cursor);

					next(2);
					cursor = 0;
					line += 1;
					continue;
				}

				// TODO: line comment, user selected char // ; #


				// TODO: block comment, nested


				if (utf8::is_ascii_digit(current_char) or (current_char == '.' and utf8::is_ascii_digit(next_char)))
				{
					read_number();
					continue;
				}


				if (current_char == '\'')
				{
					read_char();
					continue;
				}

				if (current_char == '\"')
				{
					read_string();
					continue;
				}


				if (utf8::is_identifier_start(current_char))
				{
					read_identifier();
					continue;
				}


				if (utf8::is_ascii(current_char))
				{
					read_symbol();
					continue;
				}

				dbg::println("Unknown: {}", (u32)current_char);
				dbg::trace();
				dbg::panic("what is this?");
			}

			insert_token(Token::EOF, {}, cursor);

			return m_tokens;
		}

		void read_number() noexcept
		{
			// TODO: digit separator
			// TODO: integer suffix	none i32/i64, u/U u32/u64, l/L i64,	ul/UL u64
			bool hex    = false;
			bool binary = false;

			lexeme lit;
			Token  type = Token::INTEGER;

			auto current_char   = peek(0);
			auto next_char      = peek(1);
			u32  current_cursor = cursor;


			auto diff = cursor - current_cursor;

			if (utf8::is_ascii_digit(current_char) or current_char == '.')
			{
				switch (next_char)
				{
					case 'b': [[fallthrough]];
					case 'B':

					{
						binary = true;
						lit.push_back(next());
						lit.push_back(next());
						diff = cursor - current_cursor;
						while (not eof() and utf8::is_ascii_binary_digit(peek()))
						{

							lit.push_back(next());
						}
						break;
					}
					case 'x': [[fallthrough]];
					case 'X':
					{
						hex = true;
						lit.push_back(next());
						lit.push_back(next());
						diff = cursor - current_cursor;

						while (not eof() and utf8::is_ascii_hex_digit(peek()))
						{
							lit.push_back(next());
						}
						break;
					}

					default:
					{
						u32 dotcount = 0;
						while (not eof() and (utf8::is_ascii_digit(peek()) or peek() == '.'))
						{
							// ellipsis
							if (peek(0) == '.' and peek(1) == '.')
								break;

							if (peek() == '.' and dotcount == 0)
							{
								type = Token::FLOATING_POINT;
								dotcount += 1;
								lit.push_back(next());
								continue;
							}

							if (peek() == '.' and dotcount >= 1)
							{
								type = Token::INVALID_FLOATING_POINT;
								lit.push_back(next());
								continue;
							}

							lit.push_back(next());
						}
						break;
					}
				}
			}


			if (diff == cursor - current_cursor)
			{
				if (hex)
					type = Token::INVALID_HEX;
				if (binary)
					type = Token::INVALID_BINARY;
			}

			insert_token(type, lit, current_cursor);
		}

		void read_char() noexcept
		{
			// TODO: multicharacters, 'ABCD' => 0x41424344;
			lexeme lit;
			Token  type           = Token::CHARACTER;
			u32    current_cursor = cursor;
			next(); // '

			auto diff = cursor - current_cursor;


			while (not eof() and peek() != '\'')
				lit.push_back(next());

			if (peek() != '\'')
				type = Token::INVALID_CHAR;
			else
				next(); // '

			if (diff == cursor - current_cursor)
			{
				type = Token::INVALID_CHAR;
			}

			insert_token(type, lit, current_cursor);
		}

		void read_string() noexcept
		{
			//
			lexeme lit;
			Token  type           = Token::STRING;
			u32    current_cursor = cursor;

			// TODO: multiline string

			// TODO: python triple """ string, read all until second """

			next();
			bool end_quote = false;
			while (not eof())
			{

				if (end_quote)
					break;

				auto current = peek();
				switch (current)
				{
					case '\"':
					{
						end_quote = true;
						next();
						break;
					}
					case '\\':
					{
						next();
						current = peek();
						switch (current)
						{
							case '\'':
								lit.push_back('\'');
								break;


								// TODO: hex
								// case 'x': lit.push_back('\n'); break;

							case 'n': lit.push_back('\n'); break;
							case 'r': lit.push_back('\r'); break;
							case 't': lit.push_back('\t'); break;
							case '\\': lit.push_back('\\'); break;
							case '\"': lit.push_back('\"'); break;

							default:
							{
								dbg::println("Invalid escape sequence");
							}
						}
						next();

						break;
					}
					default: lit.push_back(next()); break;
				}
			}

			insert_token(type, lit, current_cursor);
		}

		void read_identifier() noexcept
		{
			lexeme lit;
			u32    current_cursor = cursor;


			while (not eof())
			{
				if (auto n = peek();
					config.dot_identifier == true ? n == '.' or utf8::is_identifier_continue(n) : utf8::is_identifier_continue(n))
					lit.push_back(next());
				else
					break;
			}
			// TODO: max len ident

			Token type = Token::IDENTIFIER;

			if (lit.size() == 1 and lit[0] == '_')
				type = Token::UNDERSCORE;

			if (is_keyword(lit))
			{
				type = to_token(lit);
			}

			// TODO: type/usertype detection

			insert_token(type, lit, current_cursor);
		}

		void read_symbol() noexcept
		{
			//
			lexeme lit;
			Token  type           = Token::UNKNOWN;
			auto   current        = peek(0);
			auto   next_char      = peek(1);
			int    symbol_size    = 1;
			u32    current_cursor = cursor;

			switch (current)
			{
				case '=':
				{
					type = Token::EQUAL;

					if (next_char == '=')
					{
						type        = Token::EQUAL_EQUAL;
						symbol_size = 2;
						break;
					}

					break;
				}

				case '+':
				{
					type = Token::PLUS;

					if (next_char == '=')
					{
						type        = Token::PLUS_EQUAL;
						symbol_size = 2;
						break;
					}

					break;
				}

				case '-':
				{
					type = Token::MINUS;

					if (next_char == '=')
					{
						type        = Token::MINUS_EQUAL;
						symbol_size = 2;
						break;
					}
					if (next_char == '>')
					{
						type        = Token::ARROW;
						symbol_size = 2;
						break;
					}

					break;
				}

				case '*':
				{
					type = Token::STAR;

					if (next_char == '=')
					{
						type        = Token::STAR_EQUAL;
						symbol_size = 2;
					}
					if (next_char == '/')
					{
						type        = Token::STAR_SLASH;
						symbol_size = 2;
					}
					break;
				}
				case '/':
				{
					type = Token::SLASH;

					if (next_char == '/')
					{
						type        = Token::SLASH_SLASH;
						symbol_size = 2;
					}
					if (next_char == '=')
					{
						type        = Token::SLASH_EQUAL;
						symbol_size = 2;
					}
					if (next_char == '*')
					{
						type        = Token::SLASH_STAR;
						symbol_size = 2;
					}

					break;
				}

				case '%':
				{
					type = Token::PERCENT;

					if (next_char == '=')
					{
						type        = Token::PERCENT_EQUAL;
						symbol_size = 2;
					}

					break;
				}

				case '_':
				{
					type = Token::UNDERSCORE;
					break;
				}

				case '?':
				{
					type = Token::QUESTION;
					break;
				}


				case '&':
				{
					type = Token::AND;
					if (next_char == '=')
					{
						type        = Token::AND_EQUAL;
						symbol_size = 2;
					}
					if (next_char == '&')
					{
						type        = Token::AND_AND;
						symbol_size = 2;
					}
					break;
				}


				case '|':
				{
					type = Token::OR;
					if (next_char == '=')
					{
						type        = Token::OR_EQUAL;
						symbol_size = 2;
					}
					if (next_char == '|')
					{
						type        = Token::OR_OR;
						symbol_size = 2;
					}

					break;
				}

				case '^':
				{
					type = Token::XOR;
					if (next_char == '=')
					{
						type        = Token::XOR_EQUAL;
						symbol_size = 2;
					}

					break;
				}

				case '\\':
				{
					type = Token::BACK_SLASH;

					break;
				}

				case '~':
				{
					type = Token::TILDE;
					break;
				}

				case '<':
				{
					type = Token::LESSER;
					if (next_char == '=')
					{
						type        = Token::LESSER_EQUAL;
						symbol_size = 2;
					}

					break;
				}
				case '>':
				{
					type = Token::GREATER;
					if (next_char == '=')
					{
						type        = Token::GREATER_EQUAL;
						symbol_size = 2;
					}

					break;
				}

				case '.':
				{
					type = Token::DOT;
					if (next_char == '.')
					{
						type        = Token::ELLIPSIS;
						symbol_size = 2;
					}

					break;
				}

				case ',':
				{
					type = Token::COMMA;

					break;
				}

				case ':':
				{
					type = Token::COLON;

					break;
				}

				case ';':
				{
					type = Token::SEMI_COLON;

					break;
				}

				case '!':
				{
					type = Token::BANG;
					if (next_char == '=')
					{
						type        = Token::BANG_EQUAL;
						symbol_size = 2;
					}

					break;
				}

				case '#':
				{
					type = Token::HASH;

					break;
				}

				case '@':
				{
					type = Token::AT;

					break;
				}

				case '(':
				{
					type = Token::LEFT_PAREN;

					break;
				}
				case ')':
				{
					type = Token::RIGHT_PAREN;

					break;
				}

				case '[':
				{
					type = Token::LEFT_BRACKET;

					break;
				}
				case ']':
				{
					type = Token::RIGHT_BRACKET;

					break;
				}

				case '{':
				{
					type = Token::LEFT_BRACE;

					break;
				}
				case '}':
				{
					type = Token::RIGHT_BRACE;

					break;
				}


				default:
				{
					dbg::println("Unknown symbol: '{:c}'", as<u32>(current));
					type = Token::UNKNOWN;
				}
			}

			// assert::check(type != Token::UNKNOWN, "Unknown symbol");

			lit.push_back(current);
			if (symbol_size == 2)
				lit.push_back(next_char);


			next(symbol_size);

			insert_token(type, lit, current_cursor);
		}

		void insert_token(Token type, const lexeme& literal, u32 current_cursor, number num = 0) noexcept
		{
			std::wstring s;
			for (const auto& c : literal)
				s += (char32_t)c;

			token t{
			  //
			  .lexeme      = literal,
			  .str_literal = s,
			  .num         = num,
			  .line        = line,
			  .cursor      = current_cursor,
			  .type        = type
			  //
			};

			m_tokens.emplace_back(t);
		}

	private:
		bool is_keyword(const lexeme& literal) noexcept
		{
			for (const auto& word : keywords)
			{
				if (word.token == to_token(literal))
					return true;
			}

			return false;
		}

		void add_keyword(std::wstring_view word, Token tok) noexcept
		{
			longest_keyword = std::max(longest_keyword, word.size());
			keywords.push_back({word, tok});
			std::ranges::sort(keywords, std::less{});
		}

		void add_type(const std::string& type) noexcept { builtin_types.push_back(type); }

		void init_defaults() noexcept
		{
			add_keyword(L"let", Token::KEYWORD_LET);
			add_keyword(L"fn", Token::KEYWORD_FN);
			add_keyword(L"return", Token::KEYWORD_RETURN);
			add_keyword(L"if", Token::KEYWORD_IF);
			add_keyword(L"else", Token::KEYWORD_ELSE);
			add_keyword(L"true", Token::KEYWORD_TRUE);
			add_keyword(L"false", Token::KEYWORD_FALSE);
			add_keyword(L"struct", Token::KEYWORD_STRUCT);
			add_keyword(L"enum", Token::KEYWORD_ENUM);


			// Builtin types
			add_type("i8");
			add_type("u8");
			add_type("i16");
			add_type("u16");
			add_type("i32");
			add_type("u32");
			add_type("i64");
			add_type("u64");
			add_type("f32");
			add_type("f64");
		}

		Token to_token(std::wstring_view keyword)
		{
			for (const auto& word : keywords)
			{
				if (word.word == keyword)
					return word.token;
			}
			return Token::UNKNOWN;
		}

		Token to_token(lexeme lex)
		{
			std::wstring wstr;
			for (const auto& c : lex)
				wstr += c;

			return to_token(wstr);
		}

		utf8file              input_file;
		std::vector<char32_t> codepoints;


		u32              index{0};
		tokens           m_tokens;
		std::string      filename;
		tokenizer_config config;

		u32 line{0};
		u32 cursor{0};

		size_t                   longest_keyword{0};
		std::vector<keyword>     keywords;
		std::vector<std::string> builtin_types;
	};
} // namespace deckard::lexer
