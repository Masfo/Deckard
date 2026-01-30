export module deckard.archive;

import std;
import deckard.types;
namespace fs = std::filesystem;

export namespace deckard::archive
{
	enum class archive_type : u8
	{
		image = 0,
		sound = 1,
		model = 2,
		text  = 3,

		other = 255
	};

	struct archive_image_struct
	{
		u16 width{0};
		u16 height{0};
		u8  channels{0}; // 3 = rgb, 4 = rgba
	};

	struct archive_data
	{
		u32         uncompressed_size{0};
		std::vector<u8> compressed_data{};
	};

	struct DeckardArchive
	{
		std::array<u8, 4> magic{'D', 'A', 'R', '0'};
		archive_type      type{archive_type::other};

		union
		{
			archive_image_struct image;
			// future: sound, model, text
			std::array<u8, 8> reserved{};
		} info;

		archive_data data{};

	};

} // namespace deckard::archive
