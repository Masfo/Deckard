module;
#include <cstdint>

export module Deckard;
export namespace Deckard
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 261;
	
	constexpr uint32_t version = major * 10000 + minor * 1000 + build;
	constexpr uint64_t random_seed = 0x15c69858af6e37c8;
	
	constexpr char version_string[] = "v0.0.261";
	constexpr char build_time_string[] = "2023-10-26 21:35:06";
	constexpr char phrase[] = "bootlace-ceramics-vantage";
	constexpr char calver[] = "2023.43.261";
	constexpr char uuid[] = "000C2E37-806C-49B4-A9C9-1A01629913B3";

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
