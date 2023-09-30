module;
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module deckard.debug;

import std;

void output_message(const std::string_view message) noexcept { OutputDebugStringA(message.data()); }

struct alignas(64) FormatLocation
{
    std::string_view     fmt;
    std::source_location loc;
    FormatLocation(const char *s, const std::source_location &l = std::source_location::current()) noexcept
        : fmt(s), loc(l)
    {
    }
    FormatLocation(std::string_view s, const std::source_location &l = std::source_location::current()) noexcept
        : fmt(s), loc(l)
    {
    }
};
static_assert(64 == sizeof(FormatLocation), "FormatLocation is not 64 bytes");

export namespace deckard
{
    using namespace std::string_view_literals;


    // print
    template <typename... Args> void print(std::string_view fmt, Args &&...args) noexcept
    {
        output_message(std::format("{}", std::vformat(fmt, std::make_format_args(args...))));
    }

    // print
    void print(std::string_view fmt) noexcept { output_message(fmt); }

    // println
    template <typename... Args> void println(std::string_view fmt, Args &&...args) noexcept
    {

        output_message(std::format("{}\n", std::vformat(fmt, std::make_format_args(args...))));
    }

    // println
    void println() noexcept { output_message("\n"); }
    void println(std::string_view fmt) noexcept
    {
        output_message(fmt);
        println();
    }


    // trace
    template <typename... Args> void trace(FormatLocation fmt, Args &&...args) noexcept
    {
        output_message(std::format("{}({}): {}\n"sv,
                                   fmt.loc.file_name(),
                                   fmt.loc.line(),
                                   std::vformat(fmt.fmt, std::make_format_args(args...))));
    }

    // trace
    void trace(FormatLocation fmt) noexcept
    {
        output_message(std::format("{}({}): {}\n"sv, fmt.loc.file_name(), fmt.loc.line(), fmt.fmt));
    }

    void trace() noexcept { output_message("\n"); }





}   // namespace piku
