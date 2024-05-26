export module deckard.lexer;
import std;
import deckard.utf8;
import deckard.as;
import deckard.debug;
import deckard.assert;
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
		INTEGER,        // 1
		FLOATING_POINT, // 3.14
		KEYWORD,        // if, else
		IDENTIFIER,     // a123, _123
		CHARACTER,      // 'a'
		STRING,         // "abc"

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
		EOL,
		EOF,
	};

	using lexeme = std::vector<char32_t>;

	using number = std::variant<double, i64, u64>;

	export struct token
	{
		lexeme       lexeme;
		std::wstring str_literal;
		number       num;
		u32          line{0};
		u32          cursor{0}; // cursor pos in line
		Token        type;
	};

	export class tokenizer
	{
	public:
		using tokens = std::vector<token>;

		tokenizer() = default;

		tokenizer(codepoints cp) noexcept { codepoints = cp.data(true); }

		explicit tokenizer(fs::path f) noexcept
			: input(f)
		{
			utf8::codepoints cps = input.data();
			codepoints           = cps.data(true);
			m_tokens.reserve(codepoints.size());
			filename = input.filename().string();
		}

		explicit tokenizer(std::string_view str) noexcept
		{
			utf8::codepoints cps(str.data());
			codepoints = cps.data(true);
			m_tokens.reserve(codepoints.size());
			filename = "";
		}

		bool has_data(u32 offset = 0) const noexcept { return (index + offset) < codepoints.size(); }

		char32_t peek(u32 offset = 0) noexcept
		{
			if (has_data(offset))
			{
				return codepoints[index + offset];
			}
			return utf8::EOF_CHARACTER;
		}

		bool eof() noexcept { return has_data() && peek() == utf8::EOF_CHARACTER; }

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

		tokens tokenize() noexcept
		{
			init_defaults();

			while (not eof())
			{
				const auto current_char = peek(0);
				const auto next_char    = peek(1);

				if (current_char == '\n' or current_char == '\r') // linux/mac
				{
					next(1);
					cursor = 0;
					line += 1;
					continue;
				}
				if (current_char == '\r' and next_char == '\n') // windows
				{
					next(2);
					cursor = 0;
					line += 1;
					continue;
				}

				if (utf8::is_whitespace(peek()))
				{
					next();
					continue;
				}


				if (utf8::is_ascii_digit(peek()))
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


				if (utf8::is_identifier_start(peek()))
				{
					read_identifier();
					continue;
				}


				if (utf8::is_ascii(peek()))
				{
					read_symbol();
					continue;
				}

				dbg::panic("what is this?");
			}

			insert_token(Token::EOF, {0});

			return m_tokens;
		}

		void read_number() noexcept
		{
			lexeme lit;
			while (not eof())
			{
				if (auto n = peek(); utf8::is_ascii_digit(n))
				{
					lit.push_back(next());
				}
				else
					break;
			}

			insert_token(Token::INTEGER, lit);
		}

		void read_char() noexcept
		{
			lexeme lit;
			Token  type = Token::CHARACTER;

			next();

			lit.push_back(next());

			if (peek() != '\'')
				type = Token::INVALID;
			else
				next();

			insert_token(type, lit);
		}

		void read_string() noexcept
		{
			//
			lexeme lit;
			Token  type = Token::STRING;

			next();

			// TODO: escape chars
			while (not eof() and peek() != '\"')
			{
				lit.push_back(next());
			}
			next();

			insert_token(type, lit);
		}

		void read_identifier() noexcept
		{
			lexeme lit;


			while (not eof())
			{
				if (auto n = peek(); utf8::is_identifier_continue(n))
					lit.push_back(next());
				else
					break;
			}
			// TODO: max len ident

			Token type = Token::IDENTIFIER;

			if (lit.size() == 1 and lit[0] == '_')
				type = Token::UNDERSCORE;

			if (is_keyword(lit))
				type = Token::KEYWORD;

			// TODO: type/usertype detection

			insert_token(type, lit);
		}

		void read_symbol() noexcept
		{
			//
			lexeme lit;
			Token  type        = Token::UNKNOWN;
			auto   current     = peek(0);
			auto   next_char   = peek(1);
			int    symbol_size = 1;


			switch (current)
			{
				case '=':
				{
					type = Token::EQUAL;

					if (next_char == '=')
					{
						type        = Token::EQUAL_EQUAL;
						symbol_size = 2;
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
					}
					if (next_char == '>')
					{
						type        = Token::ARROW;
						symbol_size = 2;
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


			assert::check(type != Token::UNKNOWN, "Unknown symbol");

			lit.push_back(current);
			if (symbol_size == 2)
				lit.push_back(next_char);

			next(symbol_size);

			insert_token(type, lit);
		}

		void insert_token(Token type, const lexeme& literal) noexcept
		{
			std::wstring s;
			for (const auto& c : literal)
				s += (char32_t)c;
			token t{
			  //
			  .lexeme      = literal,
			  .str_literal = s,
			  .line        = line,
			  .cursor      = cursor,
			  .type        = type
			  //
			};

			m_tokens.emplace_back(t);
		}

		bool is_keyword(const lexeme& literal) noexcept
		{
			std::string str;
			for (const auto& c : literal)
				str += (char)c;

			for (const auto& word : keywords)
			{
				if (word.size() <= longest_keyword and word.size() == str.size() && word == str)
					return true;
			}


			return false;
		}

		void add_keyword(const std::string& word) noexcept
		{
			longest_keyword = std::max(longest_keyword, word.size());
			keywords.push_back(word);
			std::ranges::sort(keywords);
		}

		void add_type(const std::string& type) noexcept { builtin_types.push_back(type); }


	private:
		void init_defaults() noexcept
		{
			// Keywords
			add_keyword("let");
			add_keyword("fn");
			add_keyword("return");

			add_keyword("if");
			add_keyword("else");

			add_keyword("true");
			add_keyword("false");

			add_keyword("struct");
			add_keyword("enum");


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

		utf8file              input;
		std::vector<char32_t> codepoints;


		u32         index{0};
		tokens      m_tokens;
		std::string filename;

		u32 line{0};
		u32 cursor{0};

		size_t                   longest_keyword{0};
		std::vector<std::string> keywords;
		std::vector<std::string> builtin_types;
	};
} // namespace deckard::lexer
