module;
#include <Windows.h>
#include <cstdio>


export module deckard.debug;


import std;

using namespace std::string_view_literals;

void output_message(const std::string_view message)
{
	std::print(std::cout, "{}"sv, message);
#ifdef _DEBUG

	OutputDebugStringA(message.data());
#endif
}

void error_output_message(const std::string_view message)
{
	std::print(std::cerr, "{}"sv, message);
#ifdef _DEBUG

	OutputDebugStringA(message.data());
#endif
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

	void breakpoint()
	{
#if __cpp_lib_debugging && __has_include(<debugging>)
#error "Debugging (C++26) is supported, use it"
#endif

#ifdef _DEBUG
		if (IsDebuggerPresent())
		{
			DebugBreak();
		}
#endif
	}

	// debug
	template<typename... Args>
	void print([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args)
	{
		if constexpr (sizeof...(Args) > 0)
			output_message(format(fmt, args...));
		else
			output_message(fmt);
	}

	// println
	template<typename... Args>
	void println([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args)
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
	void eprintln([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args)
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
	void if_true([[maybe_unused]] bool cond, [[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args)
	{
#ifdef _DEBUG
		if (cond)
		{
			if constexpr (sizeof...(Args) > 0)
				println(fmt, args...);
			else
				println("{}", fmt);
		}
#endif
	}

	std::string who_called_me()
	{
#ifdef _DEBUG
		auto strace = std::stacktrace::current();
		return std::to_string(strace[2]);
#else
		return {};
#endif
	}

	// trace
	template<typename... Args>
	void trace([[maybe_unused]] FormatLocation fmt, [[maybe_unused]] Args&&... args)
	{
#ifdef _DEBUG
		println("{}({}): {}"sv, fmt.loc.file_name(), fmt.loc.line(), format(fmt.fmt, args...));
#endif
	}

	void trace([[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
#ifdef _DEBUG
		println("{}({}):"sv, loc.file_name(), loc.line());
#endif
	}

	void trace([[maybe_unused]] FormatLocation fmt) { dbg::println("{}", fmt.to_string()); }

	// Panic
	template<typename... Args>
	[[noreturn]] void panic([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args)
	{
#ifdef _DEBUG
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
#endif
		std::terminate();
	}

	[[noreturn]] void panic() { panic(""); }

	void redirect_console(bool [[maybe_unused]] show)
	{

		if (show)
		{
			static FILE* pNewStdout = nullptr;
			static FILE* pNewStderr = nullptr;


			if (AttachConsole(ATTACH_PARENT_PROCESS) == 0)
				AllocConsole();

			if (::freopen_s(&pNewStdout, "CONOUT$", "w", stdout) == 0)
				std::cout.clear();

			if (::freopen_s(&pNewStderr, "CONOUT$", "w", stderr) == 0)
				std::cerr.clear();

			std::ios::sync_with_stdio(1);
		}
		else
		{
			FreeConsole();
		}
#ifdef _DEBUG
#endif
	}


} // namespace deckard::dbg

export namespace deckard
{
	template<typename... Args>
	void todo([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args)
	{
		//
		deckard::dbg::println(format(fmt, args...));
		deckard::dbg::breakpoint();
	}

	void todo() { todo("TODO"); }


} // namespace deckard
