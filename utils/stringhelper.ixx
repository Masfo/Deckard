export module deckard.stringhelper;

import std;
import deckard.helpers;
import deckard.types;
import deckard.enums;
import deckard.debug;
import deckard.math;

export namespace deckard::string
{
	constexpr std::string_view alphanum_string{"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};

	constexpr std::string_view whitespace_string{" \t\f\n\r\v"};

	constexpr std::string_view digit_string{alphanum_string.substr(0, 10)};
	constexpr std::string_view lowercase_string{alphanum_string.substr(digit_string.size(), 26)};
	constexpr std::string_view uppercase_string{alphanum_string.substr(digit_string.size() + lowercase_string.size(), 26)};
	constexpr std::string_view alphabet_string{alphanum_string.substr(digit_string.size(), 52)};

	static_assert(whitespace_string[0] == ' ');
	static_assert(whitespace_string[5] == '\v');
	static_assert(digit_string[0] == '0');
	static_assert(digit_string[9] == '9');
	static_assert(lowercase_string[0] == 'a');
	static_assert(lowercase_string[25] == 'z');
	static_assert(uppercase_string[0] == 'A');
	static_assert(uppercase_string[25] == 'Z');
	static_assert(alphabet_string[0] == 'a');
	static_assert(alphabet_string[51] == 'Z');


	export enum class strip_option : u8 {
		digit      = BIT(0),
		whitespace = BIT(1),
		lowercase  = BIT(2),
		uppercase  = BIT(3),
		special    = BIT(4),

		reserved  = BIT(5),
		reserved2 = BIT(6),
		reserved3 = BIT(7),

		alphabet = lowercase | uppercase,
		alphanum = alphabet | digit,

		d  = digit,
		w  = whitespace,
		l  = lowercase,
		u  = uppercase,
		a  = l | u,
		an = a | d,
		s  = special,

	};

	consteval void enable_bitmask_operations(strip_option);

	template<typename T>
	auto convert_to_type(std::string_view input, [[maybe_unused]] std::string_view delims = " ") -> T
	{

		if constexpr (std::is_same_v<T, char>)
		{
			return input[0];
		}

		if constexpr (std::is_convertible_v<T, std::string_view>)
		{
			return T(input);
		}

		if constexpr (std::is_arithmetic_v<T>)
		{
			return to_number<T>(input);
		}


		dbg::panic("conversion not handled");
	}

	// ###########################################################################


	// count
	i64 count(auto str, std::string_view interests) noexcept
	{
		return std::ranges::count_if(str, [&interests](char c) { return interests.contains(c); });
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
	std::string strip(std::string_view str, strip_option option = strip_option::whitespace)
	{
		using enum strip_option;
		std::string ret{str};


		if (std::to_underlying(option) == 0)
			return ret;


		if (option && whitespace)
		{
			ret = strip(ret, '\t', '\r');
			ret = strip(ret, ' ');
		}

		if (option && digit)
			ret = strip(ret, '0', '9');

		if (option && lowercase)
			ret = strip(ret, 'a', 'z');

		if (option && uppercase)
			ret = strip(ret, 'A', 'Z');

		if (option && special)
		{
			ret = strip(ret, '!', '/');
			ret = strip(ret, ':', '@');
			ret = strip(ret, '[', '`');
			ret = strip(ret, '{', '~');
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
	template<typename T>
	std::vector<T> split(T strv, std::string_view delims = " ")
	{
		std::vector<T> output;
		size_t         first = 0;

		while (first < strv.size())
		{
			const auto second = strv.find_first_of(delims, first);

			if (first != second)
				output.emplace_back(strv.substr(first, second - first));

			if (second == T::npos)
				break;

			first = second + 1;
		}

		return output;
	}

	// split_once
	auto split_once(std::string_view str, std::string_view delims = " ")
	{
		using namespace std::string_literals;
		size_t pos = str.find(delims);
		if (pos != std::string::npos)
		{
			return make_array(str.substr(0, pos), str.substr(pos + 1));
		}
		else
		{
			return make_array(str, ""s);
		}
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

	// match
	bool match(std::string_view pattern, std::string_view input)
	{
		if (pattern.empty() && input.empty())
			return true;
		else if (pattern.starts_with('*') && pattern.size() > 1 && input.empty())
			return false;
		else if (pattern.starts_with('?') || (!pattern.empty() and !input.empty() and pattern[0] == input[0]))
			return match(pattern.substr(1), input.substr(1));
		else if (pattern.starts_with('*'))
			return match(pattern.substr(1), input) || match(pattern, input.substr(1));

		return false;
	}


} // namespace deckard::string
