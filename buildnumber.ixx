module;
#include <cstdint>

export module DeckardBuild;
export namespace DeckardBuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 1026;
	
	constexpr uint32_t version = major * 100000 + minor * 10000 + build;
	constexpr uint64_t random_seed = 0xbdf0e51cb66228e;
	
	constexpr char version_string[] = "v0.0.1026";
	constexpr char build_time_string[] = "2024-04-09 22:12:24";
	constexpr char phrase[] = "banana-outsider-cyclist";
	constexpr char calver[] = "2024.15.1026";
	constexpr char uuid[] = "B1A08780-1D96-4228-86B7-9C3557F5786C";

	// Copy paste to import to your project
	/*
		constexpr auto major = DeckardBuild::major;
		constexpr auto minor = DeckardBuild::minor;
		constexpr auto build = DeckardBuild::build;
		constexpr auto version = DeckardBuild::version;
		constexpr auto random_seed = DeckardBuild::random_seed;
			
		constexpr auto version_string = DeckardBuild::version_string;
		constexpr auto build_time_string = DeckardBuild::build_time_string;
		constexpr auto phrase = DeckardBuild::phrase;
		constexpr auto calver = DeckardBuild::calver;
		constexpr auto uuid = DeckardBuild::uuid;
	*/
}
