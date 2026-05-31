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
import deckard.stringpool;

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

	export enum class TokenKind : u8 {
		// Types
		Integer,       // -1, 1, 0x1_23_4
		FloatingPoint, // -3.14, 3.14,
		Identifier,    // x, color, UP, _Under, $dollar, café, π
		Character,     // 'a', '🌍'
		String,        // "abc", "🌍🌍"
		Comment,       // user defined

		Keyword,       // if, for, fn, return

		// Symbols
		Plus,       // +
		PlusPlus,   // ++
		Minus,      // -
		MinusMinus, // --
		Star,       // *
		Percent,    // %
		Question,   // ?
		Bang,       // !
		At,         // @

					//	Dollar,          // $ - part of identifier

		ShiftLeft,       // <<
		ShiftRight,      // >>
		ShiftLeftEqual,  // <<=
		ShiftRightEqual, // >>=
		// Underscore,         // _
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

		PlusEqual,    // +=
		MinusEqual,   // -=
		StarEqual,    // *=
		SlashEqual,   // /=
		PercentEqual, // %=
		BangEqual,    // !=
		XorEqual,     // ^=
		// QuestionEqual, // ?=
		AndEqual, // &=
		OrEqual,  // |=


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
		LeftParen,    // (
		RightParen,   // )
		LeftBrace,    // {
		RightBrace,   // }
		LeftBracket,  // [
		RightBracket, // ]

		//
		EOF,
		Unknown,

		// always last
		TokenCount
	};
	static_assert(static_cast<int>(TokenKind::TokenCount) == 63, "Token count changed, update to_string");

	export enum class TokenGroup : u8 {
		Literal,     // Integer, FloatingPoint, Character, String
		Identifier,  // Identifier
		Keyword,     // Keyword
		Comment,     // Comment
		Operator,    // Arithmetic, comparison, bitwise, assignment operators
		Punctuation, // Dot, DotDot, Ellipsis, Comma, Colon, Semicolon, Hash, Arrow
		Bracket,     // LeftParen, RightParen, LeftBrace, RightBrace, LeftBracket, RightBracket
		Unknown      // Unknown, EOF
	};

	export std::string_view to_string(TokenGroup group)
	{
		switch (group)
		{
			case TokenGroup::Literal: return "Literal"sv;
			case TokenGroup::Identifier: return "Identifier"sv;
			case TokenGroup::Keyword: return "Keyword"sv;
			case TokenGroup::Comment: return "Comment"sv;
			case TokenGroup::Operator: return "Operator"sv;
			case TokenGroup::Punctuation: return "Punctuation"sv;
			case TokenGroup::Bracket: return "Bracket"sv;
			case TokenGroup::Unknown: return "Unknown"sv;
		}
		return "Invalid group"sv;
	}

	export constexpr TokenGroup kind_to_group(TokenKind kind) noexcept
	{
		switch (kind)
		{
			case TokenKind::Integer: [[fallthrough]];
			case TokenKind::FloatingPoint: [[fallthrough]];
			case TokenKind::Character: [[fallthrough]];
			case TokenKind::String: return TokenGroup::Literal;

			case TokenKind::Identifier: return TokenGroup::Identifier;

			case TokenKind::Keyword: return TokenGroup::Keyword;

			case TokenKind::Comment: return TokenGroup::Comment;

			case TokenKind::Plus: [[fallthrough]];
			case TokenKind::PlusPlus: [[fallthrough]];
			case TokenKind::Minus: [[fallthrough]];
			case TokenKind::MinusMinus: [[fallthrough]];
			case TokenKind::Star: [[fallthrough]];
			case TokenKind::Percent: [[fallthrough]];
			case TokenKind::Question: [[fallthrough]];
			case TokenKind::Bang: [[fallthrough]];
			case TokenKind::At: [[fallthrough]];
			case TokenKind::ShiftLeft: [[fallthrough]];
			case TokenKind::ShiftRight: [[fallthrough]];
			case TokenKind::ShiftLeftEqual: [[fallthrough]];
			case TokenKind::ShiftRightEqual: [[fallthrough]];
			case TokenKind::BackSlash: [[fallthrough]];
			case TokenKind::BackSlashBackSlash: [[fallthrough]];
			case TokenKind::Slash: [[fallthrough]];
			case TokenKind::SlashSlash: [[fallthrough]];
			case TokenKind::SlashStar: [[fallthrough]];
			case TokenKind::StarSlash: [[fallthrough]];
			case TokenKind::Equal: [[fallthrough]];
			case TokenKind::LessThan: [[fallthrough]];
			case TokenKind::GreaterThan: [[fallthrough]];
			case TokenKind::LessThanEqual: [[fallthrough]];
			case TokenKind::GreaterThanEqual: [[fallthrough]];
			case TokenKind::EqualEqual: [[fallthrough]];
			case TokenKind::PlusEqual: [[fallthrough]];
			case TokenKind::MinusEqual: [[fallthrough]];
			case TokenKind::StarEqual: [[fallthrough]];
			case TokenKind::SlashEqual: [[fallthrough]];
			case TokenKind::PercentEqual: [[fallthrough]];
			case TokenKind::BangEqual: [[fallthrough]];
			case TokenKind::XorEqual: [[fallthrough]];
			case TokenKind::AndEqual: [[fallthrough]];
			case TokenKind::OrEqual: [[fallthrough]];
			case TokenKind::Or: [[fallthrough]];
			case TokenKind::And: [[fallthrough]];
			case TokenKind::Xor: [[fallthrough]];
			case TokenKind::Tilde: [[fallthrough]];
			case TokenKind::OrOr: [[fallthrough]];
			case TokenKind::AndAnd: return TokenGroup::Operator;

			case TokenKind::Dot: [[fallthrough]];
			case TokenKind::DotDot: [[fallthrough]];
			case TokenKind::Ellipsis: [[fallthrough]];
			case TokenKind::Comma: [[fallthrough]];
			case TokenKind::Colon: [[fallthrough]];
			case TokenKind::Semicolon: [[fallthrough]];
			case TokenKind::Hash: [[fallthrough]];
			case TokenKind::Arrow: return TokenGroup::Punctuation;

			case TokenKind::LeftParen: [[fallthrough]];
			case TokenKind::RightParen: [[fallthrough]];
			case TokenKind::LeftBrace: [[fallthrough]];
			case TokenKind::RightBrace: [[fallthrough]];
			case TokenKind::LeftBracket: [[fallthrough]];
			case TokenKind::RightBracket: return TokenGroup::Bracket;

			case TokenKind::EOF: [[fallthrough]];
			case TokenKind::Unknown: [[fallthrough]];
			case TokenKind::TokenCount: return TokenGroup::Unknown;

			default: return TokenGroup::Unknown;
		}
	}

	export enum class TokenError {
		None,
		InvalidInteger,
		InvalidHex,
		InvalidFloatingPoint,
		InvalidBinary,

		InvalidCharacter,
		InvalidCharacterNewline,
		InvalidCharacterEmpty,

		InvalidString,
		InvalidStringEscape,
		InvalidStringZeroWidthJoiner,

		InvalidIdentifierZeroWidthJoiner,
		InvalidIdentifierNonASCII,

		UnclosedCommentBlock,
		UnopenedCommentBlock,

		InvalidCommentBlock,
		InvalidLineComment,

		// Must be last
		TokenErrorCount
	};

	export std::string_view to_string(TokenKind type)
	{
		switch (type)
		{
			case TokenKind::Integer: return "Integer"sv;
			case TokenKind::FloatingPoint: return "FloatingPoint"sv;
			case TokenKind::Identifier: return "Identifier"sv;
			case TokenKind::Character: return "Character"sv;
			case TokenKind::String: return "String"sv;
			case TokenKind::Keyword: return "Keyword"sv;
			case TokenKind::Comment: return "Comment"sv;

			case TokenKind::Plus: return "Plus"sv;
			case TokenKind::PlusPlus: return "PlusPlus"sv;
			case TokenKind::Minus: return "Minus"sv;
			case TokenKind::MinusMinus: return "MinusMinus"sv;
			case TokenKind::Star: return "Star"sv;
			case TokenKind::Percent: return "Percent"sv;
			case TokenKind::Question: return "Question"sv;
			case TokenKind::Bang: return "Bang"sv;
			case TokenKind::At: return "At"sv;
			case TokenKind::ShiftLeft: return "ShiftLeft"sv;
			case TokenKind::ShiftRight: return "ShiftRight"sv;
			case TokenKind::ShiftLeftEqual: return "ShiftLeftEqual"sv;
			case TokenKind::ShiftRightEqual:
				return "ShiftRightEqual"sv;

				// case TokenKind::Dollar: return "Dollar"sv;
				// case TokenKind::Underscore: return "Underscore"sv;

			case TokenKind::BackSlash: return "BackSlash"sv;
			case TokenKind::BackSlashBackSlash: return "BackSlashBackSlash"sv;
			case TokenKind::Slash: return "Slash"sv;
			case TokenKind::SlashSlash: return "SlashSlash"sv;
			case TokenKind::SlashStar: return "SlashStar"sv;
			case TokenKind::StarSlash:
				return "StarSlash"sv;

				//	case TokenKind::TripleQuote: return "TripleQuote"sv;

			case TokenKind::Equal: return "Equal"sv;

			case TokenKind::LessThan: return "LessThan"sv;
			case TokenKind::GreaterThan: return "GreaterThan"sv;
			case TokenKind::LessThanEqual: return "LessThanEqual"sv;
			case TokenKind::GreaterThanEqual: return "GreaterThanEqual"sv;
			case TokenKind::EqualEqual: return "EqualEqual"sv;

			case TokenKind::PlusEqual: return "PlusEqual"sv;
			case TokenKind::MinusEqual: return "MinusEqual"sv;
			case TokenKind::StarEqual: return "StarEqual"sv;
			case TokenKind::SlashEqual: return "SlashEqual"sv;
			case TokenKind::PercentEqual: return "PercentEqual"sv;
			case TokenKind::BangEqual: return "BangEqual"sv;
			case TokenKind::XorEqual:
				return "XorEqual"sv;
				//	case TokenKind::QuestionEqual: return "QuestionEqual"sv;
			case TokenKind::AndEqual: return "AndEqual"sv;
			case TokenKind::OrEqual: return "OrEqual"sv;

			case TokenKind::Dot: return "Dot"sv;
			case TokenKind::DotDot: return "DotDot"sv;
			case TokenKind::Ellipsis: return "Ellipsis"sv;
			case TokenKind::Comma: return "Comma"sv;
			case TokenKind::Colon: return "Colon"sv;
			case TokenKind::Semicolon: return "Semicolon"sv;
			case TokenKind::Hash: return "Hash"sv;
			case TokenKind::Arrow: return "Arrow"sv;

			case TokenKind::Or: return "Or"sv;
			case TokenKind::And: return "And"sv;
			case TokenKind::Xor: return "Xor"sv;
			case TokenKind::Tilde: return "Tilde"sv;
			case TokenKind::OrOr: return "OrOr"sv;
			case TokenKind::AndAnd: return "AndAnd"sv;

			case TokenKind::LeftParen: return "LeftParen"sv;
			case TokenKind::RightParen: return "RightParen"sv;
			case TokenKind::LeftBrace: return "LeftBrace"sv;
			case TokenKind::RightBrace: return "RightBrace"sv;
			case TokenKind::LeftBracket: return "LeftBracket"sv;
			case TokenKind::RightBracket: return "RightBracket"sv;

			case TokenKind::EOF: return "EOF"sv;
			case TokenKind::Unknown: return "Unknown token"sv;
		}

		return "Invalid token"sv;
	}

	struct LexemeHash
	{
		using is_transparent = void;

		size_t operator()(const utf8::view s) const { return utils::hash_values(s); }
	};

	struct LexemeEqual
	{
		using is_transparent = void;

		bool operator()(const utf8::view a, const utf8::view b) const { return a == b; }
	};

	// TODO: extract string_pool to own module


	// TODO: only use lexeme pool id for identifiers etc, instead of offset and length
	export struct Token
	{
		u32 line;
		u32 column;

		// TODO: remove offset+length, use id
		// calculated from the lexeme in pool
		u32 offset;                      // Byte offset into the buffer
		u32 length;                      // Length in codepoints

		u64        id{limits::max<u64>}; // pool index for identifiers, keywords, string literals
		TokenKind  type{TokenKind::TokenCount};
		TokenGroup group{TokenGroup::Unknown};
		TokenError error{TokenError::None};
	};

	template<typename T>
	concept codepoint_predicate = requires(T&& v, char32 cp) {
		{ v(cp) } -> std::same_as<bool>;
	};

	export std::expected<utf8::view, std::string_view> extract_token(utf8::view buffer, Token t)
	{
		if (t.offset > buffer.size_in_bytes())
			return std::unexpected("Token offset is out of bounds");

		if (t.offset == buffer.size_in_bytes() and t.length > 0)
			return std::unexpected("Token offset is at end but length > 0");

		return buffer.subview_bytes(t.offset, buffer.size_in_bytes() - t.offset).subview(0, t.length);
	}

	// ########################################################################

	export struct TokenizeConfig
	{
		utf8::string single_line_comment_start{};
		utf8::string block_comment_start{};
		utf8::string block_comment_end{};
		bool         forbid_non_ascii_in_identifiers{false};

		std::vector<utf8::string> keywords{};
	};

	// ########################################################################
	/* TODO: put extracted token string to lexemepool, and assign to Token::Id
	 * CheckToken(..) - check against the pool id
	 *
	 *  if ', then try to get codepoint (insert that to pool) from
	 *		\xHHHHHH - hex codepoint
	 *
	 *
	 */


	export namespace detail
	{
		string_pool pool;
	};

	export std::generator<Token> tokenize(utf8::view buffer, const TokenizeConfig config = {})
	{
		using namespace deckard::utf8::basic_characters;
		utf8::scanner cursor{buffer};
		u32           line   = 1;
		u32           column = 1;
		size_t        id     = limits::max<size_t>;
		TokenKind     type   = TokenKind::EOF;

		if (detail::pool.empty())
		{
			(void)detail::pool.add("__INVALID_TOKEN__"sv);
		}

		if (not config.single_line_comment_start.empty() and
			(config.single_line_comment_start == config.block_comment_start or
			 config.single_line_comment_start == config.block_comment_end))
		{
			dbg::println("Single line comment start cannot be the same as block comment start or end");
			co_yield Token{
			  .line   = line,
			  .column = column,
			  .offset = 0,
			  .length = 0,
			  .type   = TokenKind::Comment,
			  .error  = TokenError::InvalidLineComment};
			co_return;
		}

		if (not config.block_comment_start.empty() and not config.block_comment_end.empty() and
			config.block_comment_end == config.block_comment_start)
		{
			dbg::println("Block comment start and end cannot be the same");
			co_yield Token{
			  .line   = line,
			  .column = column,
			  .offset = 0,
			  .length = 0,
			  .type   = TokenKind::Comment,
			  .error  = TokenError::InvalidCommentBlock};
			co_return;
		}

		auto consume_newline = [&]() noexcept -> bool
		{
			if (not cursor.has_next())
				return false;

			const char32 cp = *cursor;
			if (cp == LINE_FEED)
			{
				++cursor;
				line += 1;
				column = 1;
				return true;
			}
			if (cp == CARRIAGE_RETURN)
			{
				++cursor;
				cursor.skip_if(LINE_FEED);
				line += 1;
				column = 1;
				return true;
			}
			return false;
		};

		auto is_newline = [](char32 cp) noexcept { return cp == LINE_FEED or cp == CARRIAGE_RETURN; };

		auto give_token = [&](TokenKind type, u32 length = 1, TokenError error = TokenError::None)
		{
			u32  offset = as<u32>(cursor - buffer);
			auto ret    = Token{
			  .line   = line,
			  .column = column,
			  .offset = offset,
			  .length = length,
			  .type   = type,
			  .group  = kind_to_group(type),
			  .error  = error};
			column += length;
			cursor += length;
			return ret;
		};

		while (cursor.has_next())
		{
			TokenError error     = TokenError::None;
			char32     current   = *cursor;
			u32        offset    = as<u32>(cursor - buffer);
			u32        line_init = line;

			if (is_newline(current))
			{
				consume_newline();
				continue;
			}

			if (utf8::is_whitespace(current))
			{
				const u32 skipped = as<u32>(
				  cursor.skip_while([&is_newline](char32 cp) { return utf8::is_whitespace(cp) and not is_newline(cp); }));
				column += skipped;
				continue;
			}

			// Comments
			if (not config.single_line_comment_start.empty() and cursor.starts_with(config.single_line_comment_start))
			{
				const u32 skipped = as<u32>(cursor.skip_while([&is_newline](char32 cp) { return not is_newline(cp); }));
				column += skipped;
				continue;
			}

			if (not config.block_comment_end.empty() and cursor.starts_with(config.block_comment_end))
			{
				co_yield give_token(
				  TokenKind::Comment, as<u32>(config.block_comment_end.length()), TokenError::UnopenedCommentBlock);
				continue;
			}

			if (not config.block_comment_start.empty() and not config.block_comment_end.empty() and
				cursor.starts_with(config.block_comment_start))
			{
				u32 nest_level   = 1;
				u32 start_line   = line;
				u32 start_column = column;
				u32 codepoints   = as<u32>(config.block_comment_start.length());

				column += as<u32>(config.block_comment_start.length());
				cursor += as<int>(config.block_comment_start.length());

				while (cursor.has_next() and nest_level > 0)
				{
					if (cursor.starts_with(config.block_comment_start))
					{
						nest_level += 1;
						column += as<u32>(config.block_comment_start.length());
						cursor += as<int>(config.block_comment_start.length());
						continue;
					}
					if (cursor.starts_with(config.block_comment_end))
					{
						nest_level -= 1;
						column += as<u32>(config.block_comment_end.length());
						cursor += as<int>(config.block_comment_end.length());
						continue;
					}
					if (is_newline(*cursor))
					{
						consume_newline();
						continue;
					}
					++cursor;
					++column;
				}

				if (nest_level > 0)
				{
					co_yield Token{
					  .line   = start_line,
					  .column = start_column,
					  .offset = offset,
					  .length = codepoints,
					  .type   = TokenKind::Comment,
					  .error  = TokenError::UnclosedCommentBlock};
				}

				continue;
			}

			// Character literal  '...'
			if (current == APOSTROPHE)
			{
				u32  start_column  = column;
				u32  codepoints    = 1;
				bool found_closing = false;
				bool is_invalid    = false;
				bool found_newline = false;

				++cursor;
				++column; // Track opening apostrophe

				while (cursor.has_next() and *cursor != APOSTROPHE)
				{
					if (is_newline(*cursor))
					{
						found_newline = true;
						id            = detail::pool.add(buffer.subview_bytes(offset, as<u32>(cursor - buffer) - offset));
						co_yield Token{
						  .line   = line,
						  .column = start_column,
						  .offset = offset,
						  .length = codepoints,
						  .id     = id,
						  .type   = TokenKind::Character,
						  .group  = TokenGroup::Literal,
						  .error  = TokenError::InvalidCharacterNewline};
						break;
					}

					if (*cursor == REVERSE_SOLIDUS)
					{
						++cursor;
						++column;
						codepoints += 1;

						if (cursor.has_next())
						{
							if (*cursor == LATIN_SMALL_LETTER_X)
							{
								++cursor;
								++column;
								codepoints += 1;
								const u32 hex_count = as<u32>(cursor.skip_while(utf8::is_ascii_hex_digit));
								column += hex_count;
								codepoints += hex_count;
								if (hex_count == 0 or hex_count > 2)
									is_invalid = true;
							}
							else
							{
								++cursor;
								++column;
								codepoints += 1;
							}
						}
					}
					else
					{
						++cursor;
						++column;
						codepoints += 1;
					}
				}

				if (not found_newline)
				{
					if (cursor.has_next() and *cursor == APOSTROPHE)
					{
						++cursor;
						++column;
						codepoints += 1;
						found_closing = true;
					}

					type = TokenKind::Character;
					if (codepoints == 2 and found_closing)
						error = TokenError::InvalidCharacterEmpty;
					else if (codepoints == 1)
						error = TokenError::None;
					else if (is_invalid or not found_closing)
						error = TokenError::InvalidCharacter;

					const utf8::view content = buffer.subview_bytes(offset, as<u32>(cursor - buffer) - offset);
					id                       = detail::pool.add(content);
					co_yield Token{
					  .line   = line,
					  .column = start_column,
					  .offset = offset,
					  .length = codepoints,
					  .id     = id,
					  .type   = type,
					  .error  = error};
				}

				continue;
			}

			// String literal  "..."
			if (current == QUOTATION_MARK)
			{
				const u32 start_line   = line_init;
				const u32 start_column = column;
				u32       codepoints   = 1;
				bool      is_invalid   = false;

				++cursor;
				column += 1;
				while (cursor.has_next() and *cursor != QUOTATION_MARK)
				{
					if (*cursor == REVERSE_SOLIDUS)
					{
						++cursor;
						column += 1;
						codepoints += 1;

						if (not cursor.has_next())
						{
							is_invalid = true;
							break;
						}

						if (*cursor == LINE_FEED)
						{
							line += 1;
							column = 1;
							++cursor;
							codepoints += 1;
							continue;
						}
						if (*cursor == CARRIAGE_RETURN)
						{
							++cursor;
							if (cursor.has_next() and *cursor == LINE_FEED)
							{
								++cursor;
								codepoints += 1;
							}
							line += 1;
							column = 1;
							codepoints += 1;
							continue;
						}
						if (*cursor == LATIN_SMALL_LETTER_X)
						{
							++cursor;
							column += 1;
							codepoints += 1;
							const u32 hex_count = as<u32>(cursor.skip_while(utf8::is_ascii_hex_digit));
							column += hex_count;
							codepoints += hex_count;
							if (hex_count == 0 or hex_count > 2)
								is_invalid = true;
						}
						else
						{
							++cursor;
							column += 1;
							codepoints += 1;
						}
					}
					else if (is_newline(*cursor))
					{
						is_invalid = true;
						break;
					}
					else
					{
						++cursor;
						column += 1;
						codepoints += 1;
					}
				}

				const bool found_closing = cursor.skip_if(QUOTATION_MARK);
				if (found_closing)
				{
					column += 1;
					codepoints += 1;
				}
				else if (not is_invalid)
				{
					is_invalid = true;
				}

				if (is_invalid)
					error = TokenError::InvalidString;

				const utf8::view content = buffer.subview_bytes(offset, as<u32>(cursor - buffer) - offset);
				id                       = detail::pool.add(content);
				co_yield Token{
				  .line   = start_line,
				  .column = start_column,
				  .offset = offset,
				  .length = codepoints,
				  .id     = id,
				  .type   = TokenKind::String,
				  .group  = TokenGroup::Literal,
				  .error  = error};
				continue;
			}

			// ###########################################################################################################
			// Numeric literals

			if (utf8::is_ascii_digit(current))
			{
				const u32 start_column = column;
				const u32 start_offset = offset;
				u32       codepoints   = 0;
				type                   = TokenKind::Integer;

				auto scan_digits = [&](auto pred) noexcept -> u32
				{
					u32 digits = 0;
					while (cursor.has_next() and (pred(*cursor) or *cursor == LOW_LINE))
					{
						if (*cursor != LOW_LINE)
							++digits;
						++cursor;
						++codepoints;
						++column;
					}
					return digits;
				};

				if (current == DIGIT_ZERO)
				{
					const auto next = cursor.peek(1);
					if (next and (*next == LATIN_SMALL_LETTER_X or *next == LATIN_CAPITAL_LETTER_X))
					{
						// Hex: 0x / 0X
						cursor += 2;
						column += 2;
						codepoints += 2;

						const u32 digits = scan_digits(utf8::is_ascii_hex_digit);
						if (digits == 0)
						{
							// Greedy scan for better error messages
							const u32 garbage = as<u32>(cursor.skip_while(utf8::is_xid_continue));
							codepoints += garbage;
							column += garbage;

							auto number_view = buffer.subview_bytes(start_offset, as<u32>(cursor - buffer) - start_offset);
							id               = detail::pool.add(number_view);
							co_yield Token{
							  .line   = line,
							  .column = start_column,
							  .offset = start_offset,
							  .length = codepoints,
							  .id     = id,
							  .type   = TokenKind::Integer,
							  .group  = TokenGroup::Literal,
							  .error  = TokenError::InvalidHex};
						}
						else
						{
							if (cursor.peek_back() == LOW_LINE)
								error = TokenError::InvalidHex;

							auto number_view = buffer.subview_bytes(start_offset, as<u32>(cursor - buffer) - start_offset);
							id               = detail::pool.add(number_view);
							co_yield Token{
							  .line   = line,
							  .column = start_column,
							  .offset = start_offset,
							  .length = codepoints,
							  .id     = id,
							  .type   = TokenKind::Integer,
							  .group  = TokenGroup::Literal,
							  .error  = error};
						}
						continue;
					}

					if (next and (*next == LATIN_SMALL_LETTER_B or *next == LATIN_CAPITAL_LETTER_B))
					{
						// Binary: 0b / 0B
						cursor += 2;
						column += 2;
						codepoints += 2;

						const u32 digits =
						  scan_digits([](char32 cp) noexcept { return cp == DIGIT_ZERO or cp == DIGIT_ONE; });
						if (digits == 0)
						{
							error             = TokenError::InvalidBinary;
							const u32 garbage = as<u32>(cursor.skip_while(utf8::is_xid_continue));
							codepoints += garbage;
							column += garbage;
						}
						else if (cursor.peek_back() == LOW_LINE)
						{
							error = TokenError::InvalidBinary;
						}

						auto number_view = buffer.subview_bytes(start_offset, as<u32>(cursor - buffer) - start_offset);
						id               = detail::pool.add(number_view);
						co_yield Token{
						  .line   = line,
						  .column = start_column,
						  .offset = start_offset,
						  .length = codepoints,
						  .id     = id,
						  .type   = type,
						  .group  = TokenGroup::Literal,
						  .error  = error};
						continue;
					}
				}

				scan_digits(utf8::is_ascii_digit);

				// fractional part
				if (cursor.has_next() and *cursor == FULL_STOP and cursor.peek(1) != FULL_STOP)
				{
					type = TokenKind::FloatingPoint;
					++cursor;
					++codepoints;
					++column;
					scan_digits(utf8::is_ascii_digit);
				}

				// exponent part
				if (cursor.has_next() and (*cursor == LATIN_SMALL_LETTER_E or *cursor == LATIN_CAPITAL_LETTER_E))
				{
					type = TokenKind::FloatingPoint;
					++cursor;
					++codepoints;
					++column;
					if (cursor.has_next() and (*cursor == PLUS_SIGN or *cursor == HYPHEN_MINUS))
					{
						++cursor;
						++codepoints;
						++column;
					}

					const u32 exponent_digits = scan_digits(utf8::is_ascii_digit);
					if (exponent_digits == 0)
						error = TokenError::InvalidFloatingPoint;
				}

				// Check if literal terminates on a trailing digit separator
				if (cursor.peek_back() == LOW_LINE)
				{
					error = (type == TokenKind::Integer) ? TokenError::InvalidInteger : TokenError::InvalidFloatingPoint;
				}

				// suffix check
				if (cursor.has_next())
				{
					const char32 peek = *cursor;
					if (utf8::is_xid_start(peek) or (peek == FULL_STOP and cursor.peek(1) != FULL_STOP))
					{
						const u32 garbage = as<u32>(cursor.skip_while(
						  [](char32 cp) noexcept { return utf8::is_xid_continue(cp) or cp == FULL_STOP; }));
						codepoints += garbage;
						column += garbage;

						error = (type == TokenKind::Integer) ? TokenError::InvalidInteger : TokenError::InvalidFloatingPoint;
					}
				}

				auto number_view = buffer.subview_bytes(start_offset, as<u32>(cursor - buffer) - start_offset);
				id               = detail::pool.add(number_view);
				co_yield Token{
				  .line   = line,
				  .column = start_column,
				  .offset = start_offset,
				  .length = codepoints,
				  .id     = id,
				  .type   = type,
				  .group  = TokenGroup::Literal,
				  .error  = error};
				continue;
			}

			// Identifier / keyword
			if (utf8::is_xid_start(current))
			{
				error                  = TokenError::None;
				type                   = TokenKind::Identifier;
				const u32 start_column = column;
				const u32 start_offset = offset;

				if (config.forbid_non_ascii_in_identifiers and not utf8::is_ascii(current))
					error = TokenError::InvalidIdentifierNonASCII;

				const utf8::view ident_view = cursor.take_while(
				  [&](char32 cp) noexcept -> bool
				  {
					  if (not utf8::is_xid_continue(cp))
						  return false;

					  if (utf8::is_zero_width_codepoint(cp) and error == TokenError::None)
						  error = TokenError::InvalidIdentifierZeroWidthJoiner;

					  if (config.forbid_non_ascii_in_identifiers and not utf8::is_ascii(cp) and error == TokenError::None)
						  error = TokenError::InvalidIdentifierNonASCII;

					  return true;
				  });
				const u32 codepoints = as<u32>(ident_view.length());
				column += codepoints;

				id = detail::pool.add(ident_view);

				const bool is_keyword = false;
				if (is_keyword)
					type = TokenKind::Keyword;

				co_yield Token{
				  .line   = line,
				  .column = start_column,
				  .offset = start_offset,
				  .length = codepoints,
				  .id     = id,
				  .type   = type,
				  .group  = is_keyword ? TokenGroup::Keyword : TokenGroup::Identifier,
				  .error  = error};

				continue;
			}

			// Symbols
			switch (current)
			{
				using enum TokenKind;

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
					if (cursor.peek(1) == EQUALS_SIGN)
						co_yield give_token(StarEqual, 2);
					else if (cursor.peek(1) == SOLIDUS)
						co_yield give_token(StarSlash, 2);
					else
						co_yield give_token(Star, 1);
					continue;
				}

				case SOLIDUS:
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

				case REVERSE_SOLIDUS:
				{
					if (cursor.peek(1) == REVERSE_SOLIDUS)
						co_yield give_token(BackSlashBackSlash, 2);
					else
						co_yield give_token(BackSlash, 1);
					continue;
				}

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

				default:
				{
					co_yield give_token(TokenKind::Unknown, 1);
					continue;
				}
			}
		}

		co_yield Token{
		  .line   = line,
		  .column = column,
		  .offset = as<u32>(cursor - buffer),
		  .length = 0,
		  .id     = 0,
		  .type   = TokenKind::EOF,
		  .error  = TokenError::None};
		co_return;
	}

	export std::vector<Token> tokenize_all(utf8::view buffer, const TokenizeConfig config = {})
	{
		std::vector<Token> tokens;
		for (auto token : tokenize(buffer, config))
			tokens.push_back(token);
		return tokens;
	}

} // namespace deckard::lexer
