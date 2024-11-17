export module deckard.stringhelper;

import std;
import deckard.helpers;
import deckard.types;

export namespace deckard::string
{

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
	std::vector<std::string_view> split(std::string_view strv, std::string_view delims = " ")
	{
		std::vector<std::string_view> output;
		size_t                        first = 0;

		while (first < strv.size())
		{
			const auto second = strv.find_first_of(delims, first);

			if (first != second)
				output.emplace_back(strv.substr(first, second - first));

			if (second == std::string_view::npos)
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
	std::vector<std::string_view> split_exact(std::string_view str, std::string_view delims, bool include_empty = false)
	{
		std::vector<std::string_view> output;
		u64                           first = 0;
		while (first < str.size())
		{
			const auto second = str.find(delims, first);

			if (include_empty || first != second) // empty line
				output.emplace_back(str.substr(first, second - first));

			if (second == std::string_view::npos)
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
}
