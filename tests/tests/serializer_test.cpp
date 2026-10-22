#include <catch2/catch_test_macros.hpp>


import std;

import deckard;
import deckard.types;
import deckard.serializer;
import deckard.utf8;

using namespace deckard;

using namespace std::string_view_literals;
using namespace std::string_literals;

struct Pod
{
	i32 a{-1};
	u32 b{0xABCD};
};

struct NonPod
{
	i32               a{-1};
	u32               b{0xABCD};
	std::string       str{"???"};
	std::array<u8, 4> bytes{0xDE, 0xAD, 0xBE, 0xEF};

	template<serializer_type S>
	void serialize(S& s) const
	{
		s.write(a);
		s.write(b);
		s.write(str);
		s.write(bytes);
	}

	template<serializer_type S>
	void deserialize(S& s)
	{
		s.read(a);
		s.read(b);
		s.read(str);
		s.read(bytes);
	}
};

// non-member serialize/deserialize, found via ADL in the type's namespace
struct NonPod2
{
	i32               a{-1};
	u32               b{0xABCD};
	std::string       str{"???"};
	std::array<u8, 4> bytes{0xDE, 0xAD, 0xBE, 0xEF};
};

template<serializer_type S>
void serialize(S& s, const NonPod2& v)
{
	s.write(v.a);
	s.write(v.b);
	s.write(v.str);
	s.write(v.bytes);
}

template<serializer_type S>
void deserialize(S& s, NonPod2& v)
{
	s.read(v.a);
	s.read(v.b);
	s.read(v.str);
	s.read(v.bytes);
}

struct NonSerializable
{
	std::string text;
};

// serializer_type<S>: S is a serializer (checks a byte); serializer_type<S, T> checks a specific type
static_assert(serializer_type<inplace_serializer>);
static_assert(serializer_type<inplace_serializer, u8>);
static_assert(serializer_type<inplace_serializer, i32>);
static_assert(serializer_type<inplace_serializer, u64>);
static_assert(serializer_type<inplace_serializer, bool>);
static_assert(serializer_type<inplace_serializer, f32>);
static_assert(serializer_type<inplace_serializer, std::string>);
static_assert(serializer_type<inplace_serializer, std::string_view>);
static_assert(serializer_type<inplace_serializer, std::vector<u8>>);
static_assert(serializer_type<inplace_serializer, std::array<u8, 4>>);
static_assert(serializer_type<inplace_serializer, Pod>);
static_assert(serializer_type<inplace_serializer, NonPod>);
static_assert(serializer_type<inplace_serializer, NonPod2>);
static_assert(not serializer_type<inplace_serializer, NonSerializable>);

