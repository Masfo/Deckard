module;
#include <cstdint>

export module DeckardBuild;

export namespace DeckardBuild
{
	// You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 1;

	// Do not modify these
	constexpr uint32_t build = 1268;

	constexpr uint32_t version     = major * 10'0000 + minor * 1'0000 + build;
	constexpr uint64_t random_seed = 0x9e55'04e1'4436'fa6a;

	constexpr char version_string[]    = "v0.0.1268";
	constexpr char build_time_string[] = "2024-04-30 21:06:09";
	constexpr char phrase[]            = "droplet-pedometer-spur";
	constexpr char calver[]            = "2024.18.1268";
	constexpr char uuid[]              = "AB1C68AD-E1D2-46D5-B20A-171C68B28AC9";

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
} // namespace DeckardBuild
