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

export namespace deckard::lexer
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

	enum class Token : u8
	{
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

		BACK_SLASH,  // '\'
		BANG,        // !
		SLASH_SLASH, // //
		SLASH_STAR,  // /*
		STAR_SLASH,  // */

		EQUAL,       // =

		// Compare
		EQUAL_EQUAL,   // ==
		LESSER,        // <
		GREATER,       // >
		LESSER_EQUAL,  // <=
		GREATER_EQUAL, // >=
		PLUS_EQUAL,    // +=
		MINUS_EQUAL,   // -=
		STAR_EQUAL,    // *=
		SLASH_EQUAL,   // /=
		PERCENT_EQUAL, // %=
		BANG_EQUAL,    // !=


		// Delimeters
		DOT,        // .
		COMMA,      // ,
		COLON,      // :
		SEMI_COLON, // ;
		HASH,       // #
		PIPE,       // |

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

	using literals = std::vector<char32_t>;

	struct token
	{
		literals    literal_codepoints;
		std::string str_literal;
		u32         line{0};
		u32         cursor{0}; // cursor pos in line
		Token       type;
	};

	struct registered_symbol
	{
		char32_t literal;
		Token    type;
	};

	// TODO: constexpr map?
	constexpr std::array<registered_symbol, 21> rsymbols = {{
	  // Compare
	  {'=', Token::EQUAL},
	  {'+', Token::PLUS},
	  {'-', Token::MINUS},
	  {'*', Token::STAR},
	  {'/', Token::SLASH},
	  {'%', Token::PERCENT},
	  {'<', Token::LESSER},
	  {'>', Token::GREATER},
	  {'|', Token::PIPE},

	  // Separators
	  {'(', Token::LEFT_PAREN},
	  {')', Token::RIGHT_PAREN},
	  {'[', Token::LEFT_BRACKET},
	  {']', Token::RIGHT_BRACKET},
	  {'{', Token::LEFT_BRACE},
	  {'}', Token::RIGHT_BRACE},
	  {'.', Token::DOT},
	  {',', Token::COMMA},
	  {':', Token::COLON},
	  {';', Token::SEMI_COLON},
	  {'#', Token::HASH},
	  //
	  {'\\', Token::BACK_SLASH},
	}};

	class lexer
	{
	public:
		using tokens = std::vector<token>;

		lexer() = default;

		lexer(codepoints cp) noexcept { codepoints = cp.data(true); }

		lexer(fs::path f) noexcept
			: input(f)
		{
			utf8::codepoints cps = input.data();
			codepoints           = cps.data(true);
			m_tokens.reserve(codepoints.size());
			filename = input.filename().string();
		}

		explicit lexer(std::string_view str) noexcept
		{
			utf8::codepoints cps(str.data());
			codepoints = cps.data(true);
			m_tokens.reserve(codepoints.size());
			filename = "";
		}

		bool has_data(u32 offset = 0) const noexcept { return (index + offset) < codepoints.size(); }

		char32_t peek(u32 offset = 0) noexcept { return next(offset); }

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
			while (has_data())
			{

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

				cursor += 1;
				index += 1;
			}

			insert_token(Token::EOF, {0});

			return m_tokens;
		}

		void read_whitespace() noexcept
		{
			//
			while (utf8::is_whitespace(peek()))
			{
				// TODO: emit EOL
				if (utf8::is_ascii_newline(peek(0)) and utf8::is_ascii_newline(peek(1)))
				{
					next(2);
					line += 1;
					cursor = 0;
					continue;
				}

				if (utf8::is_ascii_newline(peek(0)) and not utf8::is_ascii_newline(peek(1)))
				{
					next(1);
					line += 1;
					cursor = 0;
					continue;
				}
				next();
			}
		}

		void read_number() noexcept
		{
			literals lit;
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

		void read_identifier() noexcept
		{
			literals lit;

			lit.push_back(next());

			while (not eof())
			{
				if (auto n = peek(); utf8::is_identifier_continue(n))
					lit.push_back(next());
				else
					break;
			}
			insert_token(Token::IDENTIFIER, lit);
		}

		void read_symbol() noexcept
		{
			//
			literals lit;
			Token    type   = Token::EOF;
			auto     symbol = next();

			for (const auto& rs : rsymbols)
			{
				if (symbol == rs.literal)
				{
					type = rs.type;
					lit.push_back(symbol);
					break;
				}
			}

			assert::check(type != Token::EOF);

			insert_token(type, lit);
		}

		void insert_token(Token type, const literals& literal) noexcept
		{
			std::string s;
			for (const auto& c : literal)
				s += (char)c;

			token t{
			  //
			  .literal_codepoints = literal,
			  .str_literal        = s,
			  .line               = line,
			  .cursor             = cursor,
			  .type               = type
			  //
			};

			m_tokens.emplace_back(t);
		}


	private:
		utf8file              input;
		std::vector<char32_t> codepoints;
		u32                   index{0};
		tokens                m_tokens;
		std::string           filename;

		u32 line{0};
		u32 cursor{0};


		// TODO: utf8 keywords
		std::vector<std::string> keywords;
	};
} // namespace deckard::lexer
