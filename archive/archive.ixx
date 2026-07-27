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


	struct archive_directory_entry
	{
		utf8::string name{};
		u64             offset{};
	};

	struct archive_header
	{
		std::array<u8, 4> magic{'D', 'A', 'R', 0};
		u64                 entry_count{};
		u64                 directory_offset{};
	};

	struct archive_entry
	{
		u32                  offset{};
		u32                  uncompressed_size{0};
		u32                  compressed_size{0}; // zstd
	};

	struct archive_data
	{
		std::vector<u8> compressed_data{};
	};


} // namespace deckard::archive
