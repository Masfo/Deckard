module;
#include <cstdint>

export module Deckard;
export namespace Deckard
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 264;
	
	constexpr uint32_t version = major * 10000 + minor * 1000 + build;
	constexpr uint64_t random_seed = 0x16ccb1439e5462cc;
	
	constexpr char version_string[] = "v0.0.264";
	constexpr char build_time_string[] = "2023-10-26 21:55:16";
	constexpr char phrase[] = "countdown-rug-reviving";
	constexpr char calver[] = "2023.43.264";
	constexpr char uuid[] = "5A2C82EC-5B59-46D6-9A94-7E5EA462DDF5";

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
