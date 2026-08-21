export module deckard.commandline;

import std;
import deckard.types;
import deckard.utf8;
import deckard.helpers;

namespace deckard
{
	using namespace std::string_view_literals;

	export using target
	  = std::variant<bool*, i8*, u8*, i16*, u16*, i32*, u32*, i64*, u64*, f32*, f64*, std::string*, utf8::string*>;

	export struct option_spec
	{
		// Public
		utf8::view short_name{};
		utf8::view long_name{};
		utf8::view description{};
		bool       required{false};

		// Internal
		bool level_style{false};
		u32  arity{0};

		std::string         range_info;
		target              target_value{}; // flag and level
		std::vector<target> targets;


		std::optional<std::variant<i64, u64>> level_min;
		std::optional<std::variant<i64, u64>> level_max;
		std::optional<std::variant<i64, u64>> level_default;
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


		std::vector<option_spec> options;

		bool is_duplicate(utf8::view short_name, utf8::view long_name) const
		{
			auto it = std::ranges::find_if(
			  options,
			  [&](const option_spec& opt)
			  {
				  return (not short_name.empty() and short_name == opt.short_name)
						 or (not long_name.empty() and long_name == opt.long_name);
			  });
			return it != options.end();
		}

		bool register_option(option_spec opt)
		{
			if (is_duplicate(opt.short_name, opt.long_name))
			{
				warn_duplicate(opt.short_name, opt.long_name);
				return false;
			}

			options.push_back(std::move(opt));
			return true;
		}

		std::optional<utf8::view> match_level(utf8::view arg, const option_spec& opt) const
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

		bool apply_flag(const option_spec& opt) const
		{
			if (auto* p = std::get_if<bool*>(&opt.target_value); p and *p)
				**p = true;
			return true;
		}

		bool apply_single_value(const target& t, utf8::view value) const
		{
			return std::visit(
			  [&value]<typename P>(P p) -> bool
			  {
				  if (p == nullptr)
					  return true; // no target, just validate

				  using T = std::remove_pointer_t<P>;
				  if constexpr (std::is_same_v<T, bool>)
				  {
					  return false;
				  }
				  else if constexpr (std::is_same_v<T, std::string>)
				  {
					  *p = value.to_string();
					  return true;
				  }
				  else if constexpr (std::is_same_v<T, utf8::string>)
				  {
					  *p = utf8::string{value};
					  return true;
				  }
				  else
				  {
					  auto parsed = try_to_number<T>(value.as_string_view());
					  if (not parsed)
						  return false;
					  *p = *parsed;
					  return true;
				  }
			  },
			  t);
		}

		bool apply_option_values(const option_spec& opt, std::span<const utf8::view> values) const
		{
			if (values.size() != opt.targets.size())
				return false;

			for (size_t i = 0; i < values.size(); ++i)
				if (not apply_single_value(opt.targets[i], values[i]))
					return false;

			return true;
		}

		template<std::integral V>
		bool apply_level_value(const option_spec& opt, utf8::view value) const
		{
			V parsed_value{};

			if (value.empty())
			{
				parsed_value = opt.level_default ? std::get<V>(*opt.level_default) : V{};
			}
			else
			{
				auto parsed = try_to_number<V>(value.as_string_view());
				if (not parsed)
					return false;
				parsed_value = *parsed;
			}

			const V min_value = opt.level_min ? std::get<V>(*opt.level_min) : limits::min<V>;
			const V max_value = opt.level_max ? std::get<V>(*opt.level_max) : limits::max<V>;
			const V clamped   = std::clamp(parsed_value, min_value, max_value);

			if (clamped != parsed_value)
				std::println(std::cerr, "Warning: {} value {} clamped to {}", opt.short_name, parsed_value, clamped);

			return std::visit(
			  [clamped]<typename P>(P p) -> bool
			  {
				  if (p == nullptr)
					  return true; // no target, just validate

				  using T = std::remove_pointer_t<P>;
				  if constexpr (std::integral<T>)
				  {
					  *p = static_cast<T>(clamped);
					  return true;
				  }
				  else
				  {
					  return false;
				  }
			  },
			  opt.target_value);
		}

		bool apply_level(const option_spec& opt, utf8::view value) const
		{
			if (opt.level_min and std::holds_alternative<u64>(*opt.level_min))
				return apply_level_value<u64>(opt, value);

			return apply_level_value<i64>(opt, value);
		}

