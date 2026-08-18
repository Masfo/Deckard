export module deckard.commandline;

import std;
import deckard.types;
import deckard.utf8;
import deckard.helpers;

namespace deckard
{
	using namespace std::string_view_literals;

	export struct option
	{
		// Public
		utf8::view short_name;
		utf8::view long_name;
		utf8::view description;
		bool       required{false};

		// Internal
		bool        level_style{false};
		bool        expects_value{false};
		std::string range_info;

		std::move_only_function<bool(utf8::view) const> parse_action;

		bool invoke(utf8::view value = {}) const { return parse_action(value); }
	};

	void warn_duplicate(utf8::view short_name, utf8::view long_name)
	{
		std::println(
		  std::cerr, "Warning: option {} already registered, skipping", short_name.empty() ? long_name : short_name);
	}

	template<typename T>
	concept parsable
	  = std::same_as<T, std::string> or std::same_as<T, utf8::string> or std::integral<T> or std::floating_point<T>;

	export class commandline
	{
	private:
		std::string program_name;
		std::string version;


		std::vector<option> options;

		bool is_duplicate(utf8::view short_name, utf8::view long_name) const
		{
			auto it = std::ranges::find_if(
			  options,
			  [&](const option& opt)
			  {
				  return (not short_name.empty() and short_name == opt.short_name)
						 or (not long_name.empty() and long_name == opt.long_name);
			  });
			return it != options.end();
		}

		bool register_option(option opt)
		{
			if (is_duplicate(opt.short_name, opt.long_name))
			{
				warn_duplicate(opt.short_name, opt.long_name);
				return false;
			}

			options.push_back(std::move(opt));
			return true;
		}

		std::optional<utf8::view> match_level(utf8::view arg, const option& opt) const
		{
			if (not opt.level_style)
				return std::nullopt;

			for (auto prefix : {opt.short_name, opt.long_name})
			{
				if (not prefix.empty() and arg.size() > prefix.size() and arg.starts_with(prefix))
				{
					auto rest = arg.subview(prefix.size());
					if (std::ranges::all_of(rest, [](char32 c) { return c >= U'0' and c <= U'9'; }))
						return rest;
				}
			}
			return std::nullopt;
		}

	public:
		explicit commandline(std::string_view name, std::string_view version)
			: program_name(name)
			, version(version)
		{
			add_flag({.short_name = "-h", .long_name = "--help", .description = "Show this help message"});
			add_flag({.short_name = "-V", .long_name = "--version", .description = "Show version information"});
		}

		commandline& add_flag(this auto& self, option opt, bool* target = nullptr)
		{
			opt.expects_value = false;
			opt.level_style   = false;
			opt.parse_action  = [target](utf8::view)
			{
				if (target)
					*target = true;
				return true;
			};
			self.register_option(std::move(opt));
			return self;
		}

		template<parsable T>
		commandline& add_option(this auto& self, option opt, T* target = nullptr)
		{
			opt.expects_value = true;
			opt.level_style   = false;
			opt.parse_action  = [target](utf8::view val) -> bool
			{
				if (not target)
					return false;

				if constexpr (std::is_same_v<T, std::string>)
				{
					*target = val.to_string();
					return true;
				}
				else if constexpr (std::is_same_v<T, utf8::string>)
				{
					*target = utf8::string{val};
					return true;
				}
				else
				{
					auto parsed = try_to_number<T>(val.as_string_view());
					if (not parsed)
						return false;
					*target = *parsed;
					return true;
				}
			};
			self.register_option(std::move(opt));
			return self;
		}

		// -O[min,max], -O defaults to default_value
		template<std::integral T>
		commandline& add_level_option(this auto& self, option opt, T* target = nullptr, T min_value = limits::min<T>,
									  T max_value = limits::max<T>, T default_value = T{})
		{

			bool has_range       = min_value != std::numeric_limits<T>::min() or max_value != std::numeric_limits<T>::max();
			T    clamped_default = std::clamp(default_value, min_value, max_value);

			opt.expects_value = false;
			opt.level_style   = true;
			opt.range_info    = has_range ? std::format("[{}-{}]", min_value, max_value) : std::string{};
			opt.parse_action  = [target, min_value, max_value, clamped_default, short_name = opt.short_name.to_string()](
								  utf8::view val) -> bool
			{
				if (not target)
					return false;

				if (val.empty()) // bare flag, e.g. "-O" with no digits
				{
					*target = clamped_default;
					return true;
				}

				auto parsed = try_to_number<T>(val.as_string_view());
				if (not parsed)
					return false;

				T clamped = std::clamp(*parsed, min_value, max_value);
				if (clamped != parsed)
					std::println(std::cerr, "Warning: {} value {} clamped to {}", short_name, *parsed, clamped);

				*target = clamped;
				return true;
			};
			self.register_option(std::move(opt));
			return self;
		}

