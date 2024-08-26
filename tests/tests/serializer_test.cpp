#include <catch2/catch_test_macros.hpp>


import std;

import deckard;
import deckard.types;
using namespace deckard;

using namespace std::string_view_literals;
using namespace std::string_literals;

using namespace deckard::serializer;

TEST_CASE("bitwriter", "[bitwriter][serializer]")
{
	SECTION("init")
	{
		bitwriter writer;
		REQUIRE(writer.empty());
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


		REQUIRE(writer.size() == 21 * 8); // 168
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
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


		REQUIRE(writer.size() == 22 * 8); // 176
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
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


		REQUIRE(writer.size() == 21 * 8);
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
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


		REQUIRE(writer.size() == 22 * 8);
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write array, full")
	{
		bitwriter writer;

		std::array<u8, 4> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);


		std::array<u8, 4> correct{0xFF, 0xAA, 0x11, 0x00};

		REQUIRE(writer.size() == 4 * 8);
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write array, partial")
	{
		bitwriter writer;

		std::array<u8, 4> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr, 8 * 2);


		std::array<u8, 2> correct{0xFF, 0xAA};

		REQUIRE(writer.size() == 2 * 8);
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write vector, full")
	{
		bitwriter writer;

		std::vector<u8> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);


		std::array<u8, 4> correct{0xFF, 0xAA, 0x11, 0x00};

		REQUIRE(writer.size() == 4 * 8);
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write vector, partial")
	{
		bitwriter writer;

		std::vector<u8> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr, 2 * 8);


		std::array<u8, 2> correct{0xFF, 0xAA};

		REQUIRE(writer.size() == 2 * 8);
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write stringview")
	{
		bitwriter writer;

		writer.write("abcd"sv);
		std::array<u8, 4> correct{0x61, 0x62, 0x63, 0x64};

		REQUIRE(writer.size() == 4 * 8);
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
	}

	SECTION("write string")
	{
		bitwriter writer;

		writer.write("abcd"s);
		std::array<u8, 4> correct{0x61, 0x62, 0x63, 0x64};

		REQUIRE(writer.size() == 8 * 4);
		REQUIRE(std::ranges::equal(correct, writer.data()) == true);
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


		REQUIRE(reader.read<u32>() == 0xAABB'CCDD);
		REQUIRE(reader.read<u64>() == 0xEEFF'0011'2233'4455);
		REQUIRE(reader.read<f32>() == 2.0f);
		REQUIRE(reader.read<u32>(8) == 0b1111'0010);
		REQUIRE(reader.read_string(4 * 8) == "abcd"sv);
		REQUIRE(reader.empty() == true);
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

		REQUIRE(reader.read<bool>() == true);
		REQUIRE(reader.read<u32>() == 0xAABB'CCDD);
		REQUIRE(reader.read<u64>() == 0xEEFF'0011'2233'4455);
		REQUIRE(reader.read<f32>() == 2.0f);
		REQUIRE(reader.read<u32>(8) == 0b1111'0010);
		REQUIRE(reader.read_string(4 * 8) == "abcd"sv);
		REQUIRE(reader.empty() == true);
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

		REQUIRE(reader.read<u32>() == 0xAABB'CCDD);
		REQUIRE(reader.read<u64>() == 0xEEFF'0011'2233'4455);
		REQUIRE(reader.read<f32>() == 2.0f);
		REQUIRE(reader.read<u32>(8) == 0b1111'0010);
		REQUIRE(reader.read_string(4 * 8) == "abcd"sv);
		REQUIRE(reader.empty() == true);
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

		REQUIRE(reader.read<bool>() == true);

		REQUIRE(reader.read<u32>() == 0xAABB'CCDD);
		REQUIRE(reader.read<u64>() == 0xEEFF'0011'2233'4455);
		REQUIRE(reader.read<f32>() == 2.0f);
		REQUIRE(reader.read<f64>() == 4.0);

		REQUIRE(reader.read<u32>(8) == 0b1111'0010);
		REQUIRE(reader.read_string(4 * 8) == "abcd"sv);
		REQUIRE(reader.empty() == true);
	}

	SECTION("read array, full")
	{
		bitwriter writer;

		std::array<u8, 4> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);

		// read
		bitreader reader(writer.data());

		std::array<u8, 4> readme{};
		REQUIRE(readme[0] == 0);
		REQUIRE(readme[1] == 0);
		REQUIRE(readme[2] == 0);
		REQUIRE(readme[3] == 0);


		reader.read(readme);

		REQUIRE(readme[0] == 0xFF);
		REQUIRE(readme[1] == 0xAA);
		REQUIRE(readme[2] == 0x11);
		REQUIRE(readme[3] == 0x00);
		REQUIRE(reader.empty() == true);
	}

	SECTION("read array, partial")
	{
		bitwriter writer;

		std::array<u8, 4> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);

		// read
		bitreader reader(writer.data());

		std::array<u8, 4> readme{};
		REQUIRE(readme[0] == 0);
		REQUIRE(readme[1] == 0);
		REQUIRE(readme[2] == 0);
		REQUIRE(readme[3] == 0);


		reader.read(readme, 8 * 2);

		REQUIRE(readme[0] == 0xFF);
		REQUIRE(readme[1] == 0xAA);
		REQUIRE(readme[2] == 0x00);
		REQUIRE(readme[3] == 0x00);

		REQUIRE(reader.empty() == true);
	}

	SECTION("read vector")
	{
		bitwriter writer;

		std::vector<u8> arr{0xFF, 0xAA, 0x11, 0x00};

		writer.write(arr);

		// read
		bitreader reader(writer.data());

		std::vector<u8> readme{};
		REQUIRE(readme.empty());


		reader.read(readme, 4 * 8);

		REQUIRE(readme.size() == 4);
		REQUIRE(readme[0] == 0xFF);
		REQUIRE(readme[1] == 0xAA);
		REQUIRE(readme[2] == 0x11);
		REQUIRE(readme[3] == 0x00);

		REQUIRE(reader.empty() == true);
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

		REQUIRE(readme.size() == 4);
		REQUIRE(readme[0] == 0xFF);
		REQUIRE(readme[1] == 0xAA);
		REQUIRE(readme[2] == 0x11);
		REQUIRE(readme[3] == 0x00);

		REQUIRE(reader.empty() == true);
	}
}
