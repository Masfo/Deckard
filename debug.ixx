module;
#include <Windows.h>

export module deckard.debug;

import std;

void output_message(const std::string_view message) noexcept { OutputDebugStringA(message.data()); }

struct alignas(64) FormatLocation
{
	std::string_view     fmt;
	std::source_location loc;

	FormatLocation(const char *s, const std::source_location &l = std::source_location::current()) noexcept
		: fmt(s)
		, loc(l)
	{
	}

	FormatLocation(std::string_view s, const std::source_location &l = std::source_location::current()) noexcept
		: fmt(s)
		, loc(l)
	{
	}
};

static_assert(64 == sizeof(FormatLocation), "FormatLocation is not 64 bytes");

export namespace deckard
{
	using namespace std::string_view_literals;

	// debug
	template<typename... Args>
	void dbg(FormatLocation fmt, Args &&...args) noexcept
	{
		output_message(std::format("{}"sv, std::vformat(fmt.fmt, std::make_format_args(args...))));
	}

	void dbg(FormatLocation fmt) noexcept { output_message(std::format("{}"sv, fmt.fmt)); }

	// debugln
	template<typename... Args>
	void dbgln(FormatLocation fmt, Args &&...args) noexcept
	{
		output_message(std::format("{}\n"sv, std::vformat(fmt.fmt, std::make_format_args(args...))));
	}

	void dbgln(FormatLocation fmt) noexcept { output_message(std::format("{}\n"sv, fmt.fmt)); }

	void dbgln() noexcept { output_message("\n"); }

	// trace
	template<typename... Args>
	void trace(FormatLocation fmt, Args &&...args) noexcept
	{
		output_message(
			std::format("{}({}): {}\n"sv, fmt.loc.file_name(), fmt.loc.line(), std::vformat(fmt.fmt, std::make_format_args(args...))));
	}

	void trace(FormatLocation fmt) noexcept { output_message(std::format("{}({}): {}\n"sv, fmt.loc.file_name(), fmt.loc.line(), fmt.fmt)); }

	void trace() noexcept { output_message("\n"); }

	// Panic
	template<typename... Args>
	[[noreturn]] void panic(std::string_view fmt, Args &&...args) noexcept
	{
		dbgln("PANIC: ", std::vformat(fmt.fmt, std::make_format_args(args...)));
		if (IsDebuggerPresent())
		{
			DebugBreak();
		}
		FatalExit(0);
		std::unreachable();
	}

	[[noreturn]] void panic(std::string_view message = "") noexcept { panic(message); }

} // namespace deckard
