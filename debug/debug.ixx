module;
#include <Windows.h>


export module deckard.debug;

import std;

using namespace std::string_view_literals;

void output_message(const std::string_view message)
{
	std::print(std::cout, "{}"sv, message);



	OutputDebugStringA(message.data());
}

void error_output_message(const std::string_view message)
{
	std::print(std::cerr, "{}"sv, message);

	OutputDebugStringA(message.data());
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

static_assert(64 == sizeof(FormatLocation), "FormatLocation is not 64 bytes");

// template<typename... Args>
// auto format_check(std::format_string<Args...> format, Args&&... args)
// {
// 	return std::format(format, std::forward<Args>(args)...);
// }

template<typename... Args>
auto format(std::string_view fmt, Args&&... args)
{
	return std::vformat(fmt, std::make_format_args(args...));
}

export namespace deckard::dbg
{
	using namespace std::string_view_literals;


	// TODO: own debug window





	void breakpoint()
	{
#if __cpp_lib_debugging && __has_include(<debugging>)
#error "Debugging (C++26) is supported, use it"
#endif

		if (IsDebuggerPresent())
		{
			DebugBreak();
		}
	}

	// debug
	template<typename... Args>
	void print(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(Args) > 0)
			output_message(format(fmt, args...));
		else
			output_message(fmt);
	}

	// println
	template<typename... Args>
	void println(std::string_view fmt, Args&&... args)
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
	void eprintln(std::string_view fmt, Args&&... args)
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
	void if_true(bool cond, std::string_view fmt, Args&&... args)
	{
		if (cond)
		{
			if constexpr (sizeof...(Args) > 0)
				println(fmt, args...);
			else
				println("{}", fmt);
		}
	}

	std::string who_called_me(int index = 2)
	{
		auto strace = std::stacktrace::current();
		return std::to_string(strace[index]);
	}

	void stacktrace(std::string_view file_to_ignore = __FILE__)
	{
		auto traces = std::stacktrace::current();

		for (const auto& traceline : traces)
		{
			if (traceline.source_file().contains("catch2"))
				continue;

			if (traceline.source_file().contains(file_to_ignore))
				continue;

			dbg::println("{}({}): {}", traceline.source_file(), traceline.source_line(), traceline.description());
		}
	}

	// trace
	template<typename... Args>
	void trace(FormatLocation fmt, Args&&... args)
	{
		println("\n{}({}): {}"sv, fmt.loc.file_name(), fmt.loc.line(), format(fmt.fmt, args...));
	}

	void trace(const std::source_location& loc = std::source_location::current()) { println("\n{}({}):"sv, loc.file_name(), loc.line()); }

	void trace(FormatLocation fmt) { dbg::println("{}", fmt.to_string()); }

	// Panic
	template<typename... Args>
	[[noreturn]] void panic(std::string_view fmt, Args&&... args)
	{
		dbg::print("{} : ", who_called_me());
		if constexpr (sizeof...(args) > 0)
		{
			dbg::println("PANIC: {}", format(fmt, args...));
		}
		else
		{
			dbg::println("PANIC: {}", fmt);
		}

		breakpoint();
	}

	[[noreturn]] void panic() { panic(""); }


} // namespace deckard::dbg

export namespace deckard
{
	template<typename... Args>
	void todo(std::string_view fmt, Args&&... args)
	{
		dbg::eprintln(dbg::who_called_me());
		dbg::panic(std::format("TODO{} {}", fmt.empty() ? "" : ":", format(fmt, args...)));
	}

	void todo()
	{
		dbg::eprintln(dbg::who_called_me());
		dbg::panic(std::format("TODO"));
	}

} // namespace deckard