		bool parse(std::span<const utf8::view> args)
		{
			if (args.empty())
			{
				help();
				return false;
			}

			// override -h and -V
			for (auto arg : args)
			{
				if (arg == "-h"sv or arg == "--help"sv)
				{
					help();
					return false;
				}
				if (arg == "-V"sv or arg == "--version"sv)
				{
					version_info();
					return false;
				}
			}

			std::vector<bool> seen(options.size(), false);

			for (size_t i = 0; i < args.size(); ++i)
			{
				utf8::view arg = args[i];

				auto it = std::ranges::find_if(
				  options, [&](const option& opt) { return arg == opt.short_name or arg == opt.long_name; });

				if (it != options.end())
				{
					seen[std::distance(options.begin(), it)] = true;

					if (it->expects_value)
					{
						if (i + 1 >= args.size())
						{
							std::println(std::cerr, "Error: Missing value for {}", arg);
							return false;
						}

						if (not it->invoke(args[++i]))
						{
							std::println(std::cerr, "Error: Invalid value for {}", arg);
							return false;
						}
					}
					else
					{
						it->invoke();
					}
					continue;
				}

				auto level_it    = options.end();
				auto level_value = std::optional<utf8::view>{};
				for (auto candidate = options.begin(); candidate != options.end(); ++candidate)
				{
					if (auto value = match_level(arg, *candidate))
					{
						level_it    = candidate;
						level_value = value;
						break;
					}
				}

				if (level_it != options.end())
				{
					seen[std::distance(options.begin(), level_it)] = true;

					if (not level_it->invoke(*level_value))
					{
						std::println(std::cerr, "Error: Invalid value for {}", arg);
						return false;
					}
					continue;
				}

				std::println(std::cerr, "Error: Unknown option {}", arg);
				return false;
			}

			bool missing_required = false;
			for (size_t i = 0; i < options.size(); ++i)
			{
				if (options[i].required && !seen[i])
				{
					std::println(
					  std::cerr, "Error: Missing required option {}/{}", options[i].short_name, options[i].long_name);
					missing_required = true;
				}
			}
			if (missing_required)
				return false;

			return true;
		}

		bool parse(int argc, char* argv[])
		{
			if (argc <= 1)
			{
				help();
				return false;
			}

			std::vector<utf8::view> args;
			args.reserve(static_cast<size_t>(argc) - 1);
			for (int i = 1; i < argc; ++i)
				args.emplace_back(argv[i]);

			return parse(args);
		}

		bool parse(std::string_view command_line)
		{
			std::vector<utf8::view> args;
			utf8::scanner           s{command_line};

			while (s.has_next())
			{
				s.skip_whitespace();
				if (not s.has_next())
					break;

				if (s.is(U'"') or s.is(U'\''))
				{
					char32 quote = s.current();
					s.skip();

					auto token = s.take_until(quote);
					if (not s.is(quote))
						return false;

					args.push_back(token.as_string_view());
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
			for (const auto& opt : options)
			{
				auto        short_sv = opt.short_name.as_string_view();
				auto        long_sv  = opt.long_name.as_string_view();
				std::string flag     = opt.level_style ? std::format("{}<N>", short_sv.empty() ? long_sv : short_sv)
													   : std::format("{:<4} {:<15}", short_sv, long_sv);

				std::println(
				  "  {:<20} {}{}{}",
				  flag,
				  opt.description.as_string_view(),
				  opt.range_info.empty() ? "" : std::format(" {}", opt.range_info),
				  opt.required ? " (required)" : "");
			}
			std::println();
		}

		void version_info() const { std::println("{} version {}", program_name, version); }
	};

} // namespace deckard
