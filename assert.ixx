module;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



export module piku.assert;

import std;
import deckard.debug;

using namespace deckard;

export namespace deckard
{

#ifdef _DEBUG

[[noreturn]] void debug_and_halt()
{
    if (IsDebuggerPresent())
    {
        DebugBreak();
        FatalExit(0);
    }
}

export void assert_msg(bool expr, std::string_view message, const std::source_location &loc = std::source_location::current()) noexcept
{
    if (!expr)
    {
        println("\nAssert *****\n\n {}({}): {}", loc.file_name(), loc.line(), message);


        auto traces = std::stacktrace::current();

        for (const auto &trace : traces)
        {
            if (trace.source_file().contains(__FILE__))
                continue;

            trace("{}({}): {}", trace.source_file(), trace.source_line(), trace.description());
        }

        trace("\nAssert *****\n\n");

        debug_and_halt();
    }
}


export void assert(bool expr = false, const std::source_location &loc = std::source_location::current()) noexcept
{
    assert_msg(expr, "", loc);
}


#else
export void assert_msg(bool, std::string_view) noexcept { }
export void assert(bool) noexcept { }
}
#endif