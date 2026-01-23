#include <catch2/catch_test_macros.hpp>


import std;
import deckard.image;

TEST_CASE("image", "[image]")
{
	SECTION("rgb_2x2")
	{
		//
		deckard::image_rgb img(2, 2);
		CHECK(img.width() == 2);
		CHECK(img.height() == 2);

		img[0, 0] = {255, 0, 0};
		img[1, 0] = {0, 255, 0};
		img[0, 1] = {0, 0, 255};
		img[1, 1] = {255, 255, 0};
		CHECK(img[0, 0] == deckard::rgb{255, 0, 0});
		CHECK(img[1, 0] == deckard::rgb{0, 255, 0});
		CHECK(img[0, 1] == deckard::rgb{0, 0, 255});
		CHECK(img[1, 1] == deckard::rgb{255, 255, 0});
	}

	SECTION("rgba_2x2")
	{
		//
		deckard::image_rgba img(2, 2);
		CHECK(img.width() == 2);
		CHECK(img.height() == 2);

		img[0, 0] = {255, 0, 0, 0};
		img[1, 0] = {0, 255, 0, 64};
		img[0, 1] = {0, 0, 255, 128};
		img[1, 1] = {255, 255, 0, 255};
		CHECK(img[0, 0] == deckard::rgba{255, 0, 0, 0});
		CHECK(img[1, 0] == deckard::rgba{0, 255, 0, 64});
		CHECK(img[0, 1] == deckard::rgba{0, 0, 255, 128});
		CHECK(img[1, 1] == deckard::rgba{255, 255, 0, 255});
	}

	SECTION("encode_bmp_2x2")
	{
		deckard::image_rgb img(2, 2);
		img[0, 0] = {1, 2, 3};
		img[1, 0] = {4, 5, 6};
		img[0, 1] = {7, 8, 9};
		img[1, 1] = {10, 11, 12};

		constexpr size_t header_bytes = 14u + 40u;
		constexpr size_t pixel_offset = header_bytes;
		constexpr size_t row_stride   = 2u * 3u;
		constexpr size_t row_padding  = (4u - (row_stride % 4u)) % 4u;
		constexpr size_t row_bytes    = row_stride + row_padding;
		constexpr size_t pixel_bytes  = row_bytes * 2u;
		constexpr size_t file_bytes   = header_bytes + pixel_bytes;

		std::vector<deckard::u8> out;
		out.resize(file_bytes);
		auto res = deckard::encode_bmp(img, out);
		CHECK(res.has_value());
		CHECK(*res == true);

		CHECK(out[0] == static_cast<deckard::u8>('B'));
		CHECK(out[1] == static_cast<deckard::u8>('M'));
		CHECK(out[10] == static_cast<deckard::u8>(pixel_offset & 0xFFu));
		CHECK(out[11] == static_cast<deckard::u8>((pixel_offset >> 8) & 0xFFu));
		CHECK(out[28] == 24); // 

		CHECK(out[pixel_offset + 0] == 9);  
		CHECK(out[pixel_offset + 1] == 8);  
		CHECK(out[pixel_offset + 2] == 7);  
		CHECK(out[pixel_offset + 3] == 12); 
		CHECK(out[pixel_offset + 4] == 11);
		CHECK(out[pixel_offset + 5] == 10);

		const size_t row0 = pixel_offset + row_bytes;
		CHECK(out[row0 + 0] == 3);
		CHECK(out[row0 + 1] == 2);
		CHECK(out[row0 + 2] == 1);
		CHECK(out[row0 + 3] == 6);
		CHECK(out[row0 + 4] == 5);
		CHECK(out[row0 + 5] == 4);

		auto decoded = deckard::decode_bmp(out);
		REQUIRE(decoded.has_value());
		CHECK(decoded->width() == 2);
		CHECK(decoded->height() == 2);
		CHECK((*decoded)[0, 0] == deckard::rgb{1, 2, 3});
		CHECK((*decoded)[1, 0] == deckard::rgb{4, 5, 6});
		CHECK((*decoded)[0, 1] == deckard::rgb{7, 8, 9});
		CHECK((*decoded)[1, 1] == deckard::rgb{10, 11, 12});
	}
	SECTION("rgb")
	{
		deckard::image_rgb img(4, 3);
		CHECK(img.width() == 4);
		CHECK(img.height() == 3);

		for (deckard::u16 y = 0; y < img.height(); ++y)
		{
			for (deckard::u16 x = 0; x < img.width(); ++x)
			{
				deckard::rgb p;
				p.r       = static_cast<deckard::u8>(x);
				p.g       = static_cast<deckard::u8>(y);
				p.b       = static_cast<deckard::u8>(x + y);
				img[x, y] = p;
			}
		}

		for (deckard::u16 y = 0; y < img.height(); ++y)
		{
			for (deckard::u16 x = 0; x < img.width(); ++x)
			{
				CHECK(img[x, y] ==
					  deckard::rgb{
						static_cast<deckard::u8>(x),
						static_cast<deckard::u8>(y),
						static_cast<deckard::u8>(x + y),
					  });
			}
		}

		img[1, 2] = deckard::rgb{1, 2, 3};

		CHECK(img[1, 2] == deckard::rgb{1, 2, 3});
	}

	SECTION("rgba")
	{
		deckard::image_rgba img(2, 2);
		CHECK(img.width() == 2);
		CHECK(img.height() == 2);

		for (deckard::u16 y = 0; y < img.height(); ++y)
		{
			for (deckard::u16 x = 0; x < img.width(); ++x)
			{
				deckard::rgba p;
				p.r       = static_cast<deckard::u8>(x * 10);
				p.g       = static_cast<deckard::u8>(y * 10);
				p.b       = static_cast<deckard::u8>(x + y);
				p.a       = static_cast<deckard::u8>(255 - (x + y));
				img[x, y] = p;
			}
		}

		for (deckard::u16 y = 0; y < img.height(); ++y)
		{
			for (deckard::u16 x = 0; x < img.width(); ++x)
			{
				CHECK(img[x, y] ==
					  deckard::rgba{
						static_cast<deckard::u8>(x * 10),
						static_cast<deckard::u8>(y * 10),
						static_cast<deckard::u8>(x + y),
						static_cast<deckard::u8>(255 - (x + y)),
					  });
			}
		}

		deckard::rgba c;
		c.r       = 10;
		c.g       = 20;
		c.b       = 30;
		c.a       = 40;
		img[0, 1] = c;

		CHECK(img[0, 1] == deckard::rgba{10, 20, 30, 40});
	}
}