	public:
		explicit commandline(std::string_view name, std::string_view version)
			: program_name(name)
			, version(version)
		{
			flag({.short_name = "-h", .long_name = "--help", .description = "Show this help message"});
			flag({.short_name = "-V", .long_name = "--version", .description = "Show version information"});
		}

		commandline(const commandline&)            = delete;
		commandline& operator=(const commandline&) = delete;
		commandline(commandline&&)                 = delete;
		commandline& operator=(commandline&&)      = delete;

		commandline& flag(this auto& self, option_spec opt, bool* target = nullptr)
		{
			opt.level_style  = false;
			opt.target_value = target;
			opt.arity        = 0;
			self.register_option(std::move(opt));
			return self;
		}

		template<parsable T, std::same_as<T>... Rest>
		commandline& option(this auto& self, option_spec opt, T* target, Rest*... rest)
		{
			std::array<T*, 1 + sizeof...(Rest)> ptrs{target, rest...};

			std::set<T*> unique(ptrs.begin(), ptrs.end());
			if (unique.contains(nullptr) or unique.size() != ptrs.size())
			{
				std::println(std::cerr, "Warning: option {} has null or duplicate targets, skipping", opt.short_name);
				return self;
			}

			opt.level_style = false;
			opt.arity       = static_cast<u32>(ptrs.size());
			opt.targets.assign(ptrs.begin(), ptrs.end());
			self.register_option(std::move(opt));

			return self;
		}

		// -O[min,max], -O defaults to default_value
		template<std::integral T>
		commandline& level(this auto& self, option_spec opt, T* target = nullptr, T min_value = limits::min<T>,
						   T max_value = limits::max<T>, T default_value = T{})
		{

			bool has_range       = min_value != limits::min<T> or max_value != limits::max<T>;
			T    clamped_default = std::clamp(default_value, min_value, max_value);

			opt.arity        = 0;
			opt.level_style  = true;
			opt.range_info   = has_range ? std::format("[{}-{}]", min_value, max_value) : std::string{};
			opt.target_value = target;
			if constexpr (std::is_signed_v<T>)
			{
				opt.level_min     = static_cast<i64>(min_value);
				opt.level_max     = static_cast<i64>(max_value);
				opt.level_default = static_cast<i64>(clamped_default);
			}
			else
			{
				opt.level_min     = static_cast<u64>(min_value);
				opt.level_max     = static_cast<u64>(max_value);
				opt.level_default = static_cast<u64>(clamped_default);
			}
			self.register_option(std::move(opt));
			return self;
		}

		[[nodiscard]] bool parse(std::span<const utf8::view> args)
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

			std::vector<u8> seen(options.size(), 0);

			for (size_t i = 0; i < args.size(); ++i)
			{
				utf8::view arg = args[i];

				auto it = std::ranges::find_if(
				  options, [&](const option_spec& opt) { return arg == opt.short_name or arg == opt.long_name; });

				if (it != options.end())
				{
					seen[std::distance(options.begin(), it)] = 1;

					if (it->arity > 0)
					{
						if (i + it->arity >= args.size())
						{
							std::println(std::cerr, "Error: Missing value(s) for {}", arg);
							return false;
						}

						std::span<const utf8::view> vals{args.data() + i + 1, it->arity};
						if (not apply_option_values(*it, vals))
						{
							std::println(std::cerr, "Error: Invalid value for {}", arg);
							return false;
						}
						i += it->arity;
					}
					else if (it->level_style)
					{
						apply_level(*it, {});
					}
					else
					{
						apply_flag(*it);
					}
					continue;
				}

				auto level_it = std::ranges::find_if(
				  options, [&](const auto& o) { return match_level(arg, o).has_value(); });
				if (level_it != options.end())
				{
					auto level_value = match_level(arg, *level_it);

					seen[std::distance(options.begin(), level_it)] = 1;

					if (not apply_level(*level_it, *level_value))
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
				if (options[i].required and seen[i] == 0)
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

		[[nodiscard]] bool parse(int argc, char* argv[])
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

		[[nodiscard]] bool parse(std::string_view command_line)
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
					{
						std::println(std::cerr,
									 "Error: Unterminated quote {} in command line: {}",
									 quote == U'"' ? "\"" : "'",
									 command_line);
						return false;
					}

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
				std::string values;
				for (u32 n = 0; n < opt.arity; ++n)
					values += " <value>";
				std::string flag = opt.level_style ? std::format("{}<N>", short_sv.empty() ? long_sv : short_sv)
												   : std::format("{:<4} {:<15}{}", short_sv, long_sv, values);

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
