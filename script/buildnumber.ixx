module;
#include <cstdint>

export module scbuild;
export namespace scbuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 52;
	
	constexpr uint32_t version = major * 1000 + minor * 100 + build;
	constexpr uint64_t random_seed = 0xe0c57b4801f30178;
	
	constexpr char version_string[] = "v0.0.52";
	constexpr char build_time_string[] = "2024-04-28 22:43:26";
	constexpr char phrase[] = "harmless-jolt-ambulance";
	constexpr char calver[] = "2024.17.52";
	constexpr char uuid[] = "35ABC94C-D1F9-4CCC-99C6-83A081DF105E";

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
