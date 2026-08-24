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
	
			return {};
		}

		auto parse_number() -> std::expected<value, std::string>
		{


			return {};
		}

		auto parse_bool() -> std::expected<value, std::string>
		{


			return {};
		}

		auto parse_null() -> std::expected<value, std::string>
		{

			return {};
		}
	};


} // namespace deckard::json

