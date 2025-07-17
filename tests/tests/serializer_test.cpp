#include <catch2/catch_test_macros.hpp>


import std;

import deckard;
import deckard.types;
import deckard.serializer;

using namespace deckard;

using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace deckard::bitbuffer;

// TODO: remove old bitreader/bitwriter


TEST_CASE("serializer", "[serializer]")
{

	SECTION("init")
	{
		serializer s;
		CHECK(s.empty());
		CHECK(s.size() == 0);
	}

	SECTION("init w/o padding")
	{
		serializer s(padding::no);
		CHECK(s.empty());
		CHECK(s.size() == 0);
	}

	SECTION("read/write u8")
	{
		serializer s;
		s.write<u8>(0xAB);
		s.write<u8>(0xCD);

		CHECK(s.size() == 2);
		CHECK(s.size_in_bits() == 16);
		CHECK(s.data()[0] == 0xAB);
		CHECK(s.data()[1] == 0xCD);

		s.rewind();

		CHECK(0xAB == s.read<u8>());
		CHECK(0xCD == s.read<u8>());
	}
	SECTION("read/write u8 q/o padding")
	{
		serializer s(padding::no);
		s.write<u8>(0xAB);
		s.write<bool>(true);
		s.write<u8>(0xCD);
		CHECK(s.size() == 3);
		CHECK(s.size_in_bits() == 17);
		s.rewind();
		CHECK(0xAB == s.read<u8>());
		CHECK(true == s.read<bool>());
		CHECK(0xCD == s.read<u8>());
	}


	SECTION("read/write u16")
	{
		serializer s;
		s.write<u16>(0xABCD);
		s.write<u16>(0xEF01);
		CHECK(s.size() == 4);
		CHECK(s.size_in_bits() == 32);
		CHECK(s.data()[0] == 0xAB);
		CHECK(s.data()[1] == 0xCD);
		CHECK(s.data()[2] == 0xEF);
		CHECK(s.data()[3] == 0x01);
		s.rewind();
		CHECK(0xABCD == s.read<u16>());
		CHECK(0xEF01 == s.read<u16>());
	}

	SECTION("read/write u32")
	{
		serializer s;
		s.write<u32>(0xAABB'CCDD);
		s.write<u32>(0xEEFF'0011);
		CHECK(s.size() == 8);
		CHECK(s.size_in_bits() == 64);
		CHECK(s.data()[0] == 0xAA);
		CHECK(s.data()[1] == 0xBB);
		CHECK(s.data()[2] == 0xCC);
		CHECK(s.data()[3] == 0xDD);
		CHECK(s.data()[4] == 0xEE);
		CHECK(s.data()[5] == 0xFF);
		CHECK(s.data()[6] == 0x00);
		CHECK(s.data()[7] == 0x11);
		s.rewind();
		CHECK(0xAABB'CCDD == s.read<u32>());
		CHECK(0xEEFF'0011 == s.read<u32>());
	}

	SECTION("read/write u64")
	{
		serializer s;
		s.write<u64>(0x1122'3344'5566'7788);
		s.write<u64>(0x99AA'BBCC'DDEE'FF00);
		CHECK(s.size() == 16);
		CHECK(s.size_in_bits() == 128);
		CHECK(s.data()[0] == 0x11);
		CHECK(s.data()[1] == 0x22);
		CHECK(s.data()[2] == 0x33);
		CHECK(s.data()[3] == 0x44);
		CHECK(s.data()[4] == 0x55);
		CHECK(s.data()[5] == 0x66);
		CHECK(s.data()[6] == 0x77);
		CHECK(s.data()[7] == 0x88);
		CHECK(s.data()[8] == 0x99);
		CHECK(s.data()[9] == 0xAA);
		CHECK(s.data()[10] == 0xBB);
		CHECK(s.data()[11] == 0xCC);
		CHECK(s.data()[12] == 0xDD);
		CHECK(s.data()[13] == 0xEE);
		CHECK(s.data()[14] == 0xFF);
		CHECK(s.data()[15] == 0x00);
		s.rewind();
		CHECK(0x1122'3344'5566'7788 == s.read<u64>());
		CHECK(0x99AA'BBCC'DDEE'FF00 == s.read<u64>());
	}

	SECTION("read/write bool")
	{
		serializer s;
		s.write<bool>(true);
		s.write<bool>(false);
		s.write<bool>(true);
		CHECK(s.size() == 1);
		CHECK(s.size_in_bits() == 3);
		CHECK(s.data()[0] == 0b1010'0000); // 0xA0
		s.rewind();
		CHECK(true == s.read<bool>());
		CHECK(false == s.read<bool>());
		CHECK(true == s.read<bool>());
	}

	SECTION("read/write f32")
	{
		serializer s;
		s.write<f32>(2.0f);
		s.write<f32>(4.0f);
		CHECK(s.size() == 8);
		CHECK(s.size_in_bits() == 64);

		s.rewind();
		CHECK(2.0f == s.read<f32>());
		CHECK(4.0f == s.read<f32>());
	}

	SECTION("read/write f64")
	{
		serializer s;
		s.write<f64>(2.0);
		s.write<f64>(4.0);
		CHECK(s.size() == 16);
		CHECK(s.size_in_bits() == 128);
		s.rewind();
		CHECK(2.0 == s.read<f64>());
		CHECK(4.0 == s.read<f64>());
	}

	SECTION("read/write string")
	{
		serializer s;
		s.write("Hello, World!");
		s.write("Deckard Serializer");
		CHECK(s.size() == 39);
		CHECK(s.size_in_bits() == 312);
		s.rewind();
		std::string str1 = s.read<std::string>();
		std::string str2 = s.read<std::string>();
		CHECK(str1 == "Hello, World!");
		CHECK(str2 == "Deckard Serializer");
	}

	SECTION("write/read array")
	{
		serializer        s;
		std::array<u8, 4> arr1{0x01, 0x02, 0x03, 0x04};
		std::array<u8, 6> arr2{0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
		s.write(arr1);
		s.write(arr2);
		CHECK(s.size() == 18);
		CHECK(s.size_in_bits() == 144);
		s.rewind();
		std::array<u8, 4> read_arr1{};
		std::array<u8, 6> read_arr2{};
		s.read(read_arr1);
		s.read(read_arr2);
		CHECK(std::ranges::equal(arr1, read_arr1));
		CHECK(std::ranges::equal(arr2, read_arr2));
	}

	SECTION("write/read mixed types")
	{
		serializer s;
		s.write<u8>(0xAB);
		s.write<bool>(true);
		s.write<u16>(0xCD01);
		s.write<u32>(0x2345'6789);
		s.write<f32>(3.14f);
		s.write("Deckard");
		CHECK(s.size() == 23);
		CHECK(s.size_in_bits() == 177);

		s.rewind();

		CHECK(0xAB == s.read<u8>());
		CHECK(true == s.read<bool>());
		CHECK(0xCD01 == s.read<u16>());
		CHECK(0x2345'6789 == s.read<u32>());
		CHECK(3.14f == s.read<f32>());
		CHECK("Deckard" == s.read<std::string>());
	}

	SECTION("write/read mixed types w/o padding")
	{
		serializer s(padding::no);
		s.write<u8>(0xAB);         // 8
		s.write<bool>(true);       // 9
		s.write<u16>(0xCD01);      // 25
		s.write<u32>(0x2345'6789); // 32 + 25 = 57
		s.write<f32>(3.14f);       // 89
		s.write("Deckard");        // 7*8 + 32 + 89 = 177
		CHECK(s.size() == 23);
		CHECK(s.size_in_bits() == 177);
		s.rewind();
		CHECK(0xAB == s.read<u8>());
		CHECK(true == s.read<bool>());
		CHECK(0xCD01 == s.read<u16>());
		CHECK(0x2345'6789 == s.read<u32>());
		CHECK(3.14f == s.read<f32>());
		CHECK("Deckard" == s.read<std::string>());
	}
}

TEST_CASE("bitwriter", "[bitwriter][serializer]")
{
#if 0
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
#endif
}
