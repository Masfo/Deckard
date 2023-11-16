module;
#include <cstdint>

export module DeckardBuild;
export namespace DeckardBuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 285;
	
	constexpr uint32_t version = major * 10000 + minor * 1000 + build;
	constexpr uint64_t random_seed = 0xa4a6fb3218aaa1dc;
	
	constexpr char version_string[] = "v0.0.285";
	constexpr char build_time_string[] = "2023-11-16 16:52:52";
	constexpr char phrase[] = "unstopped-hunter-cautious";
	constexpr char calver[] = "2023.46.285";
	constexpr char uuid[] = "7FABC37A-2638-4F64-BFD4-C6EEEF1844AE";

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
