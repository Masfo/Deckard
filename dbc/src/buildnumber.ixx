// AUTOMATICALLY GENERATED BY BUILDINC v0.2.363 TOOL
// LAST BUILD (CMAKE): 2024-05-15 10:16:23

module;
#include <cstdint>

export module dbc;
export namespace dbc::build
{
	 // You can modify major and minor
	constexpr uint32_t major = 0;
	constexpr uint32_t minor = 0;

	 // Do not modify these
	constexpr uint32_t patch = 2066;
	
	constexpr uint32_t version = major * 100000 + minor * 10000 + patch;
	constexpr uint64_t random_seed = 0x387446e10759cf13;
	
	constexpr char version_string[] = "v0.0.2066";
	constexpr char build_time_string[] = "2024-05-15 10:16:23";
	constexpr char phrase[] = "boring-spool-trimness";
	constexpr char calver[] = "2024.20.2066";
	constexpr char uuid[] = "94496F9C-9FAA-45DC-8525-DC3AE9C13519";

	// Copy paste to import to your project
	/*
		constexpr auto major = dbc::major;
		constexpr auto minor = dbc::minor;
		constexpr auto patch = dbc::patch;
		constexpr auto version = dbc::version;
		constexpr auto random_seed = dbc::random_seed;
			
		constexpr auto version_string = dbc::version_string;
		constexpr auto build_time_string = dbc::build_time_string;
		constexpr auto phrase = dbc::phrase;
		constexpr auto calver = dbc::calver;
		constexpr auto uuid = dbc::uuid;
	*/
}