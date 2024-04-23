module;
#include <cstdint>

export module DeckardBuild;
export namespace DeckardBuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 1156;
	
	constexpr uint32_t version = major * 100000 + minor * 10000 + build;
	constexpr uint64_t random_seed = 0xb8b0e4cc54d470c0;
	
	constexpr char version_string[] = "v0.0.1156";
	constexpr char build_time_string[] = "2024-04-23 15:21:17";
	constexpr char phrase[] = "limping-shingle-enforcer";
	constexpr char calver[] = "2024.17.1156";
	constexpr char uuid[] = "556A927B-B0B0-46D4-A3A0-05E4D8B2DDC5";

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
