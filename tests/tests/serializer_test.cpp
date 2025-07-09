#include <catch2/catch_test_macros.hpp>


import std;

import deckard;
import deckard.types;
using namespace deckard;

using namespace std::string_view_literals;
using namespace std::string_literals;

using namespace deckard::bitbuffer;

TEST_CASE("serializer", "[serializer]")
{
	SECTION("init")
	{
		//
	}
}

TEST_CASE("bitwriter", "[bitwriter][serializer]")
{
	SECTION("init")
	{
		bitwriter writer;
		CHECK(writer.empty());
	}

	SECTION("manually aligned")
	{
		bitwriter writer;

		writer.write(0xAABB'CCDD);           // 32
		writer.write(0xEEFF'0011'2233'4455); // 96
		writer.write(2.0f);                  // 128
		writer.write(0b1111'0000, 8);        // 136
		writer.write("abcd");                // 168, 8*21

		std::array<u8, 21> correct{
		  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x40, 0x00, 0x00, 0x00, 0xF0, 0x61, 0x62, 0x63, 0x64};


		CHECK(writer.size() == 21 * 8); // 168
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}


	SECTION("one-bit offset from manual alignment")
	{
		bitwriter writer;

		writer.write(true);

		writer.write(0xAABB'CCDD);
		writer.write(0xEEFF'0011'2233'4455);
		writer.write(2.0f);
		writer.write(0b1111'0000, 8);
		writer.write("abcd"); // 168 + 8

		std::array<u8, 22> correct{0xD5, 0x5D, 0xE6, 0x6E, 0xF7, 0x7F, 0x80, 0x08, 0x91, 0x19, 0xA2,
								   0x2A, 0xA0, 0x00, 0x00, 0x00, 0x78, 0x30, 0xB1, 0x31, 0xB2, 0x00};


		CHECK(writer.size() == 22 * 8); // 176
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("automatic padding")
	{
		bitwriter writer(padding::yes);

		writer.write(0xAABB'CCDD);
		writer.write(0xEEFF'0011'2233'4455);
		writer.write(2.0f);


		writer.write(0b1111'0000, 8);

		writer.write("abcd");

		std::array<u8, 21> correct{
		  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x40, 0x00, 0x00, 0x00, 0xF0, 0x61, 0x62, 0x63, 0x64};


		CHECK(writer.size() == 21 * 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("automatic padding, one bit offset")
	{
		bitwriter writer(padding::yes);

		writer.write(true);
		writer.write(0xAABB'CCDD);
		writer.write(0xEEFF'0011'2233'4455);
		writer.write(2.0f);


		writer.write(0b1111'0000, 8);

		writer.write("abcd");

		std::array<u8, 22> correct{0x80, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33,
								   0x44, 0x55, 0x40, 0x00, 0x00, 0x00, 0xF0, 0x61, 0x62, 0x63, 0x64};


		CHECK(writer.size() == 22 * 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write bits")
	{
		bitwriter writer;

		writer.write(true);
		writer.write(false);
		writer.write(true);


		std::array<u8, 1> correct{
		  0xA0, // 0b10100000
		};

		CHECK(writer.size() == 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write floats")
	{
		bitwriter writer;

		writer.write(2.0f);
		writer.write(4.0);

		std::array<u8, 4 + 8> correct{
		  // 2.0f
		  0x40,
		  0x00,
		  0x00,
		  0x00,
		  // 4.0
		  0x40,
		  0x10,
		  0x00,
		  0x00,
		  0x00,
		  0x00,
		  0x00,
		  0x00,
		};

		CHECK(writer.size() == 12 * 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write array, full")
	{
		bitwriter writer;

		std::array<u8, 4> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);


		std::array<u8, 4> correct{0xFF, 0xAA, 0x11, 0x00};

		CHECK(writer.size() == 4 * 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write array, partial")
	{
		bitwriter writer;

		std::array<u8, 4> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr, 8 * 2);


		std::array<u8, 2> correct{0xFF, 0xAA};

		CHECK(writer.size() == 2 * 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write vector, full")
	{
		bitwriter writer;

		std::vector<u8> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);


		std::array<u8, 4> correct{0xFF, 0xAA, 0x11, 0x00};

		CHECK(writer.size() == 4 * 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write vector, partial")
	{
		bitwriter writer;

		std::vector<u8> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr, 2 * 8);


		std::array<u8, 2> correct{0xFF, 0xAA};

		CHECK(writer.size() == 2 * 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write stringview")
	{
		bitwriter writer;

		writer.write("abcd"sv);
		std::array<u8, 4> correct{0x61, 0x62, 0x63, 0x64};

		CHECK(writer.size() == 4 * 8);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write string")
	{
		bitwriter writer;

		writer.write("abcd"s);
		std::array<u8, 4> correct{0x61, 0x62, 0x63, 0x64};

		CHECK(writer.size() == 8 * 4);
		CHECK(std::ranges::equal(correct, writer.data()) == true);
	}
}

TEST_CASE("bitreader", "[bitreader][serializer]")
{
	SECTION("manual padding")
	{
		bitwriter writer;

		writer.write(0xAABB'CCDD);
		writer.write(0xEEFF'0011'2233'4455);
		writer.write(2.0f);


		writer.write(0b1111'0010, 8);

		writer.write("abcd");

		//
		bitreader reader(writer.data());


		CHECK(reader.read<u32>() == 0xAABB'CCDD);
		CHECK(reader.read<u64>() == 0xEEFF'0011'2233'4455);
		CHECK(reader.read<f32>() == 2.0f);
		CHECK(reader.read<u32>(8) == 0b1111'0010);
		CHECK(reader.read_string(4 * 8) == "abcd"sv);
		CHECK(reader.empty() == true);
	}

	SECTION("manual one-bit offset")
	{
		bitwriter writer;

		writer.write(true);

		writer.write(0xAABB'CCDD);
		writer.write(0xEEFF'0011'2233'4455);
		writer.write(2.0f);


		writer.write(0b1111'0010, 8);

		writer.write("abcd");

		//
		bitreader reader(writer.data());

		CHECK(reader.read<bool>() == true);
		CHECK(reader.read<u32>() == 0xAABB'CCDD);
		CHECK(reader.read<u64>() == 0xEEFF'0011'2233'4455);
		CHECK(reader.read<f32>() == 2.0f);
		CHECK(reader.read<u32>(8) == 0b1111'0010);
		CHECK(reader.read_string(4 * 8) == "abcd"sv);
		CHECK(reader.empty() == true);
	}

	SECTION("reading automatic padding")
	{
		bitwriter writer(padding::yes);


		writer.write(0xAABB'CCDD);
		writer.write(0xEEFF'0011'2233'4455);
		writer.write(2.0f);


		writer.write(0b1111'0010, 8);

		writer.write("abcd");

		//
		bitreader reader(writer.data(), padding::yes);

		CHECK(reader.read<u32>() == 0xAABB'CCDD);
		CHECK(reader.read<u64>() == 0xEEFF'0011'2233'4455);
		CHECK(reader.read<f32>() == 2.0f);
		CHECK(reader.read<u32>(8) == 0b1111'0010);
		CHECK(reader.read_string(4 * 8) == "abcd"sv);
		CHECK(reader.empty() == true);
	}


	SECTION("reading automatic padding, one bit-offset")
	{
		bitwriter writer(padding::yes);

		writer.write(true);
		writer.write(0xAABB'CCDD);
		writer.write(0xEEFF'0011'2233'4455);
		writer.write(2.0f);
		writer.write(4.0);


		writer.write(0b1111'0010, 8);

		writer.write("abcd");

		//
		bitreader reader(writer.data(), padding::yes);

		CHECK(reader.read<bool>() == true);

		CHECK(reader.read<u32>() == 0xAABB'CCDD);
		CHECK(reader.read<u64>() == 0xEEFF'0011'2233'4455);
		CHECK(reader.read<f32>() == 2.0f);
		CHECK(reader.read<f64>() == 4.0);

		CHECK(reader.read<u32>(8) == 0b1111'0010);
		CHECK(reader.read_string(4 * 8) == "abcd"sv);
		CHECK(reader.empty() == true);
	}

	SECTION("read bits")
	{
		bitwriter writer;

		writer.write(true);
		writer.write(false);
		writer.write(true);

		//
		bitreader reader(writer.data());

		CHECK(reader.read<bool>() == true);
		CHECK(reader.read<bool>() == false);
		CHECK(reader.read<bool>() == true);
		CHECK(reader.empty());
	}

	SECTION("read floats")
	{
		bitwriter writer;

		writer.write(2.0f);
		writer.write(4.0);

		//
		bitreader reader(writer.data());

		CHECK(reader.read<f32>() == 2.0f);
		CHECK(reader.read<f64>() == 4.0);
		CHECK(reader.empty());
	}

	SECTION("read array, full")
	{
		bitwriter writer;

		std::array<u8, 4> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);

		// read
		bitreader reader(writer.data());

		std::array<u8, 4> readme{};
		CHECK(readme[0] == 0);
		CHECK(readme[1] == 0);
		CHECK(readme[2] == 0);
		CHECK(readme[3] == 0);


		reader.read(readme);

		CHECK(readme[0] == 0xFF);
		CHECK(readme[1] == 0xAA);
		CHECK(readme[2] == 0x11);
		CHECK(readme[3] == 0x00);
		CHECK(reader.empty() == true);
	}

	SECTION("read array, partial")
	{
		bitwriter writer;

		std::array<u8, 4> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);

		// read
		bitreader reader(writer.data());

		std::array<u8, 4> readme{};
		CHECK(readme[0] == 0);
		CHECK(readme[1] == 0);
		CHECK(readme[2] == 0);
		CHECK(readme[3] == 0);


		reader.read(readme, 8 * 2);

		CHECK(readme[0] == 0xFF);
		CHECK(readme[1] == 0xAA);
		CHECK(readme[2] == 0x00);
		CHECK(readme[3] == 0x00);

		CHECK(reader.empty() == true);
	}

	SECTION("read vector")
	{
		bitwriter writer;

		std::vector<u8> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);

		// read
		bitreader reader(writer.data());

		std::vector<u8> readme{};
		CHECK(readme.empty());


		reader.read(readme, 4 * 8);

		CHECK(readme.size() == 4);
		CHECK(readme[0] == 0xFF);
		CHECK(readme[1] == 0xAA);
		CHECK(readme[2] == 0x11);
		CHECK(readme[3] == 0x00);

		CHECK(reader.empty() == true);
	}

	SECTION("read span")
	{
		bitwriter writer;

		std::vector<u8> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);

		// read
		bitreader reader(writer.data());

		std::vector<u8> readme{};
		readme.resize(4);


		reader.read(std::span{readme}, 4 * 8);

		CHECK(readme.size() == 4);
		CHECK(readme[0] == 0xFF);
		CHECK(readme[1] == 0xAA);
		CHECK(readme[2] == 0x11);
		CHECK(readme[3] == 0x00);

		CHECK(reader.empty() == true);
	}
}