TEST_CASE("inplace serializer", "[serializer]")
{


	SECTION("init")
	{
		inplace_serializer s;
		CHECK(s.empty());
		CHECK(s.size() == 0);
	}

	SECTION("write/read")
	{
		inplace_serializer s;

		s.write<i8>(-1);
		CHECK(s.size() == 1);
		CHECK(s[0] == 0xFF);
		CHECK(i8{-1} == s.read<i8>());
		s.clear();

		s.write<i64>(-2);
		CHECK(s.size() == 8);
		CHECK(s[0] == 0xFF);
		CHECK(s[1] == 0xFF);
		CHECK(s[2] == 0xFF);
		CHECK(s[3] == 0xFF);
		CHECK(s[4] == 0xFF);
		CHECK(s[5] == 0xFF);
		CHECK(s[6] == 0xFF);
		CHECK(s[7] == 0xFE);
		CHECK(i64{-2} == s.read<i64>());
		s.clear();


		s.write<u16>(0xABCD);
		CHECK(s.size() == 2);
		CHECK(s[0] == 0xAB);
		CHECK(s[1] == 0xCD);
		CHECK(u16{0xABCD} == s.read<u16>());
		s.clear();

		s.write<u32>(0xDEAD'BEEF);
		CHECK(s.size() == 4);
		CHECK(s[0] == 0xDE);
		CHECK(s[1] == 0xAD);
		CHECK(s[2] == 0xBE);
		CHECK(s[3] == 0xEF);
		CHECK(u32{0xDEAD'BEEF} == s.read<u32>());
		s.clear();

		s.write<u64>(0x1122'3344'5566'7788);
		CHECK(s.size() == 8);
		CHECK(s[0] == 0x11);
		CHECK(s[1] == 0x22);
		CHECK(s[2] == 0x33);
		CHECK(s[3] == 0x44);
		CHECK(s[4] == 0x55);
		CHECK(s[5] == 0x66);
		CHECK(s[6] == 0x77);
		CHECK(s[7] == 0x88);
		CHECK(u64{0x1122'3344'5566'7788} == s.read<u64>());
		s.clear();

		s.write<bool>(true);
		CHECK(s.size() == 1);
		CHECK(s[0] == 1);
		CHECK(true == s.read<bool>());
		s.clear();

		s.write<f32>(3.14f);
		CHECK(s.size() == 4);
		CHECK(s[0] == 0x40);
		CHECK(s[1] == 0x48);
		CHECK(s[2] == 0xF5);
		CHECK(s[3] == 0xC3);
		CHECK(3.14f == s.read<f32>());
		s.clear();

		s.write("hello"sv);
		CHECK(s.size() == 4 + 5);
		CHECK(s[0] == 0x00);
		CHECK(s[1] == 0x00);
		CHECK(s[2] == 0x00);
		CHECK(s[3] == 0x05);
		CHECK(s[4] == 'h');
		CHECK(s[5] == 'e');
		CHECK(s[6] == 'l');
		CHECK(s[7] == 'l');
		CHECK(s[8] == 'o');
		CHECK("hello"sv == s.read<std::string_view>());
		s.rewind();
		CHECK("hello"s == s.read<std::string>());
		s.clear();

		s.write(utf8::string("🌍hello🌍"sv));
		CHECK(s.size() == 4 + (4 + 5 + 4));
		CHECK(s[0] == 0x00);
		CHECK(s[1] == 0x00);
		CHECK(s[2] == 0x00);
		CHECK(s[3] == 0x0D);

		CHECK(s[4] == 0xF0);
		CHECK(s[5] == 0x9F);
		CHECK(s[6] == 0x8C);
		CHECK(s[7] == 0x8D);
		CHECK(s[8] == 'h');
		CHECK(s[9] == 'e');
		CHECK(s[10] == 'l');
		CHECK(s[11] == 'l');
		CHECK(s[12] == 'o');
		CHECK(s[13] == 0xF0);
		CHECK(s[14] == 0x9F);
		CHECK(s[15] == 0x8C);
		CHECK(s[16] == 0x8D);
		CHECK("🌍hello🌍"sv == s.read<std::string_view>());
		s.clear();

		std::array<u8, 16> arr = {
		  0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC};
		s.write(arr);
		CHECK(s.size() == 4 + 16);
		CHECK(s[0] == 0x00);
		CHECK(s[1] == 0x00);
		CHECK(s[2] == 0x00);
		CHECK(s[3] == 0x10);
		for (std::size_t i = 0; i < arr.size(); ++i)
			CHECK(s[4 + i] == arr[i]);

		auto read_arr = s.read<std::array<u8, 16>>();
		for (std::size_t i = 0; i < arr.size(); ++i)
			CHECK(read_arr[i] == arr[i]);
		s.clear();
	}

	SECTION("enum serialize")
	{
		enum class Color : u8
		{
			Red = 6,
			Green,
			Blue
		};

		enum class Flags : u16
		{
			None = 0,
			A    = 0x0102,
			B    = 0x0304
		};

		enum class State 
		{
			Idle,
			Running,
			Stopped
		};

		inplace_serializer s;

		s.write(Color::Green); 
		CHECK(s.size() == 1);
		CHECK(s[0] == 7);

		s.write(Flags::A); 
		CHECK(s.size() == 1 + 2);
		CHECK(s[1] == 0x01);
		CHECK(s[2] == 0x02);

		s.write(State::Stopped); 
		CHECK(s.size() == 1 + 2 + 4);
		CHECK(s[3] == 0x00);
		CHECK(s[4] == 0x00);
		CHECK(s[5] == 0x00);
		CHECK(s[6] == 0x02);

		CHECK(s.read<Color>() == Color::Green);
		CHECK(s.read<Flags>() == Flags::A);
		CHECK(s.read<State>() == State::Stopped);
		CHECK(s.remaining() == 0);
	}

	SECTION("fill serializer")
	{
		inplace_serializer s;
		std::vector<u8>    data;
		data.resize(s.max_size() - sizeof(u32), 0x55);

		s.write(data);

		CHECK(s.size() == s.max_size());
		CHECK(s.remaining() == s.max_size());

		const auto payload = s.read();
		CHECK(payload.size() == s.max_size() - sizeof(u32));
		CHECK(s.remaining() == 0);

		CHECK(std::ranges::all_of(payload, [](u8 b) { return b == 0x55; }));

		std::vector<u8> v;
		s.rewind();
		s.read(v);
		CHECK(v.size() == s.max_size() - sizeof(u32));
		CHECK(std::ranges::all_of(v, [](u8 b) { return b == 0x55; }));


	}

	SECTION("pod struct serialize")
	{
		inplace_serializer s;
		s.write(Pod{-1, 42});
		CHECK(s.size() == 4 + 8);

		Pod read_struct{};
		s.read(read_struct);

		CHECK(read_struct.a == i32{-1});
		CHECK(read_struct.b == u32{42});
	}


	SECTION("non pod struct serialize")
	{
		inplace_serializer s;

		s.write(NonPod{-1, 42, "hello"s, {0xDE, 0xAD, 0xBE, 0xEF}});
		CHECK(s.size() == 4 + 4 + (4 + 5) + (4 + 4));

		NonPod read_struct{};
		s.read(read_struct);
		CHECK(read_struct.a == i32{-1});
		CHECK(read_struct.b == u32{42});
		CHECK(read_struct.str == "hello"s);
		CHECK(read_struct.bytes == std::array<u8, 4>{0xDE, 0xAD, 0xBE, 0xEF});
	}

	SECTION("non pod free function serialize")
	{
		inplace_serializer s;

		s.write(NonPod2{-1, 42, "hello"s, {0xDE, 0xAD, 0xBE, 0xEF}});
		CHECK(s.size() == 4 + 4 + (4 + 5) + (4 + 4));

		NonPod2 read_struct{};
		s.read(read_struct);
		CHECK(read_struct.a == i32{-1});
		CHECK(read_struct.b == u32{42});
		CHECK(read_struct.str == "hello"s);
		CHECK(read_struct.bytes == std::array<u8, 4>{0xDE, 0xAD, 0xBE, 0xEF});

		s.clear();
		s.write(NonPod2{-3, 126, "again"s, {0x01, 0x02, 0x03, 0x04}});

		const auto read_value = s.read<NonPod2>();
		CHECK(read_value.a == i32{-3});
		CHECK(read_value.b == u32{126});
		CHECK(read_value.str == "again"s);
		CHECK(read_value.bytes == std::array<u8, 4>{0x01, 0x02, 0x03, 0x04});
	}

	SECTION("mix regular, pod, non pod struct serialize")
	{

		inplace_serializer s;

		s.write(Pod{-1, 42});
		s.write<u32>(0xDEAD'BEEF);
		s.write(NonPod{-2, 84, "world"s, {0x11, 0x22, 0x33, 0x44}});

		CHECK(s.size() == (4 + 8) + 4 + (4 + 4 + (4 + 5) + (4 + 4)));

		Pod read_struct{};
		s.read(read_struct);
		CHECK(read_struct.a == i32{-1});
		CHECK(read_struct.b == u32{42});

		u32 read_u32 = s.read<u32>();
		CHECK(read_u32 == 0xDEAD'BEEF);

		NonPod read_nonpod{};
		s.read(read_nonpod);
		CHECK(read_nonpod.a == i32{-2});
		CHECK(read_nonpod.b == u32{84});
		CHECK(read_nonpod.str == "world"s);
		CHECK(read_nonpod.bytes == std::array<u8, 4>{0x11, 0x22, 0x33, 0x44});
	}
}

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
		// serializer s(padding::no);
		// s.write<u8>(0xAB);
		// s.write<bool>(true);
		// s.write<u8>(0xCD);
		// CHECK(s.size() == 3);
		// CHECK(s.size_in_bits() == 17);
		// s.rewind();
		// CHECK(0xAB == s.read<u8>());
		// CHECK(true == s.read<bool>());
		// CHECK(0xCD == s.read<u8>());
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
		// s.write("Deckard Serializer");
		// CHECK(s.size() == 39);
		// CHECK(s.size_in_bits() == 312);
		s.rewind();
		std::string str1 = s.read<std::string>();
		CHECK(str1 == "Hello, World!");
		// std::string str2 = s.read<std::string>();
		// CHECK(str2 == "Deckard Serializer");
	}

	SECTION("write/read array")
	{
		// TODO: fix writing arrays

		// serializer        s;
		// std::array<u8, 4> arr1{0x01, 0x02, 0x03, 0x04};
		// std::array<u8, 6> arr2{0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
		// s.write(arr1);
		// s.write(arr2);
		// CHECK(s.size() == 18);
		// CHECK(s.size_in_bits() == 144);
		// s.rewind();
		// std::array<u8, 4> read_arr1{};
		// std::array<u8, 6> read_arr2{};
		// s.read(read_arr1);
		// s.read(read_arr2);
		// CHECK(std::ranges::equal(arr1, read_arr1));
		// CHECK(std::ranges::equal(arr2, read_arr2));
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

	SECTION("qoi encode header/end marker")
	{
		image_rgb img(1, 1);
		img[0, 0] = rgb(10, 20, 30);

		deckard::serializer ser(deckard::padding::yes);
		// header
		ser.write<u8>('q');
		ser.write<u8>('o');
		ser.write<u8>('i');
		ser.write<u8>('f');
		ser.write_be<u32>(1);
		ser.write_be<u32>(1);
		ser.write<u8>(3);
		ser.write<u8>(0);
		// simplest possible encoding for first pixel: QOI_OP_RGB + rgb
		ser.write<u8>(0xFE);
		ser.write<u8>(10);
		ser.write<u8>(20);
		ser.write<u8>(30);
		// end marker
		for (int i = 0; i < 7; ++i)
			ser.write<u8>(0);
		ser.write<u8>(1);

		auto data = ser.data();
		CHECK(data.size() == 14 + 4 + 8);
		CHECK(data[0] == 'q');
		CHECK(data[1] == 'o');
		CHECK(data[2] == 'i');
		CHECK(data[3] == 'f');
		CHECK(data[4] == 0);
		CHECK(data[5] == 0);
		CHECK(data[6] == 0);
		CHECK(data[7] == 1);
		CHECK(data[8] == 0);
		CHECK(data[9] == 0);
		CHECK(data[10] == 0);
		CHECK(data[11] == 1);
		CHECK(data[12] == 3);
		CHECK(data[13] == 0);
		CHECK(data[data.size() - 1] == 1);
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
