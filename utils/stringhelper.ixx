export module deckard.stringhelper;

import std;
import deckard.helpers;
import deckard.types;
import deckard.enums;

export namespace deckard::string
{
	constexpr std::string_view alphanum_string{" \t\f\n\r\v0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};

	constexpr std::string_view whitespace_string{alphanum_string.substr(0, 6)};
	constexpr std::string_view digit_string{alphanum_string.substr(6, 10)};
	constexpr std::string_view lowercase_string{alphanum_string.substr(16, 26)};
	constexpr std::string_view uppercase_string{alphanum_string.substr(42,26)};
	constexpr std::string_view alphabet_string{alphanum_string.substr(16, 52)};



	export enum class strip_option : u8 {
		digit      = BIT(0),
		whitespace = BIT(1),
		lowercase  = BIT(2),
		uppercase  = BIT(3),
		special    = BIT(4),

		reserved   = BIT(5),
		reserved2  = BIT(6),
		reserved3  = BIT(7),

		alphabet = lowercase|uppercase,
		alphanum = alphabet|digit,

		d  = digit,
		w  = whitespace,
		l  = lowercase,
		u  = uppercase,
		a  = l | u,
		an = a | d,
		s  = special,

	};

	consteval void enable_bitmask_operations(strip_option);

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

	bool match(std::string_view match_str, std::string_view input)
	{
		if (match_str.empty() && input.empty())
			return true;
		else if (match_str.starts_with('*') && match_str.size() > 1 && input.empty())
			return false;
		else if (match_str.starts_with('?') || (!match_str.empty() and !input.empty() and match_str[0] == input[0]))
			return match(match_str.substr(1), input.substr(1));
		else if (match_str.starts_with('*'))
			return match(match_str.substr(1), input) || match(match_str, input.substr(1));

		return false;
	}


} // namespace deckard::string
