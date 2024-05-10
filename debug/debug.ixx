module;
#include <Windows.h>
#include <sqlite3.h>

export module deckard.debug;

import std;

using namespace std::string_view_literals;

void output_message(const std::string_view message) noexcept
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

export namespace deckard::dbg
{
	using namespace std::string_view_literals;

	// debug
	template<typename... Args>
	void print(std::string_view fmt, Args &&...args) noexcept
	{
		output_message(std::format("{}"sv, std::vformat(fmt, std::make_format_args(args...))));
	}

	void print(std::string_view fmt) noexcept { output_message(std::format("{}"sv, fmt)); }

	// debugln
	template<typename... Args>
	void println(std::string_view fmt, Args &&...args) noexcept
	{
		output_message(std::format("{}\n"sv, std::vformat(fmt, std::make_format_args(args...))));
	}

	void println(std::string_view fmt) noexcept { output_message(std::format("{}\n"sv, fmt)); }

	void println() noexcept { output_message("\n"); }

	void if_true(bool cond, std::string_view fmt) noexcept
	{
		if (cond)
			println(fmt);
	}

	template<typename... Args>
	void if_true(bool cond, std::string_view fmt, Args &&...args) noexcept
	{
		if (cond)
			println(fmt, args...);
	}

	// trace
	template<typename... Args>
	void trace(FormatLocation fmt, Args &&...args) noexcept
	{
		output_message(std::format(
						"{}({}): {}\n"sv, fmt.loc.file_name(), fmt.loc.line(), std::vformat(fmt.fmt, std::make_format_args(args...))));
	}

	void trace(const std::source_location &loc) noexcept { output_message(std::format("{}({}): "sv, loc.file_name(), loc.line())); }

	void trace(FormatLocation fmt) noexcept { output_message(std::format("{}({}): {}\n"sv, fmt.loc.file_name(), fmt.loc.line(), fmt.fmt)); }

	void trace() noexcept { output_message("\n"); }

	// Panic
	template<typename... Args>
	[[noreturn]] void panic(std::string_view fmt, Args &&...args) noexcept
	{
		if constexpr (sizeof...(args) > 0)
		{
			const auto formatted = std::vformat(fmt, std::make_format_args(args...));
			trace("PANIC: {}", formatted);
		}
		else
		{
			trace("PANIC: {}", fmt);
		}
		if (IsDebuggerPresent())
		{
			DebugBreak();
		}
		std::terminate();
	}

	template<typename... Args>
	[[noreturn]] void panic_throw(std::string_view fmt, Args &&...args)
	{
		const auto f = format("divide by zero: {}", std::vformat(fmt, std::make_format_args(args...)));
		throw std::domain_error{f};
	}

	[[noreturn]] void panic() noexcept { panic(""); }

	[[noreturn]] void panic_throw() { panic_throw(""); }

	inline void who_called_me(const std::source_location &loc = std::source_location::current())
	{
#ifdef _DEBUG
		auto strace = std::stacktrace::current();
		dbg::println("{}({}): {}: {}", loc.file_name(), loc.line(), loc.function_name(), std::to_string(strace[2]));
#endif
	}

} // namespace deckard::dbg

export namespace deckard
{
	void todo(std::string_view str, const std::source_location &loc = std::source_location::current()) noexcept
	{
		dbg::trace(loc);
		dbg::println("TODO: {}", str);
		if (IsDebuggerPresent())
			DebugBreak();
	}


} // namespace deckard
