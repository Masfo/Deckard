export module deckard.utf8:characters;
import deckard.types;

export namespace deckard::utf8::characters
{
	constexpr char32 LINE_FEED       = U'\u000A';      // \n
	constexpr char32 CARRIAGE_RETURN = U'\u000D';      // \r

	constexpr char32 EQUALS_SIGN = U'\u003D';          // =
	constexpr char32 NUMBER_SIGN = U'\u0023';          // #
	constexpr char32 QUOTE_MARK  = U'\u0022';          // "
	constexpr char32 APOSTROPHE  = U'\u0027';          // '
	constexpr char32 COMMA       = U'\u002C';          // ,
	constexpr char32 FULL_STOP   = U'\u002E';          // .


	constexpr char32 LEFT_PARENTHESIS     = U'\u0028'; // (
	constexpr char32 RIGHT_PARENTHESIS    = U'\u0029'; // )
	constexpr char32 LEFT_SQUARE_BRACKET  = U'\u005B'; // [
	constexpr char32 RIGHT_SQUARE_BRACKET = U'\u005D'; // ]
	constexpr char32 LEFT_CURLY_BRACKET   = U'\u007B'; // {
	constexpr char32 RIGHT_CURLY_BRACKET  = U'\u007C'; // }

	constexpr char32 REVERSE_SOLIDUS = U'\u005C';      // -> \ <- escape char
	constexpr char32 SOLIDUS         = U'\u002F';      // -> / <-


} // namespace deckard::utf8::characters
