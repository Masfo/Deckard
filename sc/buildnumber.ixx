module;
#include <cstdint>

export module scbuild;
export namespace scbuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 126;
	
	constexpr uint32_t version = major * 10000 + minor * 1000 + build;
	constexpr uint64_t random_seed = 0xac60df3bb4a6fb7b;
	
	constexpr char version_string[] = "v0.0.126";
	constexpr char build_time_string[] = "2024-04-30 21:06:11";
	constexpr char phrase[] = "sulphuric-book-twice";
	constexpr char calver[] = "2024.18.126";
	constexpr char uuid[] = "B2A996E4-ACE6-412F-98A1-B7CD792516BA";

	// Copy paste to import to your project
	/*
		constexpr auto major = scbuild::major;
		constexpr auto minor = scbuild::minor;
		constexpr auto build = scbuild::build;
		constexpr auto version = scbuild::version;
		constexpr auto random_seed = scbuild::random_seed;
			
		constexpr auto version_string = scbuild::version_string;
		constexpr auto build_time_string = scbuild::build_time_string;
		constexpr auto phrase = scbuild::phrase;
		constexpr auto calver = scbuild::calver;
		constexpr auto uuid = scbuild::uuid;
	*/
}
