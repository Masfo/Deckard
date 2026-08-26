export module deckard.json;

import std;
import deckard.assert;
import deckard.types;
import deckard.utf8;
import deckard.utils.hash;
import deckard.debug;
import deckard.helpers;

namespace deckard::json
{

	export class value;

	export using array  = std::vector<value>;
	export using object = std::unordered_map<utf8::string, value>;
	using Storage       = std::variant<std::nullptr_t, bool, f64, utf8::string, array, object>;

	export class value
	{
	private:
		Storage m_storage;

	public:
		value() noexcept
			: m_storage{nullptr}
		{
		}

	


	};

	export class parser
	{
	private:
		utf8::view    data;
		utf8::scanner scanner;

		[[nodiscard]] auto line_col(u64 offset) -> std::pair<u64, u64>
		{
			u64 line   = 1;
			u64 column = 1;
			u64 index  = 0;

			for (char32 cp : data)
			{
				if (index >= offset)
					break;

				if (cp == U'\n')
				{
					++line;
					column = 1;
				}
				else
				{
					++column;
				}

				index += utf8::codepoint_width(cp);
			}

			return {line, column};
		}


		[[nodiscard]] auto error(std::string_view message) -> std::unexpected<std::string>
		{
			auto [line, column] = line_col(scanner.position());
			return std::unexpected(std::format("Error at line {}, column {}: {}", line, column, message));
		}

	public:
		parser() = default;

		friend auto operator==(const parser& lhs, const parser& rhs) noexcept -> bool { return lhs.data == rhs.data; }

		parser(utf8::view v)
			: data(v)
			, scanner(v)
		{
		}

		auto parse(utf8::view v) -> std::expected<value, std::string>
		{
			scanner    = utf8::scanner(v);
			auto value = parse_value();
			if (not value)
				return std::unexpected(value.error());

			scanner.skip_whitespace();

			if (scanner.has_next())
				return std::unexpected("Unexpected characters after JSON value");

			return value;
		}

		auto parse_value() -> std::expected<value, std::string>
		{
			scanner.skip_whitespace();

			switch (scanner.try_current().value_or(U'\0'))
			{
				case U'{': return parse_object();
				case U'[': return parse_array();
				case U'"': return parse_string();
				case U't': [[fallthrough]];
				case U'f': return parse_bool();
				case U'n': return parse_null();
				case '\0': return error("Unexpected end of input");
				default: // parse number
					if (utf8::is_ascii_digit(scanner.try_current().value_or(U'\0')) or scanner.try_current() == U'-')
						return parse_number();
					else
						return error("Invalid JSON value");
			}
		}

		auto parse_object() -> std::expected<value, std::string>
		{

			return value{};
		}

		auto parse_array() -> std::expected<value, std::string>
		{
	
			return value{};
		}

		auto parse_string() -> std::expected<value, std::string>
		{
			if (not scanner.skip_if(U'"'))
				return error("Expected '\"' at the beginning of string");

			utf8::string result;

			while (scanner.has_next())
			{

				if (scanner.is(U'"'))
				{
					scanner.skip(); // closing quote
					return value{std::move(result)};
				}

				if (scanner.is(U'\\'))
				{
					scanner.skip();
					if (not scanner.has_next())
						return error("Unexpected end of input in string escape sequence");

					char32_t escaped{};
					switch (scanner.current())
					{
						case U'"': escaped = U'"'; break;
						case U'\\': escaped = U'\\'; break;
						case U'/': escaped = U'/'; break;
						case U'b': escaped = U'\b'; break;
						case U'f': escaped = U'\f'; break;
						case U'n': escaped = U'\n'; break;
						case U'r': escaped = U'\r'; break;
						case U't': escaped = U'\t'; break;
						case U'u':
							scanner.skip(); // consume 'u'
							// TODO: read 4 hex digits (and low surrogate for pairs outside the
							// BMP), decode to a code point, append via result += codepoint.
							return error("\\u escapes not yet implemented");
						default: return error("Invalid escape sequence in string");
					}

					result += escaped;
					scanner.skip();
					continue;
				}

				auto chunk = scanner.take_until(
				  [](char32_t cp) { return cp == U'"' or cp == U'\\' or utf8::is_control_character(cp); });

				if (chunk.empty())
				{
					auto cp = scanner.current();
					if (utf8::is_control_character(cp))
						return error("Control character not allowed in string");

					result += cp;
					scanner.skip();
					continue;
				}
	
				result += chunk;
			}
			return error("Unexpected end of input in string");
		}

		auto parse_number() -> std::expected<value, std::string>
		{

			scanner.skip_whitespace();

			if (scanner.is(U'-'))
			{
				if (not scanner.has_next() or not utf8::is_ascii_digit(scanner.peek().value_or(U'\0')))
					return error("Expected digit after '-' in number");
			}


			auto text
			  = scanner
				  .take_until(
					[](char32_t cp)
					{
						return not(
						  utf8::is_ascii_digit(cp) or cp == U'.' or cp == U'e' or cp == U'E' or cp == U'+' or cp == U'-');
					})
				  .as_string_view();


			auto val = try_to_number<f64>(text);
			if (val)
				return value{*val};

			return error(std::format("Invalid number: {}", text));
		}

		auto parse_bool() -> std::expected<value, std::string>
		{
			scanner.skip_whitespace();
			if (scanner.has_next() and scanner.peek(0) == U't')
			{
				if (not scanner.expect("true"))
					return error("Invalid literal, expected 'true'");
				return value{true};
			}
			else if (scanner.has_next() and scanner.peek(0) == U'f')
			{
				if (not scanner.expect("false"))
					return error("Invalid literal, expected 'false'");
				return value{false};
			}

			return error("Expected 'true' or 'false'");
		}

		auto parse_null() -> std::expected<value, std::string>
		{
			for (char c : std::string_view{"null"})
			{
				if (not scanner.skip_if(static_cast<char32_t>(c)))
					return error("Invalid literal, expected 'null'");
			}
			return value{nullptr};
		}
	};


} // namespace deckard::json

