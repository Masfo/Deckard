module;
#include <cstdint>

export module scbuild;
export namespace scbuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 119;
	
	constexpr uint32_t version = major * 10000 + minor * 1000 + build;
	constexpr uint64_t random_seed = 0xee8e2418b11e4590;
	
	constexpr char version_string[] = "v0.0.119";
	constexpr char build_time_string[] = "2024-04-30 15:58:10";
	constexpr char phrase[] = "tug-propeller-expediter";
	constexpr char calver[] = "2024.18.119";
	constexpr char uuid[] = "E37564AD-D5FB-454A-9E51-A4F1A21220F2";

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
