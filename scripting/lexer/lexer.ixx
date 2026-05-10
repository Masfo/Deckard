export module deckard.lexer;
import std;
import deckard.utf8;
import deckard.as;
import deckard.debug;
import deckard.assert;
import deckard.helpers;
import deckard.types;
import deckard.file;
import deckard.sbo;
import deckard.utils.hash;

namespace fs = std::filesystem;
using namespace std::string_view_literals;

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



		Lexer as a generator, generate tokens to parser, parser can peek tokens, lookahead, etc.

		✅⚠️❌❗




		parser -> request next token from lexer



	 */

	export enum class TokenType : u8 {
		// Types
		Integer,       // -1, 1
		FloatingPoint, // -3.14, 3.14
		Identifier,    // x, color, UP
		Character,     // 'a', '🌍'
		String,        // "abc", "🌍🌍"

					   // TODO
		Keyword, // if, for, fn, return


		// Symbols
		Plus,               // +
		PlusPlus,           // ++
		Minus,              // -
		MinusMinus,         // --
		Star,               // *
		Percent,            // %
		Question,           // ?
		Bang,               // !
		At,                 // @
		Dollar,             // $^
		ShiftLeft,          // <<
		ShiftRight,         // >>
		ShiftLeftEqual,     // <<=
		ShiftRightEqual,    // >>=
		//Underscore,         // _
		BackSlash,          // '\'
		BackSlashBackSlash, // '\\'
		Slash,              // /
		SlashSlash,         // //
		SlashStar,          // /*
		StarSlash,          // */

		// TODO
		// TripleQuote,        // """

		Equal, // =

		// Compare
		LessThan,         // <
		GreaterThan,      // >
		LessThanEqual,    // <=
		GreaterThanEqual, // >=
		EqualEqual,       // ==

		// LesserEqualGreater,  // <=>
		// EqualEqualEqual,     // ===

		PlusEqual,     // +=
		MinusEqual,    // -=
		StarEqual,     // *=
		SlashEqual,    // /=
		PercentEqual,  // %=
		BangEqual,     // !=
		XorEqual,      // ^=
		QuestionEqual, // ?=
		AndEqual,      // &=
		OrEqual,       // |=


		//
		Dot,       // .
		DotDot,    // ..
		Ellipsis,  // ...
		Comma,     // ,
		Colon,     // :
		Semicolon, // ;
		Hash,      // #
		Arrow,     // ->

		Or,        // |
		And,       // &
		Xor,       // ^
		Tilde,     // ~
		OrOr,      // ||
		AndAnd,    // &&

		// brackets
		LeftParen,                       // (
		RightParen,                      // )
		LeftBrace,                       // {
		RightBrace,                      // }
		LeftBracket,                     // [
		RightBracket,                    // ]
		LeftAngleBracket  = LessThan,    // <
		RightAngleBracket = GreaterThan, // >

		// LeftBracketLeftBracket,   // [[
		// RightBracketRightBracket, // ]]

		//
		NEWLINE = RightBracket + 1, // continue from last symbol
		EOF,
		UNKNOWN,

		// always last
		TokenCount
	};

	export enum class TokenError {
		None,
		InvalidInteger,
		InvalidHex,
		InvalidFloatingPoint,
		InvalidBinary,
		InvalidString,
		InvalidCharacter,
		InvalidStringEscape,


		// Must be last
		TokenErrorCount
	};

	static_assert(static_cast<int>(TokenType::TokenCount) == 65, "Token count changed, update to_string");

	export std::string_view to_string(TokenType type)
	{
		switch (type)
		{
			case TokenType::Integer: return "Integer"sv;
			case TokenType::FloatingPoint: return "FloatingPoint"sv;
			case TokenType::Identifier: return "Identifier"sv;
			case TokenType::Character: return "Character"sv;
			case TokenType::String: return "String"sv;
			case TokenType::Keyword: return "Keyword"sv;

			case TokenType::Plus: return "Plus"sv;
			case TokenType::PlusPlus: return "PlusPlus"sv;
			case TokenType::Minus: return "Minus"sv;
			case TokenType::MinusMinus: return "MinusMinus"sv;
			case TokenType::Star: return "Star"sv;
			case TokenType::Percent: return "Percent"sv;
			case TokenType::Question: return "Question"sv;
			case TokenType::Bang: return "Bang"sv;
			case TokenType::At: return "At"sv;
			case TokenType::Dollar: return "Dollar"sv;
			case TokenType::ShiftLeft: return "ShiftLeft"sv;
			case TokenType::ShiftRight: return "ShiftRight"sv;
			case TokenType::ShiftLeftEqual: return "ShiftLeftEqual"sv;
			case TokenType::ShiftRightEqual: return "ShiftRightEqual"sv;

			//case TokenType::Underscore: return "Underscore"sv;
			case TokenType::BackSlash: return "BackSlash"sv;
			case TokenType::BackSlashBackSlash: return "BackSlashBackSlash"sv;
			case TokenType::Slash: return "Slash"sv;
			case TokenType::SlashSlash: return "SlashSlash"sv;
			case TokenType::SlashStar: return "SlashStar"sv;
			case TokenType::StarSlash:
				return "StarSlash"sv;

				//	case TokenType::TripleQuote: return "TripleQuote"sv;

			case TokenType::Equal: return "Equal"sv;

			case TokenType::LessThan: return "LessThan"sv;
			case TokenType::GreaterThan: return "GreaterThan"sv;
			case TokenType::LessThanEqual: return "LessThanEqual"sv;
			case TokenType::GreaterThanEqual: return "GreaterThanEqual"sv;
			case TokenType::EqualEqual: return "EqualEqual"sv;

			case TokenType::PlusEqual: return "PlusEqual"sv;
			case TokenType::MinusEqual: return "MinusEqual"sv;
			case TokenType::StarEqual: return "StarEqual"sv;
			case TokenType::SlashEqual: return "SlashEqual"sv;
			case TokenType::PercentEqual: return "PercentEqual"sv;
			case TokenType::BangEqual: return "BangEqual"sv;
			case TokenType::XorEqual: return "XorEqual"sv;
			case TokenType::QuestionEqual: return "QuestionEqual"sv;
			case TokenType::AndEqual: return "AndEqual"sv;
			case TokenType::OrEqual: return "OrEqual"sv;

			case TokenType::Dot: return "Dot"sv;
			case TokenType::DotDot: return "DotDot"sv;
			case TokenType::Ellipsis: return "Ellipsis"sv;
			case TokenType::Comma: return "Comma"sv;
			case TokenType::Colon: return "Colon"sv;
			case TokenType::Semicolon: return "Semicolon"sv;
			case TokenType::Hash: return "Hash"sv;
			case TokenType::Arrow: return "Arrow"sv;

			case TokenType::Or: return "Or"sv;
			case TokenType::And: return "And"sv;
			case TokenType::Xor: return "Xor"sv;
			case TokenType::Tilde: return "Tilde"sv;
			case TokenType::OrOr: return "OrOr"sv;
			case TokenType::AndAnd: return "AndAnd"sv;

			case TokenType::LeftParen: return "LeftParen"sv;
			case TokenType::RightParen: return "RightParen"sv;
			case TokenType::LeftBrace: return "LeftBrace"sv;
			case TokenType::RightBrace: return "RightBrace"sv;
			case TokenType::LeftBracket: return "LeftBracket"sv;
			case TokenType::RightBracket:
				return "RightBracket"sv;
				// case TokenType::LeftAngleBracket: return "LeftAngleBracket"sv;
				// case TokenType::RightAngleBracket: return "RightAngleBracket"sv;

			case TokenType::NEWLINE: return "Newline"sv;
			case TokenType::EOF: return "EOF"sv;
			case TokenType::UNKNOWN: return "Unknown token"sv;
		}

		return "Invalid token"sv;
	}

	using Lexeme = std::span<const u8>;

	struct LexemeHash
	{
		using is_transparent = void;

		size_t operator()(Lexeme s) const { return utils::hash_values(s); }
	};

	struct LexemeEqual
	{
		using is_transparent = void;

		bool operator()(Lexeme a, Lexeme b) const { return std::ranges::equal(a, b); }
	};

	// Deduplicated storage for lexemes (identifiers, keywords, string literals)
	export struct lexemepool
	{
		std::deque<std::vector<u8>>                              storage;
		std::unordered_map<Lexeme, u32, LexemeHash, LexemeEqual> lookup;

		lexemepool() { lookup.reserve(1024); }

		u32 add(Lexeme view)
		{
			auto it = lookup.find(view);
			if (it != lookup.end())
				return it->second;

			u32 id = as<u32>(storage.size());
			storage.emplace_back(view.begin(), view.end());

			Lexeme key{storage.back().data(), storage.back().size()};
			lookup[key] = id;

			return id;
		}

		u32 add(std::string_view str) { return add(Lexeme{as<const u8*>(str.data()), str.size()}); }

		u32 add(utf8::string str) { return add(Lexeme{str.data().data(), str.data().size()}); }

		Lexeme get(u32 id) const
		{
			if (id >= storage.size())
				return {};

			assert::check(not storage[id].empty(), std::format("Lexeme ID {} is empty", id));

			return {storage[id].data(), storage[id].size()};
		}

		Lexeme operator[](u32 id) const { return get(id); }

		u32 size() const { return as<u32>(storage.size()); }
	};

	export struct Token
	{
		u32        line;
		u32        column;
		u32        offset; // Byte offset into the buffer
		u32        length; // Length in codepoints
		TokenType  type{TokenType::TokenCount};
		TokenError error{TokenError::None};
	};

	template<typename T>
	concept codepoint_predicate = requires(T&& v, char32 cp) {
		{ v(cp) } -> std::same_as<bool>;
	};

	export std::expected<utf8::view, std::string_view> extract_token(utf8::view buffer, Token t)
	{
		if (t.offset >= buffer.size_in_bytes())
			return std::unexpected("Token offset is out of bounds");

		u32 available_bytes = as<u32>(buffer.size_in_bytes()) - t.offset;

		if (t.length > available_bytes)
			return std::unexpected("Token length exceeds available bytes in buffer");

		return buffer.subview_bytes(t.offset, buffer.size_in_bytes() - t.offset).subview(t.length);
	}



	// ########################################################################

	struct TokenizeConfig
	{
		utf8::string single_line_comment_start{"//"};
	};




	// ########################################################################

	export std::generator<Token> tokenize(utf8::view buffer, const TokenizeConfig config = {})
	{
		using namespace deckard::utf8::basic_characters;
		utf8::view cursor = buffer;
		u32        line   = 1;
		u32        column = 1;
		TokenType  type   = TokenType::EOF;


		auto give_token = [&](TokenType type, u32 length = 1, TokenError error = TokenError::None)
		{
			u32  offset = as<u32>(cursor - buffer);
			auto ret =
			  Token{.line = line, .column = column, .offset = offset, .length = length, .type = type, .error = error};
			column += length;
			cursor += length;
			return ret;
		};

		while (cursor.has_next())
		{
			TokenError error   = TokenError::None;
			char32     current = *cursor;
			u32        offset  = as<u32>(cursor - buffer);

			if (current == LINE_FEED)
			{
				line += 1;
				column = 1;
				++cursor;
				continue;
			}
			else if (current == CARRIAGE_RETURN)
			{
				++cursor;
				if (cursor.has_next() and *cursor == LINE_FEED)
					++cursor;

				line += 1;
				column = 1;
				continue;
			}

			if (utf8::is_whitespace(current))
			{
				++cursor;
				column += 1;
				continue;
			}


			// ###########################################################################################################
			// ###########################################################################################################
			// ###########################################################################################################


			if (current == APOSTROPHE)
			{
				u32  start_column  = column;
				u32  codepoints    = 1;
				bool found_closing = false;
				bool is_invalid    = false;

				++cursor;

				while (cursor.has_next() and *cursor != APOSTROPHE)
				{
					if (*cursor == REVERSE_SOLIDUS)
					{
						++cursor;
						codepoints += 1;
						if (cursor.has_next())
						{
							if (*cursor == LATIN_SMALL_LETTER_X)
							{
								++cursor;
								codepoints += 1;
								int hex_count = 0;
								while (cursor.has_next() and utf8::is_ascii_hex_digit(*cursor))
								{
									++cursor;
									codepoints += 1;
									++hex_count;
								}
								if (hex_count == 0 or hex_count > 2)
									is_invalid = true;
							}
							else
							{
								++cursor;
								codepoints += 1;
							}
						}
					}
					else
					{
						++cursor;
						codepoints += 1;
					}
				}

				if (cursor.has_next() and *cursor == APOSTROPHE)
				{
					++cursor;
					codepoints += 1;
					found_closing = true;
				}

				type = TokenType::Character;
				if (is_invalid or not found_closing)
					error = TokenError::InvalidCharacter;

				co_yield Token{
				  .line   = line,
				  .column = start_column,
				  .offset = offset,
				  .length = codepoints,
				  .type   = type,
				  .error  = error};

				column += codepoints;
				continue;
			}

			// ###########################################################################################################
			// ###########################################################################################################


			if (current == QUOTATION_MARK)
			{
				u32  start_column = column;
				u32  codepoints   = 1;
				bool is_invalid   = false;

				++cursor;

				while (cursor.has_next() and *cursor != QUOTATION_MARK)
				{
					if (*cursor == LINE_FEED or *cursor == CARRIAGE_RETURN)
					{
						is_invalid = true;
						break;
					}

					if (*cursor == REVERSE_SOLIDUS)
					{
						++cursor;
						codepoints += 1;
						if (cursor.has_next())
						{
							if (*cursor == LATIN_SMALL_LETTER_X)
							{
								++cursor;
								codepoints += 1;
								int hex_count = 0;

								while (cursor.has_next() and utf8::is_ascii_hex_digit(*cursor))
								{
									++cursor;
									codepoints += 1;
									++hex_count;
								}
								if (hex_count == 0 or hex_count > 2)
									is_invalid = true;
							}
							else
							{
								++cursor;
								codepoints += 1;
							}
						}
						else
							is_invalid = true;
					}
					else
					{
						++cursor;
						codepoints += 1;
					}
				}

				bool found_closing = false;
				if (cursor.has_next() and *cursor == QUOTATION_MARK)
				{
					++cursor;
					codepoints += 1;
					found_closing = true;
				}

				TokenType token_type = TokenType::String;
				if (is_invalid or not found_closing)
					error = TokenError::InvalidString;

				co_yield Token{
				  .line   = line,
				  .column = start_column,
				  .offset = offset,
				  .length = codepoints,
				  .type   = token_type,
				  .error  = error};

				column += codepoints;
				continue;
			}

			// ###########################################################################################################
			// ###########################################################################################################


			if (utf8::is_ascii_digit(current))
			{
				u32       start_column = column;
				u32       start_offset = offset;
				u32       codepoints   = 0;
				type         = TokenType::Integer;

				if (current == DIGIT_ZERO and cursor.peek(1))
				{
					auto next = cursor.peek(1);
					if (next and (*next == LATIN_SMALL_LETTER_X or *next == LATIN_CAPITAL_LETTER_X))
					{
						// hex
						cursor += 2;
						column += 2;
						codepoints += 2;

						u32 digits = 0;
						while (cursor.has_next() and (utf8::is_ascii_hex_digit(*cursor) or *cursor == LOW_LINE))
						{
							if (*cursor != LOW_LINE)
								digits += 1;

							codepoints += 1;
							column += 1;
							++cursor;
						}

						if (digits == 0)
						{
							// greedy, better error messages
							while (cursor.has_next() and utf8::is_xid_continue(*cursor))
							{
								codepoints += 1;
								column += 1;
								++cursor;
							}

							[[maybe_unused]] auto number_view =
							  buffer.subview_bytes(start_offset, as<u32>(cursor - buffer) - start_offset);

							co_yield Token{
							  .line   = line,
							  .column = start_column,
							  .offset = start_offset,
							  .length = codepoints,
							  .type   = TokenType::Integer,
							  .error  = TokenError::InvalidHex};
						}
						else
						{
							co_yield Token{
							  .line   = line,
							  .column = start_column,
							  .offset = start_offset,
							  .length = codepoints,
							  .type   = TokenType::Integer,
							  .error  = TokenError::None};
						}

						continue;
					}
					else if (*next == LATIN_SMALL_LETTER_B or *next == LATIN_CAPITAL_LETTER_B)
					{
						// binary
						cursor += 2;
						column += 2;
						codepoints += 2;

						u32 digits = 0;

						while (cursor.has_next() and (*cursor == DIGIT_ZERO or *cursor == DIGIT_ONE or *cursor == LOW_LINE))
						{
							if (*cursor != LOW_LINE)
								digits += 1;

							codepoints += 1;
							column += 1;
							++cursor;
						}

						[[maybe_unused]] auto number_view = buffer.subview_bytes(start_offset, as<u32>(cursor - buffer) - start_offset);

						if (digits == 0)
						{
							error = TokenError::InvalidBinary;

							// greedy, better error messages
							while (cursor.has_next() and utf8::is_xid_continue(*cursor))
							{
								codepoints += 1;
								column += 1;
								++cursor;
							}
						}

						co_yield Token{
						  .line   = line,
						  .column = start_column,
						  .offset = start_offset,
						  .length = codepoints,
						  .type   = type,
						  .error  = error};
						continue;
					}
				}

				// integer part
				type = TokenType::Integer;
				while (cursor.has_next() and (utf8::is_ascii_digit(*cursor) or *cursor == LOW_LINE))
				{
					codepoints += 1;
					column += 1;
					++cursor;
				}

				// fractional part
				if (cursor.has_next() and *cursor == FULL_STOP)
				{
					if (cursor.peek(1) != FULL_STOP)
					{

						type = TokenType::FloatingPoint;
						codepoints += 1;
						column += 1;
						++cursor;

						// u32 fraction_digits = 0;
						while (cursor.has_next() and (utf8::is_ascii_digit(*cursor) or *cursor == LOW_LINE))
						{
							// if (*cursor != LOW_LINE)
							//	fraction_digits += 1;

							codepoints += 1;
							column += 1;
							++cursor;
						}

						// if (fraction_digits == 0)
						//	type = TokenType::InvalidFloatingPoint;
					}
				}

				// Exponent part
				if (cursor.has_next() and (*cursor == LATIN_SMALL_LETTER_E or *cursor == LATIN_CAPITAL_LETTER_E))
				{
					type = TokenType::FloatingPoint;
					codepoints += 1;
					column += 1;
					++cursor;
					if (cursor.has_next() and (*cursor == PLUS_SIGN or *cursor == HYPHEN_MINUS))
					{
						codepoints += 1;
						column += 1;
						++cursor;
					}

					u32 exponent_digits = 0;

					while (cursor.has_next() and (utf8::is_ascii_digit(*cursor) or *cursor == LOW_LINE))
					{
						if (*cursor != LOW_LINE)
							exponent_digits += 1;

						codepoints += 1;
						column += 1;
						++cursor;
					}

					if (exponent_digits == 0)
						error = TokenError::InvalidFloatingPoint;
				}

				// TODO: check for trailing types (ull, f)

				bool has_garbage = false;
				if (cursor.has_next())
				{
					char32 peek = *cursor;
					if (utf8::is_xid_start(peek) or (peek == FULL_STOP and cursor.peek(1) != FULL_STOP))
					{
						has_garbage = true;
						while (cursor.has_next() and (utf8::is_xid_continue(*cursor) or *cursor == FULL_STOP))
						{
							codepoints += 1;
							column += 1;
							++cursor;
						}
					}
				}

				if (has_garbage)
				{
					if (type == TokenType::Integer)
						error = TokenError::InvalidInteger;
					else if (type == TokenType::FloatingPoint)
						error = TokenError::InvalidFloatingPoint;
				}


				[[maybe_unused]] auto number_view = buffer.subview_bytes(start_offset, as<u32>(cursor - buffer) - start_offset);

				co_yield Token{
				  .line   = line,
				  .column = start_column,
				  .offset = start_offset,
				  .length = codepoints,
				  .type   = type,
				  .error  = error};
				continue;
			}

			// ###########################################################################################################
			// ###########################################################################################################


			// Identifier/keyword
			if (utf8::is_xid_start(current))
			{
				u32 start_column = column;
				u32 start_offset = offset;
				u32 codepoints   = 1;
				++cursor;
				column += 1;

				while (cursor.has_next() and utf8::is_xid_continue(*cursor))
				{
					++cursor;
					column += 1;
					codepoints += 1;
				}


				[[maybe_unused]] auto identifier_view = buffer.subview_bytes(start_offset, as<u32>(cursor - buffer) - start_offset);

				bool is_keyword = false;


				co_yield Token{
				  .line   = line,
				  .column = start_column,
				  .offset = start_offset,
				  .length = codepoints,
				  .type   = is_keyword ? TokenType::Keyword : TokenType::Identifier};


				continue;
			}

			// ###########################################################################################################
			// ###########################################################################################################
			// Symbols

			switch (current)
			{
				using enum TokenType;
				// #######################################################################################################


				// #######################################################################################################
				case HASH:
				{
					co_yield give_token(Hash, 1);
					continue;
				}
				case PLUS_SIGN:
				{
					if (cursor.peek(1) == PLUS_SIGN)
						co_yield give_token(PlusPlus, 2);
					else if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(PlusEqual, 2);
					else
						co_yield give_token(Plus, 1);
					continue;
				}

				case HYPHEN_MINUS:
				{
					if (cursor.peek(1) == HYPHEN_MINUS)
						co_yield give_token(MinusMinus, 2);
					else if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(MinusEqual, 2);
					else if (cursor.peek(1) == GREATER_THAN_SIGN)
						co_yield give_token(Arrow, 2);
					else
						co_yield give_token(Minus, 1);
					continue;
				}

				case ASTERISK:
				{
					if (cursor.peek(1) == ASTERISK)
						co_yield give_token(SlashStar, 2);
					else if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(StarEqual, 2);
					else if (cursor.peek(1) == SOLIDUS)
						co_yield give_token(StarSlash, 2);
					else
						co_yield give_token(Star, 1);
					continue;
				}

				case SOLIDUS: // Slash
				{
					if (cursor.peek(1) == SOLIDUS)
						co_yield give_token(SlashSlash, 2);
					else if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(SlashEqual, 2);
					else if (cursor.peek(1) == ASTERISK)
						co_yield give_token(SlashStar, 2);
					else
						co_yield give_token(Slash, 1);
					continue;
				}

				case REVERSE_SOLIDUS: // Backslash
				{
					if (cursor.peek(1) == REVERSE_SOLIDUS)
						co_yield give_token(BackSlashBackSlash, 2);
					else
						co_yield give_token(BackSlash, 1);
					continue;
				}


				// #######################################################################################################
				// Extended symbols
				case PERCENT_SIGN:
				{
					if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(PercentEqual, 2);
					else
						co_yield give_token(Percent, 1);
					continue;
				}
				case QUESTION_MARK:
				{
					co_yield give_token(Question, 1);
					continue;
				}

				case EXCLAMATION_MARK:
				{
					co_yield (cursor.peek(1) == EQUALS_SIGN) ? give_token(BangEqual, 2) : give_token(Bang, 1);
					continue;
				}
				case COMMERCIAL_AT:
				{
					co_yield give_token(At, 1);
					continue;
				}
					// Dollar sign can be used as identifier
					// case DOLLAR_SIGN:
					//{
					//	co_yield give_token(Dollar, 1);
					//	continue;
					//}

				case LESS_THAN_SIGN:
				{
					if (cursor.peek(1) == LESS_THAN_SIGN)
					{
						if (cursor.peek(2) == EQUALS_SIGN)
							co_yield give_token(ShiftLeftEqual, 3);
						else
							co_yield give_token(ShiftLeft, 2);
					}
					else if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(LessThanEqual, 2);
					else
						co_yield give_token(LessThan, 1);
					continue;
				}

				case GREATER_THAN_SIGN:
				{
					if (cursor.peek(1) == GREATER_THAN_SIGN)
					{
						if (cursor.peek(2) == EQUALS_SIGN)
							co_yield give_token(ShiftRightEqual, 3);
						else
							co_yield give_token(ShiftRight, 2);
					}
					else if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(GreaterThanEqual, 2);
					else
						co_yield give_token(GreaterThan, 1);
					continue;
				}

				//case LOW_LINE:
				//{
				//	co_yield give_token(Underscore, 1);
				//	continue;
				//}

					// #######################################################################################################
					// Brackets and braces

				case LEFT_PARENTHESIS:
				{
					co_yield give_token(LeftParen, 1);
					continue;
				}
				case RIGHT_PARENTHESIS:
				{
					co_yield give_token(RightParen, 1);
					continue;
				}

				case LEFT_CURLY_BRACKET:
				{
					co_yield give_token(LeftBrace, 1);
					continue;
				}
				case RIGHT_CURLY_BRACKET:
				{
					co_yield give_token(RightBrace, 1);
					continue;
				}

				case LEFT_SQUARE_BRACKET:
				{
					co_yield give_token(LeftBracket, 1);
					continue;
				}

				case RIGHT_SQUARE_BRACKET:
				{
					co_yield give_token(RightBracket, 1);
					continue;
				}

					// #######################################################################################################
				case EQUALS_SIGN:
				{
					if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(EqualEqual, 2);
					else
						co_yield give_token(Equal, 1);
					continue;
				}

				case FULL_STOP:
				{
					if (cursor.peek(1) == FULL_STOP)
					{
						if (cursor.peek(2) == FULL_STOP)
							co_yield give_token(Ellipsis, 3);
						else
							co_yield give_token(DotDot, 2);
					}
					else
						co_yield give_token(Dot, 1);
					continue;
				}

				case COMMA:
				{
					co_yield give_token(Comma, 1);
					continue;
				}

				case COLON:
				{
					co_yield give_token(Colon, 1);
					continue;
				}
				case SEMICOLON:
				{
					co_yield give_token(Semicolon, 1);
					continue;
				}

				case VERTICAL_LINE:
				{
					if (cursor.peek(1) == VERTICAL_LINE)
						co_yield give_token(OrOr, 2);
					else if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(OrEqual, 2);
					else
						co_yield give_token(Or, 1);
					continue;
				}

				case AMPERSAND:
				{
					if (cursor.peek(1) == AMPERSAND)
						co_yield give_token(AndAnd, 2);
					else if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(AndEqual, 2);
					else
						co_yield give_token(And, 1);
					continue;
				}

				case CIRCUMFLEX_ACCENT:
				{
					if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(XorEqual, 2);
					else
						co_yield give_token(Xor, 1);
					continue;
				}

				case TILDE:
				{
					co_yield give_token(Tilde, 1);
					continue;
				}


				// #######################################################################################################
				default:
				{
					co_yield give_token(TokenType::UNKNOWN, 1);
					continue;
				}
			}
		}

		co_return;
	}


} // namespace deckard::lexer
