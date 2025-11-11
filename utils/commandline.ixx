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
import deckard.platform;
import deckard.as;

using namespace std::literals::string_view_literals;

namespace deckard
{
	export std::vector<std::string> get_arguments([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
	{
		std::vector<std::string> args(std::next(argv, 1), std::next(argv, static_cast<std::ptrdiff_t>(argc)));
		return args;
	}

	export std::string join_arguments(int argc, const char* argv[])
	{
		const auto ret = get_arguments(argc, argv);
		return ret | std::views::join_with(' ') | std::ranges::to<std::string>();
	}

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

	struct option
	{
		Value       default_value;
		std::string help_description;
	};

	class cli
	{
	private:
		std::string input;
		u64         pos{0};

		std::unordered_map<std::string, option>      options;
		std::unordered_map<std::string, std::string> alias;
		std::unordered_map<std::string, std::string> descriptions;


	public:
		void skip_whitespace()
		{
			while (pos < input.size() and utf8::is_whitespace(input[pos]))
				++pos;
		}

		std::optional<char> peek() const { return pos < input.size() ? std::optional(input[pos]) : std::nullopt; }

		std::optional<char> consume() { return pos < input.size() ? std::optional(input[pos++]) : std::nullopt; }

		std::expected<char, std::string> expect(char expected_char)
		{
			auto got = consume();
			if (not got or *got != expected_char)
				return std::unexpected(std::format(R"(Expected '{}', got '{}')", expected_char, *got));

			return expected_char;
		}

		std::expected<std::string_view, std::string> expect(std::string_view expected_string)
		{
			for (const auto& expected_char : expected_string)
			{
				auto got = consume();
				if (not got or *got != expected_char)
					return std::unexpected(std::format(R"(Expected "{}", got "{}")", expected_char, *got));
			}

			return expected_string;
		}

		cli(int argc, const char* argv[]) { input = join_arguments(argc, argv); }

		cli(std::string_view sv)
			: input(sv)
		{
		}

		template<typename T>
		void add_argument(std::string_view shortname, std::string_view longname, std::string_view description)
		{
			//
		}

		void add_option(const std::string_view shortname, std::string_view longname, std::string_view description, option o)
		{
			//
			std::string ln{longname};
			alias[ln]                     = shortname;
			alias[std::string{shortname}] = ln;

			options[ln] = o;

			descriptions[ln] = description;
		}

		void add_flag(const std::string& shortname, const std::string& longname, const std::string& description)
		{
			// TODO: error expected
			if (shortname.empty() and longname.empty())
				return;
			else if (not shortname.empty() and not longname.empty())
			{
				alias[shortname] = longname;
				alias[longname]  = shortname;
			}
			else if (shortname.empty() and not longname.empty())
			{
				alias[longname] = longname;
			}
			else if (not shortname.empty() and longname.empty())
			{
				alias[shortname] = shortname;
			}

			option o{.default_value = false, .help_description = description};
			options[longname] = o;
		}
	};

	export void test_cmdliner()
	{
		int         argc   = 3;
		const char* argv[] = {"program", "-d", "-v"};

		cli cliv(argc, argv);

		//
		std::string input(R"(-d -v -dv)");

		cli cli(input);

		cli.add_flag("d", "debug", "Debug output");
		cli.add_flag("v", "verbose", "Verbose output");
		cli.add_flag("r", "", "Rest");
		cli.add_flag("", "east", "east");


		while (cli.peek().has_value())
		{
			dbg::println("{}", utf8::string(*cli.consume()));
		}

		Value v = true;

		auto new_bool = try_to_value<bool>(v);
		auto new_str2 = try_to_value<std::string>(v);

		v = "hello";

		auto new_str   = try_to_value<std::string>(v);
		auto new_bool2 = try_to_value<bool>(v);


		_;
	}


} // namespace deckard
