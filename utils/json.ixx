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

		template<typename T>
		requires(not std::same_as<std::remove_cvref_t<T>, value>)
		explicit value(T&& v)
			: m_storage{std::forward<T>(v)}
		{
		}

		friend auto operator==(const value&, const value&) -> bool = default;

		template<typename T>
		[[nodiscard]] auto holds() const noexcept -> bool
		{
			return std::holds_alternative<T>(m_storage);
		}

		[[nodiscard]] auto is_null() const noexcept -> bool { return holds<std::nullptr_t>(); }

		[[nodiscard]] auto is_bool() const noexcept -> bool { return holds<bool>(); }

		[[nodiscard]] auto is_number() const noexcept -> bool { return holds<f64>(); }

		[[nodiscard]] auto is_string() const noexcept -> bool { return holds<utf8::string>(); }

		[[nodiscard]] auto is_array() const noexcept -> bool { return holds<array>(); }

		[[nodiscard]] auto is_object() const noexcept -> bool { return holds<object>(); }

		template<typename T>
		[[nodiscard]] auto as() const noexcept -> std::optional<std::reference_wrapper<const T>>
		{
			if (auto* p = std::get_if<T>(&m_storage))
				return std::cref(*p);
			return std::nullopt;
		}

		template<typename T>
		[[nodiscard]] auto try_as() const noexcept -> const T*
		{
			return std::get_if<T>(&m_storage);
		}

		[[nodiscard]] auto as_bool() const -> bool { return as<bool>().value(); }

		[[nodiscard]] auto as_number() const noexcept -> f64
		{
			if (holds<f64>())
				return as<f64>().value();

			assert::check(false, "Value is not a number");
			std::unreachable();
		}

		[[nodiscard]] auto as_string() const -> const utf8::string& { return as<utf8::string>().value(); }

		[[nodiscard]] auto as_array() const -> const array& { return as<array>().value(); }

		[[nodiscard]] auto as_object() const -> const object& { return as<object>().value(); }

		[[nodiscard]] auto contains(std::string_view key) const -> bool { return as_object().contains(utf8::string{key}); }

		[[nodiscard]] auto at(std::string_view path) const -> std::expected<const value*, std::string>
		// TODO: expected &
		{
			const value* cur   = this;
			std::size_t  start = 0;

			while (true)
			{
				auto dot = path.find('.', start);
				auto key = path.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start);

				if (not cur->is_object())
					return std::unexpected(std::format("cannot index '{}': value is not an object", key));

				const auto& obj = cur->as_object();
				auto        it  = obj.find(utf8::string{key});
				if (it == obj.end())
					return std::unexpected(std::format("key not found: '{}'", key));

				cur = &it->second;

				if (dot == std::string_view::npos)
					break;
				start = dot + 1;
			}

			return cur;
		}

		[[nodiscard]] auto operator[](std::string_view key) const -> const value&
		{
			const auto& obj = as_object();
			auto        it  = obj.find(utf8::string{key});
			assert::check(it != obj.end(), std::format("no such key: {}", key));
			return it->second;
		}

		[[nodiscard]] auto operator[](utf8::string key) const -> const value&
		{
			const auto& obj = as_object();
			auto        it  = obj.find(key);

			assert::check(it != obj.end(), std::format("no such key: {}", key));

			return it->second;
		}

		[[nodiscard]] auto operator[](std::size_t i) const -> const value& { return as_array().at(i); }

		[[nodiscard]] auto size() const -> std::size_t
		{
			if (is_array())
				return as_array().size();

			if (is_object())
				return as_object().size();

			assert::check(false, "size() called on a value that is neither an array nor an object");
			std::unreachable();
		}

		template<typename Visitor>
		auto visit(Visitor&& vis) const
		{
			return std::visit(std::forward<Visitor>(vis), m_storage);
		}

		auto find(std::string_view key) const -> std::optional<const value*>
		{
			if (not is_object())
				return std::nullopt;

			const auto& obj = as_object();
			auto        it  = obj.find(utf8::string{key});
			if (it == obj.end())
				return std::nullopt;

			return &it->second;
		}

		friend auto operator==(const value& lhs, const value& rhs) -> bool = default;

		std::string to_string() const
		{
			return std::visit(
			  [](const auto& v) -> std::string
			  {
				  using T = std::decay_t<decltype(v)>;
				  if constexpr (std::is_same_v<T, std::nullptr_t>)
					  return "null";
				  else if constexpr (std::is_same_v<T, bool>)
					  return v ? "true" : "false";
				  else if constexpr (std::is_same_v<T, f64>)
					  return std::to_string(v);
				  else if constexpr (std::is_same_v<T, utf8::string>)
					  return std::format("\"{}\"", v);
				  else if constexpr (std::is_same_v<T, array>)
				  {
					  std::string result = "[";
					  for (size_t i = 0; i < v.size(); ++i)
					  {
						  result += v[i].to_string();
						  if (i < v.size() - 1)
							  result += ", ";
					  }
					  result += "]";
					  return result;
				  }
				  else if constexpr (std::is_same_v<T, object>)
				  {
					  std::string result = "{";
					  size_t      count  = 0;
					  for (const auto& [key, val] : v)
					  {
						  result += std::format("\"{}\": {}", key.to_string(), val.to_string());
						  if (++count < v.size())
							  result += ", ";
					  }
					  result += "}";
					  return result;
				  }
				  else
					  static_assert(false, "Non-exhaustive visitor!");
			  },
			  m_storage);
		}
	};

	std::ostream& operator<<(std::ostream& os, const value& val) { return os << val.to_string(); }

	export class parser
	{
	private:
		utf8::view    data;
		utf8::scanner scanner;

		[[nodiscard]] auto error(std::string_view message) -> std::unexpected<std::string>
		{
			auto [line, column] = scanner.position();
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
			if (not scanner.skip_if(U'{'))
				return error("Expected '{' at the beginning of object");

			object obj;

			scanner.skip_whitespace();
			if (scanner.skip_if(U'}'))
				return value{std::move(obj)}; // Empty object

			while (true)
			{
				scanner.skip_whitespace();

				auto key = parse_string();
				if (not key)
					return std::unexpected(key.error());

				scanner.skip_whitespace();
				if (not scanner.skip_if(U':'))
					return error("Expected ':' after key in object");

				auto val = parse_value();
				if (not val)
					return std::unexpected(val.error());

				obj.emplace((*key).as_string(), std::move(*val));

				scanner.skip_whitespace();
				if (scanner.current() == U'}')
				{
					scanner.skip();
					break;
				}
				else if (scanner.skip_if(U','))
				{
					scanner.skip_whitespace();
					if (scanner.current() == U'}')
						return error("Trailing comma in object is not allowed");
				}
				else
				{
					return error("Expected ',' or '}' after value in object");
				}
			}

			return value{std::move(obj)};
		}

		auto parse_array() -> std::expected<value, std::string>
		{
			if (not scanner.skip_if(U'['))
				return error("Expected '[' at the beginning of array");

			array arr;

			scanner.skip_whitespace();
			if (scanner.skip_if(U']'))
				return value{std::move(arr)}; // Empty array

			while (true)
			{
				auto elem = parse_value();
				if (not elem)
					return std::unexpected(elem.error());

				arr.push_back(std::move(*elem));

				scanner.skip_whitespace();
				if (scanner.current() == U']')
				{
					scanner.skip();
					break;
				}
				else if (scanner.skip_if(U','))
				{
					scanner.skip_whitespace();
					if (scanner.current() == U']')
						return error("Trailing comma in array is not allowed");
				}
				else
				{
					return error("Expected ',' or ']' after value in array");
				}
			}

			return value{std::move(arr)};
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

export template<>
struct std::hash<deckard::json::value>
{
	[[nodiscard]] auto operator()(const deckard::json::value& v) const noexcept -> std::size_t
	{
		using namespace deckard::utils;

		return v.visit(
		  []<typename T>(const T& x) -> std::size_t
		  {
			  if constexpr (std::same_as<T, std::nullptr_t>)
			  {
				  return std::size_t{0x9E37'79B9}; // fixed tag: all nulls hash equal
			  }
			  else if constexpr (std::same_as<T, bool>)
			  {
				  return hash_combine(0x1, std::hash<bool>{}(x));
			  }
			  else if constexpr (std::same_as<T, i64>)
			  {
				  return hash_combine(0x2A, std::hash<i64>{}(x));
			  }
			  else if constexpr (std::same_as<T, u64>)
			  {
				  return hash_combine(0x2B, std::hash<u64>{}(x));
			  }
			  else if constexpr (std::same_as<T, f64>)
			  {
				  return hash_combine(0x2, std::hash<f64>{}(x));
			  }
			  else if constexpr (std::same_as<T, utf8::string>)
			  {
				  return hash_combine(0x3, std::hash<utf8::string>{}(x));
			  }
			  else if constexpr (std::same_as<T, deckard::json::array>)
			  {
				  std::size_t seed = 0x4;
				  for (const auto& elem : x)
					  seed = hash_combine(seed, std::hash<deckard::json::value>{}(elem));
				  return seed;
			  }
			  else
			  {
				  std::size_t seed = 0x5;
				  for (const auto& [key, val] : x)
				  {
					  auto pair_hash = hash_combine(std::hash<utf8::string>{}(key), std::hash<deckard::json::value>{}(val));
					  seed ^= pair_hash;
				  }
				  return seed;
			  }
		  });
	}
};
