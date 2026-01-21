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
