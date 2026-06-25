#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.random;
import deckard.math.utils;
import deckard.types;
using namespace deckard;





TEST_CASE("random", "[random]")
{


	SECTION("random between limits")
	{


		CHECK(random::randu8(128, 128) == 128);

		CHECK(random::randu8(0, 128) <= 128);
		CHECK(random::randu16(0, 6789) <= 6789);
		CHECK(random::randu32(0, 123'456'789) <= 123'456'789);
		CHECK(random::randu64(0, 123'456'789'012'345'678) <= 123'456'789'012'345'678);

		CHECK(math::is_between(random::rand(), limits::min<u32>, limits::max<u32>));

		CHECK(math::is_between(random::randi8(-64, 64), -64_i8, 64_i8));
		CHECK(math::is_between(random::randi16(-1000, 1000), -1000_i16, 1000_i16));
		CHECK(math::is_between(random::randi32(-1'000'000, 1'000'000), -1'000'000_i32, 1'000'000_i32));
		CHECK(
		  math::is_between(random::randi64(-1'000'000'000'000, 1'000'000'000'000), -1'000'000'000'000_i64, 1'000'000'000'000_i64));
		f32 v01 = random::float01();
		CHECK(v01 >= 0.0f);
		CHECK(v01 <= 1.0f);

		f32 v11 = random::float11();
		CHECK(v11 >= -1.0f);
		CHECK(v11 <= 1.0f);
	}
}
