export module deckard.archive;

import std;
import deckard.types;
import deckard.utf8;
namespace fs = std::filesystem;

export namespace deckard::archive
{
	// name directory
	// 1. u8 bytecount
	// 2. utf8 string, bytecount length
	// 3. u64 offset in archive

	constexpr u8  archive_version = 0;
	constexpr u64 archive_magic = std::byteswap(0x4445'434B'4152'4400 | archive_version); // "DECKARD" + version byte

	static_assert(archive_version <= 255, "archive_version must be <= 255");

	struct archive_directory_entry
	{
		u64 hash{0};
		u64 offset{0};      // use ZSTD_isFrame to determine if compressed or not
		u32 size{0};
		u32 name_offset{0}; // offset into the string table
	};

	struct archive_string_table
	{
		std::vector<u8> data{};
	};

	struct archive_header
	{
		u64 magic{archive_magic};
	};

	struct archive_directory
	{
		std::vector<archive_directory_entry> entries{};
	};

	struct archive_footer
	{
		u64 directory_offset{};
		u64 magic{archive_magic};
	};

	struct archive_entry
	{
		u32 offset{};
		u32 uncompressed_size{0};
	};

	struct archive_data
	{
		std::vector<u8> compressed_data{};
	};


	class archivebuilder
	{
	private:
		std::vector<fs::path> files{};
	public:
	};


} // namespace deckard::archive
