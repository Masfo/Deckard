module;
#include <Windows.h>

export module deckard.debug;

import std;

using namespace std::string_view_literals;

void output_message(const std::string_view message) noexcept
{
	std::print(std::cout, "{}"sv, message);
#ifdef _DEBUG

	OutputDebugStringA(message.data());
#endif
}

struct alignas(64) FormatLocation
{
	std::string_view     fmt;
	std::source_location loc;

	FormatLocation(const char* s, const std::source_location& l = std::source_location::current()) noexcept
		: fmt(s)
		, loc(l)
	{
	}

	FormatLocation(std::string_view s, const std::source_location& l = std::source_location::current()) noexcept
		: fmt(s)
		, loc(l)
	{
	}

	std::string to_string() const { return std::format("{}({}): {}", loc.file_name(), loc.line(), fmt); }
};

static_assert(64 == sizeof(FormatLocation), "FormatLocation is not 64 bytes");

// template<typename... Args>
// auto format_check(std::format_string<Args...> format, Args&&... args) noexcept
// {
// 	return std::format(format, std::forward<Args>(args)...);
// }

template<typename... Args>
auto format(std::string_view fmt, Args&&... args) noexcept
{
	return std::vformat(fmt, std::make_format_args(args...));
}

export namespace deckard::dbg
{
	using namespace std::string_view_literals;

	// debug
	template<typename... Args>
	void print([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args) noexcept
	{
		if constexpr (sizeof...(Args) > 0)
			output_message(format(fmt, args...));
		else
			output_message(fmt);
	}

	// debugln

	template<typename... Args>
	void println([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args) noexcept
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

	void println() noexcept { output_message("\n"); }

	template<typename... Args>
	void if_true([[maybe_unused]] bool cond, [[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args) noexcept
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
	void trace([[maybe_unused]] FormatLocation fmt, [[maybe_unused]] Args&&... args) noexcept
	{
#ifdef _DEBUG
		println("{}({}): {}"sv, fmt.loc.file_name(), fmt.loc.line(), format(fmt.fmt, args...));
#endif
	}

	void trace([[maybe_unused]] const std::source_location& loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG
		println("{}({}):"sv, loc.file_name(), loc.line());
#endif
	}

	void trace([[maybe_unused]] FormatLocation fmt) noexcept { dbg::println("{}", fmt.to_string()); }

	// Panic
	template<typename... Args>
	[[noreturn]] void panic([[maybe_unused]] std::string_view fmt, [[maybe_unused]] Args&&... args) noexcept
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
		if (IsDebuggerPresent())
		{
			DebugBreak();
		}
#endif
		std::terminate();
	}

	[[noreturn]] void panic() noexcept { panic(""); }


} // namespace deckard::dbg

export namespace deckard
{
	void todo([[maybe_unused]] std::string_view            fmt,
			  [[maybe_unused]] const std::source_location& loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG
		dbg::println("{}({}): TODO: {}", loc.file_name(), loc.line(), fmt);
		if (IsDebuggerPresent())
			DebugBreak();
#endif
	}


} // namespace deckard
