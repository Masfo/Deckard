export module deckard.commandline;

import std;
import deckard.debug;
import deckard.types;
import deckard.utf8;
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
	 */


	// using CLIValue = std::variant<std::monostate, bool, char, i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, std::string>;
	using CLIValue = std::variant<bool, std::string>;


	const auto visitor = overloads{
	  [](bool b)
	  {
		  dbg::println("bool = {}\n", b);
		  return b;
	  },
	  [](std::string_view s) { dbg::println("string = “{}”", s); },
	  [](const i64 v) { dbg::println("i64 = {}", v); }};

	template<class T>
	auto try_to_value(const CLIValue& v) -> std::optional<T>
	{

		if constexpr (std::is_same_v<T, bool>)
		{
			if (std::holds_alternative<bool>(v))
				return std::get<bool>(v);
			return {};
		}

#if 0
		if (std::holds_alternative<char>(v))
			return std::get<char>(v);

		// integers
		if (std::holds_alternative<i8>(v))
			return std::get<i8>(v);

		if (std::holds_alternative<u8>(v))
			return std::get<u8>(v);

		if (std::holds_alternative<i16>(v))
			return std::get<i16>(v);

		if (std::holds_alternative<u16>(v))
			return std::get<u16>(v);

		if (std::holds_alternative<i32>(v))
			return std::get<i32>(v);

		if (std::holds_alternative<u32>(v))
			return std::get<u32>(v);

		if (std::holds_alternative<i64>(v))
			return std::get<i64>(v);

		if (std::holds_alternative<u64>(v))
			return std::get<u64>(v);

		// floats
		if (std::holds_alternative<f32>(v))
			return std::get<f32>(v);

		if (std::holds_alternative<f64>(v))
			return std::get<f64>(v);

#endif
		//
		if constexpr (std::is_same_v<T, std::string>)
		{
			if (std::holds_alternative<std::string>(v))
				return std::get<std::string>(v);
			return {};
		}

		return T{};
	}

	static_assert(sizeof(CLIValue) == 48);

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

		CLIValue v = true;

		auto new_bool = try_to_value<bool>(v);
		auto new_str2 = try_to_value<std::string>(v);

		v = "hello";

		auto new_str   = try_to_value<std::string>(v);
		auto new_bool2 = try_to_value<bool>(v);


		_;
	}


} // namespace deckard
