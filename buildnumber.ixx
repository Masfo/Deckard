module;
#include <cstdint>

export module Deckard;
export namespace Deckard
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 254;
	
	constexpr uint32_t version = major * 10000 + minor * 1000 + build;
	constexpr uint64_t random_seed = 0x43228c62e3e45de6;
	
	constexpr char version_string[] = "v0.0.254";
	constexpr char build_time_string[] = "2023-10-26 19:33:04";
	constexpr char phrase[] = "green-hate-disbelief";
	constexpr char calver[] = "2023.43.254";
	constexpr char uuid[] = "11EEA47C-8288-4FD9-A9CC-92359A377A5F";

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
}
