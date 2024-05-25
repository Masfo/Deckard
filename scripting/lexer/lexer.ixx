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
		INTEGER,        //
		FLOATING_POINT, //
		KEYWORD,        //
		IDENTIFIER,     //
		CHARACTER,      // 'a'
		STRING,         // "abc"

		// Op
		PLUS,        // +
		MINUS,       // -
		STAR,        // *
		SLASH,       // /
		PERCENT,     // %
		QUESTION,    // ?
		BANG,        // !


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

		//
		DOT,        // .
		ELLIPSIS,   // ...
		COMMA,      // ,
		COLON,      // :
		SEMI_COLON, // ;
		HASH,       // #
		ARROW,      // ->

		PIPE,       // |
		AND,        // &
		XOR,        // ^
		TILDE,      // ~

		// Brackets
		LEFT_PAREN,    // (
		RIGHT_PAREN,   // )
		LEFT_BRACE,    // {
		RIGHT_BRACE,   // }
		LEFT_BRACKET,  // [
		RIGHT_BRACKET, // ]

		//
		NOP, //
		EOL,
		EOF,
	};

	using lexeme = std::vector<char32_t>;

	export struct token
	{
		lexeme       lexeme;
		std::wstring str_literal;
		u32          line{0};
		u32          cursor{0}; // cursor pos in line
		Token        type;
	};

	struct registered_symbol
	{
		char32_t literal;
		Token    type;
	};

	// TODO: constexpr map?
	constexpr std::array<registered_symbol, 27> rsymbols = {{
	  {'=', Token::EQUAL},
	  {'+', Token::PLUS},
	  {'-', Token::MINUS},
	  {'*', Token::STAR},
	  {'/', Token::SLASH},
	  {'\\', Token::BACK_SLASH},
	  {'%', Token::PERCENT},
	  {'<', Token::LESSER},
	  {'>', Token::GREATER},
	  {'|', Token::PIPE},
	  {'?', Token::QUESTION},
	  {'|', Token::PIPE},
	  {'&', Token::AND},
	  {'^', Token::XOR},
	  {'~', Token::TILDE},

	  //
	  {'(', Token::LEFT_PAREN},
	  {')', Token::RIGHT_PAREN},
	  {'[', Token::LEFT_BRACKET},
	  {']', Token::RIGHT_BRACKET},
	  {'{', Token::LEFT_BRACE},
	  {'}', Token::RIGHT_BRACE},
	  //
	  {'.', Token::DOT},
	  {'...', Token::ELLIPSIS},
	  {',', Token::COMMA},
	  {':', Token::COLON},
	  {';', Token::SEMI_COLON},
	  {'#', Token::HASH},
	  //
	}};

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
			init_default_keyword();

			while (not eof())
			{
				const auto p  = peek(0);
				const auto p2 = peek(1);
				if (p == '\n' or p == '\r') // linux/mac
				{
					next(1);
					cursor = 0;
					line += 1;
					continue;
				}
				if (p == '\r' and p2 == '\n') // windows
				{
					next(2);
					cursor = 0;
					line += 1;
					continue;
				}

				if (utf8::is_whitespace(peek()))
				{
					read_whitespace();
					continue;
				}

				if (utf8::is_ascii_digit(peek()))
				{
					read_number();
					continue;
				}

				if (peek() == '\"')
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
			}

			insert_token(Token::EOF, {0});

			return m_tokens;
		}

		void read_whitespace() noexcept
		{
			//
			while (utf8::is_whitespace(peek()))
			{
				next();
			}
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

		void read_string() noexcept
		{
			//
			lexeme lit;
			next();
			while (not eof() and peek() != '\"')
			{
				lit.push_back(next());
			}
			next();

			insert_token(Token::STRING, lit);
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

			Token type = Token::IDENTIFIER;
			if (is_keyword(lit))
				type = Token::KEYWORD;

			insert_token(type, lit);
		}

		void read_symbol() noexcept
		{
			//
			lexeme lit;
			Token  type   = Token::EOF;
			auto   symbol = next();

			for (const auto& rs : rsymbols)
			{
				if (symbol == rs.literal)
				{
					type = rs.type;
					lit.push_back(symbol);
					break;
				}
			}

			assert::check(type != Token::EOF, "Unknown symbol");


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

		bool is_keyword(const lexeme& literal)
		{
			std::string str;
			for (const auto& c : literal)
				str += (char)c;

			for (const auto& word : keywords)
			{
				if (word == str)
					return true;
			}
			return false;
		}


	private:
		void init_default_keyword()
		{
			keywords.push_back("fn");
			keywords.push_back("if");
			keywords.push_back("let");
			keywords.push_back("return");
		}

		utf8file              input;
		std::vector<char32_t> codepoints;


		u32         index{0};
		tokens      m_tokens;
		std::string filename;

		u32 line{0};
		u32 cursor{0};


		std::vector<std::string> keywords;
	};
} // namespace deckard::lexer
