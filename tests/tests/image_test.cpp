#include <catch2/catch_test_macros.hpp>


import std;
import deckard.image;
import deckard.zstd;
import deckard.types;

TEST_CASE("tga loader", "[image][tga]")
{
	using namespace deckard;
	std::array<u8, 80> tga_3x3 = {
	  0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00,
	  0x20, 0x08, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x40, 0x40,
	  0x40, 0x00, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF,
	  0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x52,
	  0x55, 0x45, 0x56, 0x49, 0x53, 0x49, 0x4F, 0x4E, 0x2D, 0x58, 0x46, 0x49, 0x4C, 0x45, 0x2E, 0x00};

	SECTION("load tga 3x3, as rgb")
	{
		auto result = image::decode_tga<image_rgb>(tga_3x3);
		REQUIRE(result.has_value());
		auto img = result.value();
		REQUIRE(img.width() == 3);
		REQUIRE(img.height() == 3);
		REQUIRE(img.channels() == 3);


	}

	SECTION("load tga 3x3, as rgba")
	{
		auto result = image::decode_tga<image_rgba>(tga_3x3);
		REQUIRE(result.has_value());
		auto img = result.value();
		REQUIRE(img.width() == 3);
		REQUIRE(img.height() == 3);
		REQUIRE(img.channels() == 4);

	}

	SECTION("load tga 3x3 rgba (32-bit, alpha preserved)")
	{
		// tga_3x3 is a 32-bit BGRA image, bottom-left origin (first stored row is the bottom row)
		auto result = image::decode_tga<image_rgba>(tga_3x3);
		REQUIRE(result.has_value());
		auto& img = result.value();
		REQUIRE(img.width() == 3);
		REQUIRE(img.height() == 3);

		// top row
		CHECK(img[0, 0] == rgba{255, 0, 0, 255});     // red
		CHECK(img[1, 0] == rgba{0, 255, 0, 255});     // green
		CHECK(img[2, 0] == rgba{0, 0, 255, 255});     // blue

		// middle row (exercises partial/zero alpha)
		CHECK(img[0, 1] == rgba{0x40, 0x40, 0x40, 0}); // transparent gray
		CHECK(img[1, 1] == rgba{255, 0, 0, 128});     // semi-transparent red
		CHECK(img[2, 1] == rgba{255, 0, 0, 0});       // fully transparent red

		// bottom row
		CHECK(img[0, 2] == rgba{255, 255, 0, 255});   // yellow
		CHECK(img[1, 2] == rgba{0, 255, 255, 255});   // cyan
		CHECK(img[2, 2] == rgba{255, 0, 255, 255});   // magenta

		// decoding the same 32-bit file as image_rgb must drop alpha
		auto rgb_result = image::decode_tga<image_rgb>(tga_3x3);
		REQUIRE(rgb_result.has_value());
		CHECK((*rgb_result)[1, 1] == rgb{255, 0, 0});
		CHECK((*rgb_result)[0, 1] == rgb{0x40, 0x40, 0x40});

		// the default template argument is image_rgba, so alpha survives an unqualified load
		// (this is what image::load_tga with no explicit type returns)
		auto default_result = image::decode_tga(tga_3x3);
		REQUIRE(default_result.has_value());
		CHECK((*default_result)[1, 1] == rgba{255, 0, 0, 128});
	}

	SECTION("tga rgba 32-bit round trip")
	{
		image_rgba img(2, 2);
		img[0, 0] = {10, 20, 30, 255};
		img[1, 0] = {40, 50, 60, 200};
		img[0, 1] = {70, 80, 90, 150};
		img[1, 1] = {100, 110, 120, 100};

		auto encoded = image::encode_tga(img);
		REQUIRE(encoded.has_value());
		const auto& buffer = *encoded;
		REQUIRE(buffer.size() == 18u + 2u * 2u * 4u);

		CHECK(buffer[2] == 2);     // image type (uncompressed true-color)
		CHECK(buffer[16] == 32);   // pixel depth (BGRA)
		CHECK(buffer[17] == 0x08); // descriptor (bottom-left origin, 8 alpha bits)

		auto decoded
		  = image::decode_tga<image_rgba>(std::span<const u8>{buffer.data(), buffer.size()});
		REQUIRE(decoded.has_value());
		CHECK(decoded->width() == 2);
		CHECK(decoded->height() == 2);
		CHECK(*decoded == img);
	}

	SECTION("tga rgb 24-bit round trip")
	{
		image_rgb img(2, 2);
		img[0, 0] = {1, 2, 3};
		img[1, 0] = {4, 5, 6};
		img[0, 1] = {7, 8, 9};
		img[1, 1] = {10, 11, 12};

		auto encoded = image::encode_tga(img);
		REQUIRE(encoded.has_value());
		const auto& buffer = *encoded;
		REQUIRE(buffer.size() == 18u + 2u * 2u * 3u);

		CHECK(buffer[16] == 24);   // pixel depth (BGR)
		CHECK(buffer[17] == 0x00); // descriptor (bottom-left origin, no alpha)

		auto decoded = image::decode_tga<image_rgb>(std::span<const u8>{buffer.data(), buffer.size()});
		REQUIRE(decoded.has_value());
		CHECK(*decoded == img);

		// re-decoding as rgba fills alpha with 255
		auto as_rgba = image::decode_tga<image_rgba>(std::span<const u8>{buffer.data(), buffer.size()});
		REQUIRE(as_rgba.has_value());
		CHECK((*as_rgba)[0, 0] == rgba{1, 2, 3, 255});
	}
}

