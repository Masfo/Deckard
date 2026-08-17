export module deckard.commandline;

import std;
import deckard.utf8;

namespace deckard
{
	export struct option
	{
		// Public
		utf8::view short_name;
		utf8::view long_name;
		utf8::view description;
		bool       required{false};
	};

	void warn_duplicate(utf8::view short_name, utf8::view long_name)
	{
		std::println(
		  std::cerr, "Warning: option {} already registered, skipping", short_name.empty() ? long_name : short_name);
	}

	template<typename T>
	concept parsable = std::same_as<T, std::string> or std::integral<T> or std::floating_point<T>;

	export class commandline
	{
	private:
		std::string program_name;
		std::string version;


	public:
		explicit commandline(std::string_view name, std::string_view version)
			: program_name(name)
			, version(version)
		{
		}

		commandline& add_flag(this auto& self, option opt, bool* target = nullptr) { return self; }

		template<parsable T>
		commandline& add_option(this auto& self, option opt, T* target = nullptr)
		{

			return self;
		}

		// Compiler-style level flag (e.g. -O3). Bare "-O" (empty value) resolves to default_value.
		template<std::integral T>
		commandline& add_level_option(
		  this auto& self, option opt, T* target = nullptr, T min_value = std::numeric_limits<T>::min(),
		  T max_value = std::numeric_limits<T>::max(), T default_value = T{})
		{

			return self;
		}

		bool parse(std::span<const std::string_view> args)
		{
			if (args.empty())
			{
				help();
				return false;
			}

			// override -h and -V
			for (auto arg : args)
			{
				if (arg == "-h" or arg == "--help")
				{
					help();
					return false;
				}
				if (arg == "-V" or arg == "--version")
				{
					version_info();
					return false;
				}
			}

			return true;
		}

		bool parse(int argc, char* argv[])
		{
			if (argc <= 1)
			{
				help();
				return false;
			}

			std::vector<std::string_view> args;
			args.reserve(static_cast<size_t>(argc) - 1);
			for (int i = 1; i < argc; ++i)
				args.emplace_back(argv[i]);

			return parse(args);
		}

		bool parse(std::string_view command_line)
		{
			std::vector<std::string_view> args;
			utf8::scanner                 s{command_line};

			while (s.has_next())
			{
				s.skip_whitespace();
				if (not s.has_next())
					break;

				if (s.is(U'"') or s.is(U'\''))
				{
					char32 quote = s.current();
					s.skip();
					auto value = s.take_until(quote);
					if (not s.has_next())
					{
						std::println(std::cerr, "Error: Missing closing quote for {}", utf8::as_utf8(quote));
						return false;
					}

					args.push_back(value.as_string_view());
					s.skip();
				}
				else
				{
					args.push_back(s.take_while([](char32 cp) { return not utf8::is_whitespace(cp); }).as_string_view());
				}
			}

			return parse(args);
		}

		void help() const
		{
			std::println("\nUsage: {} [options]\n\nOptions:", program_name);

			std::println();
		}

		void version_info() const { std::println("{} version {}", program_name, version); }
	};

} // namespace deckard
