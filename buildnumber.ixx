module;
#include <cstdint>

export module Deckard;

export namespace Deckard
{
	// You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	// Do not modify these
	constexpr uint32_t build = 260;

	constexpr uint32_t version     = major * 1'0000 + minor * 1000 + build;
	constexpr uint64_t random_seed = 0xeeae'a7c7'2191'1465;

	constexpr char version_string[]    = "v0.0.255";
	constexpr char build_time_string[] = "2023-10-26 21:26:49";
	constexpr char phrase[]            = "tumbling-pregnant-customize";
	constexpr char calver[]            = "2023.43.255";
	constexpr char uuid[]              = "22915652-6ABC-4D06-A379-18293DF32958";

	// Copy paste to import to your project
	/*
		constexpr auto major = Deckard::major;
		constexpr auto minor = Deckard::minor;
		constexpr auto build = Deckard::build;
		constexpr auto version = Deckard::version;
		constexpr auto random_seed = Deckard::random_seed;

		constexpr auto version_string = Deckard::version_string;
		constexpr auto build_time_string = Deckard::build_time_string;
		constexpr auto phrase = Deckard::phrase;
		constexpr auto calver = Deckard::calver;
		constexpr auto uuid = Deckard::uuid;
	*/
} // namespace Deckard
