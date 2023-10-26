module;
#include <cstdint>

export module Deckard;
export namespace Deckard
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 262;
	
	constexpr uint32_t version = major * 10000 + minor * 1000 + build;
	constexpr uint64_t random_seed = 0xe458c45c60fa2e5a;
	
	constexpr char version_string[] = "v0.0.262";
	constexpr char build_time_string[] = "2023-10-26 21:43:21";
	constexpr char phrase[] = "stout-gawk-parrot";
	constexpr char calver[] = "2023.43.262";
	constexpr char uuid[] = "296CA924-531B-43AE-88B6-BC09A09DD06D";

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
