module;
#include <cstdint>

export module scbuild;
export namespace scbuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 29;
	
	constexpr uint32_t version = major * 1000 + minor * 100 + build;
	constexpr uint64_t random_seed = 0x87f99f2fcded4ea1;
	
	constexpr char version_string[] = "v0.0.29";
	constexpr char build_time_string[] = "2024-04-28 20:29:11";
	constexpr char phrase[] = "gab-derby-overbook";
	constexpr char calver[] = "2024.17.29";
	constexpr char uuid[] = "A39803F5-44ED-4208-839B-F722DB60B62A";

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
