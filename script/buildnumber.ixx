module;
#include <cstdint>

export module scbuild;
export namespace scbuild
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t build = 65;
	
	constexpr uint32_t version = major * 1000 + minor * 100 + build;
	constexpr uint64_t random_seed = 0x7926ccabffb054cb;
	
	constexpr char version_string[] = "v0.0.65";
	constexpr char build_time_string[] = "2024-04-29 13:18:19";
	constexpr char phrase[] = "coeditor-corporate-carpool";
	constexpr char calver[] = "2024.18.65";
	constexpr char uuid[] = "C49D10BB-0CE9-462F-B9AF-28147EB21C93";

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
