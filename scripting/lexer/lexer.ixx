export module deckard.lexer;
import deckard.types;
import deckard.as;
import deckard.utf8;
import std;

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

	enum class token_type : u8
	{
		INTEGER,        //
		FLOATING_POINT, //
		KEYWORD,        //
		IDENTIFIER,     //
		CHARACTER,      // 'a'
		STRING,         // "abc"

		// Op
		PLUS,       // +
		MINUS,      // -
		STAR,       // *
		SLASH,      // /
		PERCENT,    // %

		BACK_SLASH, // '\'
		BANG,       // !

		EQUAL,      // =

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
		DOT,
		COMMA,
		COLON,
		SEMI_COLON,

		// Brackets
		LEFT_PAREN,
		RIGHT_PAREN,   // ()
		LEFT_BRACE,
		RIGHT_BRACE,   // {}
		LEFT_BRACKET,
		RIGHT_BRACKET, // []

		//
		EOL,
		EOF,
	};

	struct token
	{
		std::string_view literal;
		std::string_view filename;
		token_type       type;
		u32              line{0};
		u32              cursor{0}; // cursor pos in line
	};

	class lexer
	{
	public:
		lexer(std::string_view i) { input = std::span{as<u8*>(i.data()), i.size()}; }

		lexer(std::span<u8> i)
			: input(i)
		{
		}


	private:
		std::span<u8> input;
	};
} // namespace deckard::lexer
