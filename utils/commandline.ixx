module;
#if defined(_WIN32)
#include <Windows.h>
#include <shellapi.h>
#endif

export module deckard.commandline;

import std;
import deckard.debug;
import deckard.types;
import deckard.utf8;
import deckard.win32;
import deckard.as;

using namespace std::literals::string_view_literals;

namespace deckard
{
	/*
	 *   -v  --verbose				- Flag
	 *  -O1 -O2 -optimize=2			- Level, single (O1), clamped 0-3 (O0, O1, O2, O3)
	 *  --log=debug.log				- Option, single value (no short)
	 *  --path=path/to/file			- Option, single value (no short)
	 *
	 *
	 *	-d, --debug					- Option, single value (no short)
	 *	-v, --verbose				- Flag
	 *  -dv (same as -d -v)			- Multiple flags
	 *
	 *  Key:
	 *		-o --output
	 *
	 *  Value:
	 *		--output=file
	 *		--output file
	 *
	 *  Booleans:
	 *		f,false,t,true, 0,1
	 *
	 *	bool verbose = false;
	 *  cli.option("-v, --verbose", "Enable verbose output", verbose);
	 *
	 *  Validate in option:
	 *  .validate, .range, is_directory, is_file, is_ipv4/6,
	 *
	 *
	 *  Flag	- boolean
	 *		-v			- short form
	 *		--verbose	- long form
	 *  Option	- some value
	 *		-o <path>
	 *      --output <path>
	 *
	 *		-o=<path>
	 *		--output=<path>
	 *
	 *		-/o=<path>		slash to ignore/disable
	 *		--/output=<path>
	 *
	 *  Subcommands:
	 *		TODO
	 *
	 *	cli cli;
	 *
	 *
	 */

	export std::vector<std::string> get_arguments([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
	{
#if defined(_WIN32)
		int     wargc = 0;
		LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
		if (not argv)
			return {};

		std::vector<std::string> ret;
		ret.reserve(wargc);

		for (int i = 1; i < wargc; ++i)
			ret.emplace_back(system::from_wide(wargv[i]));

		return ret;
#else

		std::vector<std::string> args(argv, argv + argc);
		returg                   args | std::views::drop(1) | std::ranges::to<std::vector>();
#endif
	}

	export std::string join_arguments(int argc, char* argv[])
	{
		const auto ret = get_arguments(argc, argv);
		return ret | std::views::join_with(' ') | std::ranges::to<std::string>();
	}

	using Value = std::variant<bool, char, i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, std::string>;

	template<class T>
	auto try_to_value(const Value& v) -> std::optional<T>
	{

#define MACRO_TypeGet(TYPE)                                                                                                                \
	if constexpr (std::is_same_v<T, TYPE>)                                                                                                 \
	{                                                                                                                                      \
		if (std::holds_alternative<TYPE>(v))                                                                                               \
			return std::get<TYPE>(v);                                                                                                      \
		return {};                                                                                                                         \
	}

#if defined(__cpp_reflection)
		error "Native reflection supported. use it."
#endif
		  // TODO: reflection?

		  MACRO_TypeGet(bool);
		MACRO_TypeGet(char);

		MACRO_TypeGet(i8);
		MACRO_TypeGet(u8);
		MACRO_TypeGet(i16);
		MACRO_TypeGet(u16);
		MACRO_TypeGet(i32);
		MACRO_TypeGet(u32);
		MACRO_TypeGet(i64);
		MACRO_TypeGet(u64);
		MACRO_TypeGet(f32);
		MACRO_TypeGet(f64);
		MACRO_TypeGet(std::string);
#undef MACRO_TypeGet
	}

	static_assert(sizeof(Value) == 48);

	class cli
	{
	private:
		utf8::string                   commandline;
		std::unordered_map<char, bool> short_flags;

	public:
		cli(int argc, const char* const argv[])
		{
			std::vector<std::string> args(argv, argv + argc);
			commandline = args | std::views::join_with(' ') | std::ranges::to<std::string>();
		}

		cli(std::string_view sv)
			: commandline(sv)
		{
		}

		cli(utf8::string us)
			: commandline(us)
		{
		}

		void flag(std::string_view name)
		{
			//
		}
	};

	export void test_cmdliner()
	{
		//
		std::string input(R"(-d -v)");

		cli cli(input);

		Value v = true;

		auto new_bool = try_to_value<bool>(v);
		auto new_str2 = try_to_value<std::string>(v);

		v = "hello";

		auto new_str   = try_to_value<std::string>(v);
		auto new_bool2 = try_to_value<bool>(v);


		_;
	}


} // namespace deckard
