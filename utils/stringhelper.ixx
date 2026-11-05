export module deckard.stringhelper;

import std;
import deckard.helpers;
import deckard.types;
import deckard.enums;
import deckard.debug;
import deckard.math;
import deckard.assert;

export namespace deckard::string
{
	constexpr std::string_view alphanum_string{"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};

	constexpr std::string_view whitespace_string{" \t\f\n\r\v"};

	constexpr std::string_view digit_string{alphanum_string.substr(0, 10)};
	constexpr std::string_view lowercase_alphabet{alphanum_string.substr(digit_string.size(), 26)};
	constexpr std::string_view uppercase_alphabet{
	  alphanum_string.substr(digit_string.size() + lowercase_alphabet.size(), 26)};
	constexpr std::string_view alphabet{alphanum_string.substr(digit_string.size(), 52)};

	static_assert(whitespace_string[0] == ' ');
	static_assert(whitespace_string[5] == '\v');
	static_assert(digit_string[0] == '0');
	static_assert(digit_string[9] == '9');
	static_assert(lowercase_alphabet[0] == 'a');
	static_assert(lowercase_alphabet[25] == 'z');
	static_assert(uppercase_alphabet[0] == 'A');
	static_assert(uppercase_alphabet[25] == 'Z');
	static_assert(alphabet[0] == 'a');
	static_assert(alphabet[51] == 'Z');


	export enum class option : u8 {
		digit      = BIT(0),
		whitespace = BIT(1),
		lowercase  = BIT(2),
		uppercase  = BIT(3),
		special    = BIT(4),

		reserved1 = BIT(5),
		reserved2 = BIT(6),
		reserved3 = BIT(7),

		alphabet = lowercase | uppercase,
		alphanum = alphabet | digit,
		all      = whitespace | alphanum | special,

		d  = digit,
		w  = whitespace,
		l  = lowercase,
		u  = uppercase,
		a  = l | u,
		an = a | d,
		s  = special,

		Count = 8

	};

	consteval void enable_bitmask_operations(option);

	template<typename T>
	struct vector_string_traits;

	template<typename T>
	requires std::is_arithmetic_v<T>
	struct vector_string_traits<vec::generic_vec2<T>>
	{
		using component_type                    = T;
		static constexpr size_t component_count = 2;
	};

	template<typename T>
	requires std::is_arithmetic_v<T>
	struct vector_string_traits<vec::generic_vec3<T>>
	{
		using component_type                    = T;
		static constexpr size_t component_count = 3;
	};

	template<typename T>
	requires std::is_arithmetic_v<T>
	struct vector_string_traits<vec::generic_vec4<T>>
	{
		using component_type                    = T;
		static constexpr size_t component_count = 4;
	};

	template<typename T>
	concept vector_convertible = requires {
		typename vector_string_traits<T>::component_type;
		vector_string_traits<T>::component_count;
	};

	template<typename T>
	concept split_conversion_target
	  = std::is_arithmetic_v<T> or std::constructible_from<T, std::string_view> or vector_convertible<T>;

	template<typename T>
	requires split_conversion_target<T>
	auto convert_to_type(std::string_view input, [[maybe_unused]] std::string_view delims = " ") -> T
	{
		if constexpr (std::same_as<T, char>)
		{
			assert::check(input.size() == 1, "Expected one character");
			return input.front();
		}
		else if constexpr (std::is_arithmetic_v<T>)
			return to_number<T>(input);
		else if constexpr (vector_convertible<T>)
		{
			using Component                                               = typename vector_string_traits<T>::component_type;
			constexpr size_t                              component_count = vector_string_traits<T>::component_count;
			std::array<std::string_view, component_count> components{};
			size_t                                        first = 0;

			for (size_t i = 0; i < component_count; ++i)
			{
				const auto separator = input.find(',', first);
				if (i + 1 < component_count)
				{
					assert::check(separator != std::string_view::npos, "Missing vector component");
					components[i] = input.substr(first, separator - first);
					first         = separator + 1;
				}
				else
				{
					assert::check(separator == std::string_view::npos, "Too many vector components");
					components[i] = input.substr(first);
				}
			}

			return [&]<size_t... I>(std::index_sequence<I...>) -> T
			{ return T{to_number<Component>(components[I])...}; }(std::make_index_sequence<component_count>{});
		}
		else
			return T(input);
	}

	// ###########################################################################
	template<typename Matrix>
	auto transpose(
	  const Matrix& matrix,
	  std::ranges::range_value_t<std::ranges::range_value_t<Matrix>> pad = {})
	  -> std::vector<std::vector<std::ranges::range_value_t<std::ranges::range_value_t<Matrix>>>>
	{
		if (matrix.empty())
			return {};

		

		using Row = std::ranges::range_value_t<Matrix>;
		using T   = std::ranges::range_value_t<Row>;

		const usize rows = matrix.size();
		const usize cols = std::ranges::max(matrix | std::views::transform([](const auto& row) { return row.size(); }));
		std::vector<std::vector<T>> transposed(cols, std::vector<T>(rows, pad));
		for (usize i = 0; i < rows; ++i)
		{
			for (usize j = 0; j < matrix[i].size(); ++j)
				transposed[j][i] = matrix[i][j];
		}
		return transposed;
	}

	template<typename Matrix>
	[[nodiscard]] auto rotate_cw(
	  const Matrix& in,
	  std::ranges::range_value_t<std::ranges::range_value_t<Matrix>> pad = {})
	  -> std::vector<std::vector<std::ranges::range_value_t<std::ranges::range_value_t<Matrix>>>>
	{
		auto out = transpose(in, pad);
		for (auto& row : out)
			std::ranges::reverse(row);
		return out;
	}

	template<typename Matrix>
	[[nodiscard]] auto rotate_ccw(
	  const Matrix& in,
	  std::ranges::range_value_t<std::ranges::range_value_t<Matrix>> pad = {})
	  -> std::vector<std::vector<std::ranges::range_value_t<std::ranges::range_value_t<Matrix>>>>
	{
		auto out = transpose(in, pad);

		std::ranges::reverse(out);
		return out;
	}

	//

	// count
	i64 count(auto str, std::string_view interests) noexcept
	{
		return std::ranges::count_if(str, [&interests](char c) { return interests.contains(c); });
	}

	i64 count_exact(std::string_view str, std::string_view value) noexcept
	{
		if (value.empty())
			return 0;

		i64    count = 0;
		size_t pos   = 0;
		while ((pos = str.find(value, pos)) != std::string_view::npos)
		{
			++count;
			pos += value.size();
		}
		return count;
	}

	std::unordered_map<char, i64> histogram(std::string_view str) noexcept
	{
		std::unordered_map<char, i64> counts;
		for (const char& c : str)
			counts[c] += 1;
		return counts;
	}

	// strip - single
	std::string strip(std::string_view str, char a)
	{
		std::string ret;
		ret.reserve(str.size());
		for (const char& c : str)
			if (c != a)
				ret += c;

		return ret;
	}

	// strip - range
	std::string strip(std::string_view str, char a, char z)
	{
		std::string ret;
		ret.reserve(str.size());
		for (const char& c : str)
			if (c < a || c > z)
				ret += c;

		return ret;
	}

	// strip -
	std::string strip(std::string_view str, std::string_view strip_chars)
	{
		std::string ret;
		ret.reserve(str.size());

		for (auto& c : str)
			if (!strip_chars.contains(c))
				ret += c;

		return ret;
	}

	// strip - option
	// input = strip(input, w|u|d);
	std::string strip(std::string_view str, option opt = option::whitespace)
	{
		using enum option;
		std::string ret{str};


		if (std::to_underlying(opt) == 0)
			return ret;


		if (has(opt, whitespace))
		{
			ret = strip(ret, '\t', '\r');
			ret = strip(ret, ' ');
			ret = strip(ret, '\n');
		}

		if (has(opt, digit))
			ret = strip(ret, '0', '9');

		if (has(opt, lowercase))
			ret = strip(ret, 'a', 'z');

		if (has(opt, uppercase))
			ret = strip(ret, 'A', 'Z');

		if (has(opt, special))
		{
			ret = strip(ret, '!', '/');
			ret = strip(ret, ':', '@');
			ret = strip(ret, '[', '`');
			ret = strip(ret, '{', '~');
		}

		return ret;
	}

	// include_only
	std::string include_only(std::string_view input, option opt = option::all, std::string_view extra = "")
	{
		using enum option;

		std::string ret;
		ret.reserve(input.size());
		for (const char& c : input)
		{
			bool selected = false;
			if (has(opt, whitespace) and (in_range('\t', '\r', c) or c == ' '))
				selected = true;
			if (has(opt, digit) and in_range('0', '9', c))
				selected = true;
			if (has(opt, lowercase) and in_range('a', 'z', c))
				selected = true;
			if (has(opt, uppercase) and in_range('A', 'Z', c))
				selected = true;
			if (has(opt, special)
				and (in_range('!', '/', c) or in_range(':', '@', c) or in_range('[', '`', c) or in_range('{', '~', c)))
				selected = true;
			if (extra.contains(c))
				selected = true;

			if (selected)
				ret += c;
		}
		return ret;
	}

	// exclude_only
	std::string exclude_only(std::string_view input, option opt = option::all, std::string_view extra = "")
	{
		using enum option;

		std::string ret;
		ret.reserve(input.size());
		for (const char& c : input)
		{
			bool selected = false;
			if (has(opt, whitespace) and (in_range('\t', '\r', c) or c == ' '))
				selected = true;
			if (has(opt, digit) and in_range('0', '9', c))
				selected = true;
			if (has(opt, lowercase) and in_range('a', 'z', c))
				selected = true;
			if (has(opt, uppercase) and in_range('A', 'Z', c))
				selected = true;
			if (has(opt, special)
				and (in_range('!', '/', c) or in_range(':', '@', c) or in_range('[', '`', c) or in_range('{', '~', c)))
				selected = true;
			if (extra.contains(c))
				selected = true;

			if (not selected)
				ret += c;
		}
		return ret;
	}

	// replace
	std::string replace(std::string_view subject, const std::string_view search, std::string_view replace)
	{
		std::string output(subject);
		size_t      pos = 0;
		while ((pos = output.find(search, pos)) != std::string::npos)
		{
			output.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return output;
	}

	// split
	template<typename T = std::string_view>
	std::vector<T> split(std::string_view input, std::string_view delims = " ")
	{
		std::vector<T> output;
		size_t         first = 0;

		while (first < input.size())
		{
			const auto second = input.find_first_of(delims, first);

			if (first != second)
				output.emplace_back(convert_to_type<T>(input.substr(first, second - first)));

			if (second == std::string_view::npos)
				break;

			first = second + 1;
		}

		return output;
	}


	// auto [a,b] = split<std::string, std::string>(input, delims);
	template<typename... Ts>
	requires(sizeof...(Ts) > 1 and (split_conversion_target<Ts> and ...))
	auto split(std::string_view input, std::string_view delims = " ") -> std::tuple<Ts...>
	{
		const auto parts = split(input, delims);
		assert::check(
		  parts.size() == sizeof...(Ts), std::format("Expected {} parts, found {}", sizeof...(Ts), parts.size()));

		return [&]<size_t... I>(std::index_sequence<I...>) -> std::tuple<Ts...>
		{ return {convert_to_type<Ts>(parts[I])...}; }(std::index_sequence_for<Ts...>{});
	}

	// split_once
	auto split_once(std::string_view str, std::string_view delims = " ")
	{
		using namespace std::string_literals;
		size_t pos = str.find(delims);
		if (pos != std::string::npos)
		{
			return make_array(str.substr(0, pos), str.substr(pos + delims.size()));
		}
		else
		{
			return make_array(str, ""s);
		}
	}

	template<typename T1, typename T2>
	requires(split_conversion_target<T1> and split_conversion_target<T2>)
	auto split_once(std::string_view str, std::string_view delims = " ") -> std::tuple<T1, T2>
	{
		const auto parts = split_once(str, delims);
		return {convert_to_type<T1>(parts[0]), convert_to_type<T2>(parts[1])};
	}

	// split_exact
	template<string_like_container T>
	std::vector<T> split_exact(T str, std::string_view delims, bool include_empty = false)
	{
		std::vector<T> output;
		u64            first = 0;
		while (first < str.size())
		{
			const auto second = str.find(delims, first);

			if (include_empty || first != second) // empty line
				output.emplace_back(str.substr(first, second - first));

			if (second == T::npos)
				break;
			first = second + delims.size();
		}
		return output;
	}

	template<typename... Ts>
	requires(sizeof...(Ts) > 0 and (split_conversion_target<Ts> and ...))
	auto split_exact(std::string_view str, std::string_view delims, bool include_empty = false) -> std::tuple<Ts...>
	{
		std::vector<std::string_view> parts;
		size_t                        first = 0;
		while (first < str.size())
		{
			const auto second = str.find(delims, first);

			if (include_empty or first != second)
				parts.emplace_back(str.substr(first, second - first));

			if (second == std::string_view::npos)
				break;
			first = second + delims.size();
		}

		assert::check(
		  parts.size() == sizeof...(Ts), std::format("Expected {} parts, found {}", sizeof...(Ts), parts.size()));

		return [&]<size_t... I>(std::index_sequence<I...>) -> std::tuple<Ts...>
		{ return {convert_to_type<Ts>(parts[I])...}; }(std::index_sequence_for<Ts...>{});
	}

	// split_exact
	std::vector<std::string_view> split_exact(std::string_view str, u64 length)
	{
		std::vector<std::string_view> v;
		v.reserve(2);
		if (length <= str.length())
		{
			v.emplace_back(str.substr(0, length));
			v.emplace_back(str.substr(length));
		}
		return v;
	}

	template<typename... Ts>
	requires(sizeof...(Ts) > 0 and (split_conversion_target<Ts> and ...))
	auto split_exact(std::string_view str, u64 length) -> std::tuple<Ts...>
	{
		const auto parts = split_exact(str, length);
		assert::check(
		  parts.size() == sizeof...(Ts), std::format("Expected {} parts, found {}", sizeof...(Ts), parts.size()));

		return [&]<size_t... I>(std::index_sequence<I...>) -> std::tuple<Ts...>
		{ return {convert_to_type<Ts>(parts[I])...}; }(std::index_sequence_for<Ts...>{});
	}

	// split_stride
	std::vector<std::string_view> split_stride(std::string_view str, u64 stride)
	{
		std::vector<std::string_view> v;

		for (u64 i = 0; i <= str.length(); i += stride)
		{
			if (i < str.length())
				v.emplace_back(str.substr(i, stride));
		}
		return v;
	}

	template<typename T>
	requires split_conversion_target<T>
	auto split_stride(std::string_view str, u64 stride) -> std::vector<T>
	{
		const auto     parts = split_stride(str, stride);
		std::vector<T> result;
		result.reserve(parts.size());

		for (const auto part : parts)
			result.emplace_back(convert_to_type<T>(part));

		return result;
	}

	// split_to_vector
	template<typename T>
	auto split_to_vector(std::string_view input, std::string_view delims = " ")
	{
		auto splitted = split(input, delims);

		std::vector<T> ret;
		ret.reserve(splitted.size());

		for (const auto& r : splitted)
			ret.push_back(convert_to_type<T>(r));

		return ret;
	}

	template<typename T, size_t COUNT>
	auto split_to_vector(std::string_view input, std::string_view delims = " ")
	{
		auto splitted = split_to_vector<T>(input, delims);

		splitted.resize(COUNT);

		return splitted;
	}

	// trim
	std::string_view trim_front(std::string_view s)
	{
		if (s.empty())
			return s;

		s.remove_prefix(s.find_first_not_of(whitespace_string));
		return s;
	}

	std::string_view trim_back(std::string_view s)
	{
		if (s.empty())
			return s;

		s.remove_suffix(s.size() - s.find_last_not_of(whitespace_string) - 1);
		return s;
	}

	std::string_view trim(std::string_view s)
	{
		s = trim_front(s);
		return trim_back(s);
	};

	std::string_view to_string_view(std::span<const u8> input)
	{
		return std::string_view(reinterpret_cast<const char*>(input.data()), input.size());
	}

	// match
	bool match(std::string_view pattern, std::string_view input)
	{
		if (pattern.empty() and input.empty())
			return true;
		else if (pattern.starts_with('*') and pattern.size() > 1 and input.empty())
			return false;
		else if (pattern.starts_with('?') or (not pattern.empty() and not input.empty() and pattern[0] == input[0]))
			return match(pattern.substr(1), input.substr(1));
		else if (pattern.starts_with('*'))
			return match(pattern.substr(1), input) or match(pattern, input.substr(1));

		return false;
	}

	// ints
	template<size_t N, std::integral T = i64>
	requires(N > 1)
	constexpr auto ints(const std::string_view str) -> std::array<T, N>
	{
		std::array<T, N> ret{};

		T    acc   = 0;
		bool added = false;
		bool neg   = false;

		size_t index = 0;
		for (const auto& c : str)
		{

			if (index >= N)
				break;

			if (c == '-' and neg == false)
			{
				neg = true;
				continue;
			}

			if (in_range(0, 9, c - '0'))
			{
				acc *= 10;
				acc += c - '0';
				added = true;
				continue;
			}

			if (added and index < N)
			{
				if constexpr (std::is_unsigned_v<T>)
					ret[index++] = acc;
				else
					ret[index++] = neg ? -acc : acc;

				added = false;
				neg   = false;
				acc   = 0;
			}
		}


		if (added and index < N)
		{
			if constexpr (std::is_unsigned_v<T>)
				ret[index++] = acc;
			else
				ret[index++] = neg ? -acc : acc;

			added = false;
			neg   = false;
			acc   = 0;
		}

		assert::check(not added, std::format("\"{}\" does not contain a number", str));
		return ret;
	}

	template<size_t N, std::integral T = i64>
	requires(N == 1)
	constexpr auto ints(const std::string_view str) -> T
	{
		T    acc   = 0;
		bool added = false;
		bool neg   = false;

		for (const auto& c : str)
		{

			if (c == '-' and neg == false)
			{
				neg = true;
				continue;
			}

			if (in_range(0, 9, c - '0'))
			{
				acc *= 10;
				acc += c - '0';
				added = true;
				continue;
			}


			if (added)
			{
				if constexpr (std::is_unsigned_v<T>)
					return acc;
				else
					return neg ? -acc : acc;
			}
		}

		if (added)
		{
			if constexpr (std::is_unsigned_v<T>)
				return acc;
			else
				return neg ? -acc : acc;
		}
		dbg::panic(std::format("\"{}\" does not contain a number", str));
	}

	template<std::integral T = i64>
	constexpr auto ints(const std::string_view str) -> std::vector<T>
	{
		std::vector<T> ret{};

		T    acc   = 0;
		bool added = false;
		bool neg   = false;
		for (const auto& c : str)
		{
			if (c == '-' and neg == false)
			{
				neg = true;
				continue;
			}
			if (in_range(0, 9, c - '0'))
			{
				acc *= 10;
				acc += c - '0';
				added = true;
				continue;
			}

			if (added)
			{

				if constexpr (std::is_unsigned_v<T>)
					ret.emplace_back(acc);
				else
					ret.emplace_back(neg ? -acc : acc);
				acc   = 0;
				added = false;
				neg   = false;
			}
		}
		if (added)
		{

			if constexpr (std::is_unsigned_v<T>)
				ret.emplace_back(acc);
			else
				ret.emplace_back(neg ? -acc : acc);
			acc   = 0;
			added = false;
			neg   = false;
		}


		return ret;
	}

	// ints - one type per number: ints<i32, i64>("123 456")
	template<std::integral... Ts>
	requires(sizeof...(Ts) > 1)
	constexpr auto ints(const std::string_view str) -> std::tuple<Ts...>
	{
		std::array<std::pair<u64, bool>, sizeof...(Ts)> values{};

		u64    acc   = 0;
		bool   added = false;
		bool   neg   = false;
		size_t index = 0;

		auto flush = [&]
		{
			values[index++] = {acc, neg};
			acc             = 0;
			added           = false;
			neg             = false;
		};

		for (const auto& c : str)
		{
			if (index >= values.size())
				break;

			if (c == '-' and neg == false)
			{
				neg = true;
				continue;
			}

			if (in_range(0, 9, c - '0'))
			{
				acc *= 10;
				acc += c - '0';
				added = true;
				continue;
			}

			if (added and index < values.size())
				flush();
		}

		if (added and index < values.size())
			flush();

		auto convert = []<typename T>(std::pair<u64, bool> value) -> T
		{
			if constexpr (std::is_unsigned_v<T>)
				return static_cast<T>(value.first);
			else
				return value.second ? -static_cast<T>(value.first) : static_cast<T>(value.first);
		};

		return [&]<size_t... I>(std::index_sequence<I...>) -> std::tuple<Ts...>
		{ return {convert.template operator()<Ts>(values[I])...}; }(std::index_sequence_for<Ts...>{});
	}

	export auto simple_pattern_match(std::string_view expression, std::string_view input) -> std::vector<std::string_view>
	{
		std::vector<std::string_view> ret;

		size_t expression_index = 0;
		size_t input_index      = 0;

		while (expression_index < expression.size() and input_index < input.size())
		{

			if (expression[expression_index] == input[input_index])
			{
				expression_index += 1;
				input_index += 1;
				continue;
			}

			if (expression[expression_index] == '?')
			{
				expression_index += 1;

				char read_until = expression_index < expression.size() ? expression[expression_index] : ' ';

				size_t start = input_index;

				while (input_index < input.size() and input[input_index] != read_until)
					input_index += 1;


				std::string_view token = input.substr(start, input_index - start);
				ret.emplace_back(token);
			}
		}
		return ret;
	}

	export template<typename... Ts>
	requires(sizeof...(Ts) > 0)
	auto simple_pattern_match(std::string_view expr, std::string_view input) -> std::tuple<Ts...>
	{
		auto result = simple_pattern_match(expr, input);

		assert::check(result.size() >= sizeof...(Ts),
					  std::format("Expected pattern to match at least {}, found {}", sizeof...(Ts), result.size()));

		auto convert = []<typename T>(std::string_view value) -> T
		{
			if constexpr (std::is_integral_v<T>)
				return to_number<T>(value);
			else if constexpr (std::is_convertible_v<T, std::string>)
				return T(value);
			else
				return T{};
		};

		return [&]<std::size_t... I>(std::index_sequence<I...>) -> std::tuple<Ts...>
		{ return {convert.template operator()<Ts>(result[I])...}; }(std::index_sequence_for<Ts...>{});
	}

} // namespace deckard::string

#ifdef __cpp_lib_format_path
#error ("std::formatter<std::filesystem::path> is already provided by the standard library");
#else
template<>
struct std::formatter<std::filesystem::path> : std::formatter<std::string_view>
{
	auto format(const std::filesystem::path& p, std::format_context& ctx) const
	{
		return std::formatter<std::string_view>::format(p.string(), ctx);
	}
};
#endif


template<deckard::not_builtin T>
struct std::formatter<std::optional<T>>
{
	std::formatter<T> underlying;

	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	auto format(const std::optional<T>& opt, std::format_context& ctx) const
	{
		if (opt)
			return underlying.format(*opt, ctx);

		return std::format_to(ctx.out(), "nullopt");
	}
};

template<deckard::not_builtin T, typename E>
struct std::formatter<std::expected<T, E>>
{
	std::formatter<T> underlying;

	constexpr auto parse(std::format_parse_context& ctx) { return underlying.parse(ctx); }

	auto format(const std::expected<T, E>& exp, std::format_context& ctx) const
	{
		if (exp.has_value())
		{
			if constexpr (std::is_void_v<T>)
				return std::format_to(ctx.out(), "void");
			else
				return underlying.format(*exp, ctx);
		}
		return std::format_to(ctx.out(), "{}", exp.error());
	}
};