TEST_CASE("png loader", "[image][png]")
{
	using namespace deckard;

	std::array<u8, 84> png_3x3 = {
	  0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00,
	  0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x08, 0x06, 0x00, 0x00, 0x00, 0x56, 0x28, 0xB5, 0xBF, 0x00,
	  0x00, 0x00, 0x1B, 0x49, 0x44, 0x41, 0x54, 0x78, 0x01, 0x15, 0xC1, 0x81, 0x01, 0x00, 0x00, 0x04, 0xC0,
	  0xA0, 0x4E, 0xF7, 0xF9, 0x50, 0x22, 0x89, 0xF2, 0x62, 0xBC, 0x92, 0x8E, 0x5A, 0x16, 0x28, 0x10, 0x71,
	  0xF8, 0xF3, 0xEA, 0x4A, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};


	std::array<u8, 145> invalid = {
	  0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00,
	  0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0xCC, 0x68, 0xA6, 0x62, 0x00, 0x00, 0x00, 0x04, 0x67,
	  0x41, 0x4D, 0x41, 0x00, 0x01, 0x86, 0xA0, 0x31, 0xE8, 0x96, 0x5F, 0x00, 0x00, 0x00, 0x48, 0x49, 0x44, 0x41, 0x54,
	  0x78, 0x9C, 0xED, 0xD5, 0xC1, 0x09, 0x00, 0x30, 0x0C, 0x02, 0x40, 0x85, 0xEC, 0x91, 0xFD, 0xB7, 0x72, 0x13, 0x3B,
	  0x44, 0x2B, 0xF4, 0xA1, 0xF8, 0xCE, 0xE1, 0x2B, 0xB4, 0x0D, 0x04, 0x3B, 0x80, 0x0A, 0x14, 0xF8, 0x1C, 0xA0, 0xED,
	  0xE4, 0x7D, 0x4C, 0x78, 0x40, 0x81, 0x02, 0x0F, 0x4A, 0x87, 0x1F, 0xC2, 0x84, 0x07, 0x14, 0x28, 0xF0, 0xA0, 0x74,
	  0x38, 0x23, 0xA9, 0x40, 0x81, 0xBB, 0x70, 0x77, 0xA3, 0xC0, 0x01, 0x82, 0xB1, 0xF9, 0x5E, 0x0F, 0x40, 0xCF, 0x4B,
	  0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};


	SECTION("load_png")
	{
		auto result = deckard::image::load_png(png_3x3);
		REQUIRE(result);
		CHECK(result->width() == 3);
		CHECK(result->height() == 3);
		CHECK((*result)[0, 0] == deckard::rgba{255, 0, 0, 255});   // red
		CHECK((*result)[1, 0] == deckard::rgba{0, 255, 0, 255});   // green
		CHECK((*result)[2, 0] == deckard::rgba{0, 0, 255, 255});   // blue

		CHECK((*result)[0, 1] == deckard::rgba{0, 0, 0, 0});       // transparent
		CHECK((*result)[1, 1] == deckard::rgba{255, 0, 0, 128});   // semi-transparent red
		CHECK((*result)[2, 1] == deckard::rgba{0, 0, 0, 0});       // transparent

		CHECK((*result)[0, 2] == deckard::rgba{255, 255, 0, 255}); // yellow
		CHECK((*result)[1, 2] == deckard::rgba{0, 255, 255, 255}); // cyan
		CHECK((*result)[2, 2] == deckard::rgba{255, 0, 255, 255}); // magenta
	}

	SECTION("load_png_invalid")
	{
		auto result = deckard::image::load_png(invalid);
		REQUIRE(not result);
	}
}

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

	SECTION("row")
	{
		deckard::image_rgba img(2, 2);
		CHECK(img.width() == 2);
		CHECK(img.height() == 2);

		img[0, 0] = {255, 0, 0, 0};
		img[1, 0] = {0, 255, 0, 64};
		img[0, 1] = {0, 0, 255, 128};
		img[1, 1] = {255, 255, 0, 255};

		auto row0 = img.row(0);
		CHECK(row0[0] == deckard::rgba{255, 0, 0, 0});
		CHECK(row0[1] == deckard::rgba{0, 255, 0, 64});

		row0[0] = deckard::rgba{1, 2, 3, 4};
		CHECK(row0[0] == deckard::rgba{1, 2, 3, 4});
		CHECK(row0[1] == deckard::rgba{0, 255, 0, 64});

		auto row1 = img.row(1);
		CHECK(row1[0] == deckard::rgba{0, 0, 255, 128});
		CHECK(row1[1] == deckard::rgba{255, 255, 0, 255});


		auto row0_bytes = img.row_bytes(0);
		CHECK(row0_bytes[0] == 1);
		CHECK(row0_bytes[1] == 2);
		CHECK(row0_bytes[2] == 3);
		CHECK(row0_bytes[3] == 4);
		CHECK(row0_bytes[4] == 0);
		CHECK(row0_bytes[5] == 255);
		CHECK(row0_bytes[6] == 0);
		CHECK(row0_bytes[7] == 64);

		auto row1_bytes = img.row_bytes(1);
		CHECK(row1_bytes[0] == 0);
		CHECK(row1_bytes[1] == 0);
		CHECK(row1_bytes[2] == 255);
		CHECK(row1_bytes[3] == 128);
		CHECK(row1_bytes[4] == 255);
		CHECK(row1_bytes[5] == 255);
		CHECK(row1_bytes[6] == 0);
		CHECK(row1_bytes[7] == 255);


		row0_bytes[0] = 64;
		CHECK(img[0, 0] == deckard::rgba{64, 2, 3, 4});
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

		auto encoded = deckard::image::encode_bmp<deckard::image_rgb>(img);
		REQUIRE(encoded.has_value());
		const std::vector<deckard::u8>& out = *encoded;
		REQUIRE(out.size() == file_bytes);

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

		auto decoded = deckard::image::decode_bmp<deckard::image_rgb>(out);
		REQUIRE(decoded.has_value());
		CHECK(decoded->width() == 2);
		CHECK(decoded->height() == 2);
		CHECK((*decoded)[0, 0] == deckard::rgb{1, 2, 3});
		CHECK((*decoded)[1, 0] == deckard::rgb{4, 5, 6});
		CHECK((*decoded)[0, 1] == deckard::rgb{7, 8, 9});
		CHECK((*decoded)[1, 1] == deckard::rgb{10, 11, 12});
	}

	SECTION("encode_bmp_2x2 rgba (32-bit)")
	{
		deckard::image_rgba img(2, 2);
		img[0, 0] = {1, 2, 3, 4};
		img[1, 0] = {4, 5, 6, 7};
		img[0, 1] = {7, 8, 9, 10};
		img[1, 1] = {10, 11, 12, 13};

		constexpr size_t header_bytes = 14u + 40u;
		constexpr size_t pixel_offset = header_bytes;
		constexpr size_t row_bytes    = 2u * 4u; // 32-bit: no padding
		constexpr size_t file_bytes   = header_bytes + row_bytes * 2u;

		auto encoded = deckard::image::encode_bmp(img);
		REQUIRE(encoded.has_value());
		const std::vector<deckard::u8>& out = *encoded;
		REQUIRE(out.size() == file_bytes);

		CHECK(out[0] == static_cast<deckard::u8>('B'));
		CHECK(out[1] == static_cast<deckard::u8>('M'));
		CHECK(out[28] == 32); // bits per pixel (BGRA)

		// bottom-up: first stored row is image row 1
		CHECK(out[pixel_offset + 0] == 9);
		CHECK(out[pixel_offset + 1] == 8);
		CHECK(out[pixel_offset + 2] == 7);
		CHECK(out[pixel_offset + 3] == 10);

		auto decoded = deckard::image::decode_bmp<deckard::image_rgba>(out);
		REQUIRE(decoded.has_value());
		CHECK(decoded->width() == 2);
		CHECK(decoded->height() == 2);
		CHECK((*decoded)[0, 0] == deckard::rgba{1, 2, 3, 4});
		CHECK((*decoded)[1, 0] == deckard::rgba{4, 5, 6, 7});
		CHECK((*decoded)[0, 1] == deckard::rgba{7, 8, 9, 10});
		CHECK((*decoded)[1, 1] == deckard::rgba{10, 11, 12, 13});

		// decoding the 32-bit result as rgb drops alpha
		auto as_rgb = deckard::image::decode_bmp<deckard::image_rgb>(out);
		REQUIRE(as_rgb.has_value());
		CHECK((*as_rgb)[0, 0] == deckard::rgb{1, 2, 3});
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
			auto encoded = deckard::image::encode_dif_rgb(img, std::span{buffer.data(), buffer.size()});
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
			auto decoded = deckard::image::decode_dif_rgb(std::span<const deckard::u8>{buffer.data(), encoded.value()});
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
			auto encoded = deckard::image::encode_dif_rgba(img, std::span{buffer.data(), buffer.size()});
			REQUIRE(encoded.has_value());
			CHECK(encoded.value() > 10);

			// Check DIF header
			CHECK(buffer[0] == 'D');
			CHECK(buffer[1] == 'I');
			CHECK(buffer[2] == 'F');
			CHECK(buffer[3] == '1');
			CHECK(buffer[8] == 4); // channels (RGBA)

			// Decode
			auto decoded = deckard::image::decode_dif_rgba(std::span<const deckard::u8>{buffer.data(), encoded.value()});
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
					  static_cast<deckard::u8>((x + y) * 2)};
				}
			}

			// Encode
			const deckard::u32       uncompressed_size = 64 * 64 * 3;
			std::vector<deckard::u8> buffer;
			buffer.resize(10 + deckard::zstd::bound(uncompressed_size));

			auto encoded = deckard::image::encode_dif_rgb(img, std::span{buffer.data(), buffer.size()});
			REQUIRE(encoded.has_value());

			// Compressed size should be less than uncompressed (with header)
			CHECK(encoded.value() < (10 + uncompressed_size));

			// Decode and verify
			auto decoded = deckard::image::decode_dif_rgb(std::span<const deckard::u8>{buffer.data(), encoded.value()});
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
			auto result = deckard::image::encode_dif_rgb(img, std::span{small_buffer.data(), small_buffer.size()});

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

			auto result = deckard::image::decode_dif_rgb(std::span{buffer.data(), buffer.size()});
			CHECK(not result.has_value());
		}

		SECTION("decode_wrong_channels")
		{
			// Create valid RGBA DIF file
			deckard::image_rgba rgba_img(2, 2);
			rgba_img.at(0, 0) = {1, 2, 3, 4};

			std::vector<deckard::u8> buffer(1024);
			auto encoded = deckard::image::encode_dif_rgba(rgba_img, std::span{buffer.data(), buffer.size()});
			REQUIRE(encoded.has_value());

			// Try to decode as RGB (should fail - wrong channel count)
			auto result = deckard::image::decode_dif_rgb(std::span<const deckard::u8>{buffer.data(), encoded.value()});
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
				CHECK(img[x, y]
					  == deckard::rgb{
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
				CHECK(img[x, y]
					  == deckard::rgba{
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
