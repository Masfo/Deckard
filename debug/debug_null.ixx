module;
#include <Windows.h>
#include <cstdio>


export module deckard.debug;

import std;
using namespace std::string_view_literals;

void output_message(const std::string_view message) 
{ std::print(std::cout, "{}"sv, message); }

void error_output_message(const std::string_view message) { std::print(std::cerr, "{}"sv, message); }


template<typename... Args>
auto format(std::string_view fmt, Args&&... args)
{
	return std::vformat(fmt, std::make_format_args(args...));
}


struct alignas(64) FormatLocation
{
	std::string_view     fmt;
	std::source_location loc;

	FormatLocation(const char* s, const std::source_location& l = std::source_location::current())
		: fmt(s)
		, loc(l)
	{
	}

	FormatLocation(std::string_view s, const std::source_location& l = std::source_location::current())
		: fmt(s)
		, loc(l)
	{
	}

	std::string to_string() const { return std::format("{}({}): {}", loc.file_name(), loc.line(), fmt); }
};

export namespace deckard::dbg
{

	void breakpoint() { }

	template<typename... Args>
	void print(std::string_view fmt, Args&&...args)
	{
		if constexpr (sizeof...(Args) > 0)
			output_message(format(fmt, args...));
		else
			output_message(fmt);
	}

	// println
	template<typename... Args>
	void println(std::string_view fmt, Args&&...args)
	{
		if constexpr (sizeof...(Args) > 0)
		{
			output_message(format("{}\n"sv, format(fmt, args...)));
		}
		else
		{
			output_message(format("{}\n"sv, fmt));
		}
	}

	void println() { output_message("\n"); }

	// eprintln
	template<typename... Args>
	void eprintln(std::string_view fmt, Args&&...args)
	{
		if constexpr (sizeof...(Args) > 0)
		{
			error_output_message(format("{}\n"sv, format(fmt, args...)));
		}
		else
		{
			error_output_message(format("{}\n"sv, fmt));
		}
	}

	void eprintln() { error_output_message("\n"); }

	template<typename... Args>
	void if_true(bool, std::string_view, Args&&...)
	{
	}

	std::string who_called_me() { }

	void stacktrace(std::string_view) { }

	// trace
	template<typename... Args>
	void trace(FormatLocation, Args&&...)
	{
	}

	void trace([[maybe_unused]] const std::source_location& loc = std::source_location::current()) { }

	void trace([[maybe_unused]] FormatLocation fmt) { }

	// Panic
	template<typename... Args>
	[[noreturn]] void panic(std::string_view, Args&&...)
	{
		std::terminate();
	}

	[[noreturn]] void panic() { panic(""); }


} // namespace deckard::dbg

export namespace deckard
{
	template<typename... Args>
	void todo(std::string_view, Args&&...)
	{
	}

	void todo() { }


} // namespace deckard
