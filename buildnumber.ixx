module;
#include <cstdint>

export module DeckardBuild;
export namespace DeckardBuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 1266;
	
	constexpr uint32_t version = major * 100000 + minor * 10000 + build;
	constexpr uint64_t random_seed = 0x6fca24830ae4b963;
	
	constexpr char version_string[] = "v0.0.1266";
	constexpr char build_time_string[] = "2024-04-30 15:49:29";
	constexpr char phrase[] = "canteen-acclaim-conjuror";
	constexpr char calver[] = "2024.18.1266";
	constexpr char uuid[] = "8CEAE6C1-3217-476E-8EA1-1FB19A885DAF";

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
