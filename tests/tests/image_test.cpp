#include <catch2/catch_test_macros.hpp>


import std;
import deckard.image;
import deckard.zstd;

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

	SECTION("dif encode/decode")
	{
		SECTION("rgb_2x2")
		{
			// Create a simple 2x2 RGB image
			deckard::image_rgb img(2, 2);
			img.at(0, 0) = {10, 20, 30};
			img.at(1, 0) = {40, 50, 60};
			img.at(0, 1) = {70, 80, 90};
			img.at(1, 1) = {100, 110, 120};

			// Allocate buffer for encoding
			std::vector<deckard::u8> buffer;
			buffer.resize(1024); // More than enough for 2x2

			// Encode
			auto encoded = deckard::encode_dif_rgb(img, std::span{buffer.data(), buffer.size()});
			REQUIRE(encoded.has_value());
			CHECK(encoded.value() > 10); // Should be at least header size

			// Check DIF header
			CHECK(buffer[0] == 'D');
			CHECK(buffer[1] == 'I');
			CHECK(buffer[2] == 'F');
			CHECK(buffer[3] == '1');
			
			// Check width/height (little-endian)
			CHECK(buffer[4] == 2); // width low byte
			CHECK(buffer[5] == 0); // width high byte
			CHECK(buffer[6] == 2); // height low byte
			CHECK(buffer[7] == 0); // height high byte
			CHECK(buffer[8] == 3); // channels (RGB)

			// Decode
			auto decoded = deckard::decode_dif_rgb(std::span<const deckard::u8>{buffer.data(), encoded.value()});
			REQUIRE(decoded.has_value());
			CHECK(decoded->width() == 2);
			CHECK(decoded->height() == 2);

			// Verify pixel data matches
			CHECK(decoded->at(0, 0) == deckard::rgb{10, 20, 30});
			CHECK(decoded->at(1, 0) == deckard::rgb{40, 50, 60});
			CHECK(decoded->at(0, 1) == deckard::rgb{70, 80, 90});
			CHECK(decoded->at(1, 1) == deckard::rgb{100, 110, 120});
		}

		SECTION("rgba_2x2")
		{
			// Create a simple 2x2 RGBA image
			deckard::image_rgba img(2, 2);
			img.at(0, 0) = {10, 20, 30, 255};
			img.at(1, 0) = {40, 50, 60, 200};
			img.at(0, 1) = {70, 80, 90, 150};
			img.at(1, 1) = {100, 110, 120, 100};

			// Allocate buffer for encoding
			std::vector<deckard::u8> buffer;
			buffer.resize(1024);

			// Encode
			auto encoded = deckard::encode_dif_rgba(img, std::span{buffer.data(), buffer.size()});
			REQUIRE(encoded.has_value());
			CHECK(encoded.value() > 10);

			// Check DIF header
			CHECK(buffer[0] == 'D');
			CHECK(buffer[1] == 'I');
			CHECK(buffer[2] == 'F');
			CHECK(buffer[3] == '1');
			CHECK(buffer[8] == 4); // channels (RGBA)

			// Decode
			auto decoded = deckard::decode_dif_rgba(std::span<const deckard::u8>{buffer.data(), encoded.value()});
			REQUIRE(decoded.has_value());
			CHECK(decoded->width() == 2);
			CHECK(decoded->height() == 2);

			// Verify pixel data matches including alpha
			CHECK(decoded->at(0, 0) == deckard::rgba{10, 20, 30, 255});
			CHECK(decoded->at(1, 0) == deckard::rgba{40, 50, 60, 200});
			CHECK(decoded->at(0, 1) == deckard::rgba{70, 80, 90, 150});
			CHECK(decoded->at(1, 1) == deckard::rgba{100, 110, 120, 100});
		}

		SECTION("rgb_larger_image")
		{
			// Test with a larger image to verify compression
			deckard::image_rgb img(64, 64);
			for (deckard::u16 y = 0; y < 64; ++y)
			{
				for (deckard::u16 x = 0; x < 64; ++x)
				{
					img.at(x, y) = deckard::rgb{
						static_cast<deckard::u8>(x * 4),
						static_cast<deckard::u8>(y * 4),
						static_cast<deckard::u8>((x + y) * 2)
					};
				}
			}

			// Encode
			const deckard::u32 uncompressed_size = 64 * 64 * 3;
			std::vector<deckard::u8> buffer;
			buffer.resize(10 + deckard::zstd::bound(uncompressed_size));
			
			auto encoded = deckard::encode_dif_rgb(img, std::span{buffer.data(), buffer.size()});
			REQUIRE(encoded.has_value());
			
			// Compressed size should be less than uncompressed (with header)
			CHECK(encoded.value() < (10 + uncompressed_size));

			// Decode and verify
			auto decoded = deckard::decode_dif_rgb(std::span<const deckard::u8>{buffer.data(), encoded.value()});
			REQUIRE(decoded.has_value());
			CHECK(decoded->width() == 64);
			CHECK(decoded->height() == 64);

			// Spot check a few pixels
			CHECK(decoded->at(0, 0) == deckard::rgb{0, 0, 0});
			CHECK(decoded->at(32, 32) == deckard::rgb{128, 128, 128});
			CHECK(decoded->at(63, 63) == deckard::rgb{252, 252, 252});
		}

		SECTION("buffer_too_small")
		{
			deckard::image_rgb img(2, 2);
			img.at(0, 0) = {1, 2, 3};
			
			// Try with a buffer that's too small
			std::vector<deckard::u8> small_buffer(5);
			auto result = deckard::encode_dif_rgb(img, std::span{small_buffer.data(), small_buffer.size()});
			
			REQUIRE(not result.has_value());
			CHECK(result.error().find("buffer too small") != std::string::npos);
		}

		SECTION("decode_invalid_magic")
		{
			std::vector<deckard::u8> buffer(20, 0);
			buffer[0] = 'X'; // Wrong magic
			buffer[1] = 'Y';
			buffer[2] = 'Z';
			buffer[3] = '1';
			
			auto result = deckard::decode_dif_rgb(std::span{buffer.data(), buffer.size()});
			CHECK(not result.has_value());
		}

		SECTION("decode_wrong_channels")
		{
			// Create valid RGBA DIF file
			deckard::image_rgba rgba_img(2, 2);
			rgba_img.at(0, 0) = {1, 2, 3, 4};
			
			std::vector<deckard::u8> buffer(1024);
			auto encoded = deckard::encode_dif_rgba(rgba_img, std::span{buffer.data(), buffer.size()});
			REQUIRE(encoded.has_value());
			
			// Try to decode as RGB (should fail - wrong channel count)
			auto result = deckard::decode_dif_rgb(std::span<const deckard::u8>{buffer.data(), encoded.value()});
			CHECK(not result.has_value());
		}
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
