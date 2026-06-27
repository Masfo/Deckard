#include <catch2/catch_test_macros.hpp>
// #include <catch2/catch_approx.hpp>
//  #include <catch2/matchers/catch_matchers_floating_point.hpp>


import std;
import deckard.types;
import deckard.memory;
import deckard.helpers;
import deckard.as;

TEST_CASE("stack arena", "[arena][memory]")
{
	using namespace deckard;

	struct Particle
	{
		f32 x, y, z;
	};

	SECTION("stackarena ctor")
	{
		memory::stackarena<1024> frame;
		CHECK(frame.capacity() == 1024);
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}
	SECTION("stackarena reset")
	{
		memory::stackarena<1024> frame;
		auto                     ptr1 = frame.allocate(128);
		CHECK(not ptr1.empty());
		CHECK(frame.used() == 128);
		CHECK(frame.free() == 896);
		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("stackarena allocate")
	{
		memory::stackarena<1024> frame;
		auto                     ptr1 = frame.allocate(128);
		CHECK(frame.used() == 128);
		CHECK(frame.free() == 896);
		auto ptr2 = frame.allocate(256);
		CHECK(frame.used() == 384);
		CHECK(frame.free() == 640);
		
		auto ptr3 = frame.allocate(700); // Should fail
		CHECK(ptr3.size() == 0);
		CHECK(frame.used() == 384);
		CHECK(frame.free() == 640);

		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("stackarea allocate aligned")
	{
		memory::stackarena<1024> frame;
		const usize               base = as<usize>(frame.data().data());

		auto ptr1 = frame.allocate(128, 32);
		CHECK(not ptr1.empty());
		CHECK(is_pointer_aligned(ptr1.data(), 32));
		CHECK(frame.used() == as<usize>(ptr1.data()) - base + 128);
		CHECK(frame.used() + frame.free() == 1024);

		auto ptr2 = frame.allocate(256, 64);
		CHECK(not ptr2.empty());
		CHECK(is_pointer_aligned(ptr2.data(), 64));
		CHECK(frame.used() == as<usize>(ptr2.data()) - base + 256);
		CHECK(frame.used() + frame.free() == 1024);
		CHECK(ptr1.data() != ptr2.data());

		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}


	SECTION("stackarena create particles")
	{
		memory::stackarena<1024> frame;
		CHECK(frame.capacity() == 1024);
		auto* p = frame.create<Particle>(1.0f, 2.0f, 3.0f);
		CHECK(p != nullptr);
		CHECK(frame.used() == sizeof(Particle));
		CHECK(frame.free() == 1024 - sizeof(Particle));
		CHECK(p->x == 1.0f);
		CHECK(p->y == 2.0f);
		CHECK(p->z == 3.0f);
	}

	SECTION("stackarena create array of particles")
	{
		memory::stackarena<1024> frame;
		CHECK(frame.capacity() == 1024);
		auto particles = frame.create_array<Particle>(5);
		CHECK(particles.size() == 5);
		CHECK(frame.used() == sizeof(Particle) * 5);
		CHECK(frame.free() == 1024 - sizeof(Particle) * 5);
		for (size_t i = 0; i < particles.size(); ++i)
		{
			particles[i].x = static_cast<f32>(i);
			particles[i].y = static_cast<f32>(i + 1);
			particles[i].z = static_cast<f32>(i + 2);
		}
		for (size_t i = 0; i < particles.size(); ++i)
		{
			CHECK(particles[i].x == static_cast<f32>(i));
			CHECK(particles[i].y == static_cast<f32>(i + 1));
			CHECK(particles[i].z == static_cast<f32>(i + 2));
		}
	}
}

TEST_CASE("arena", "[arena][memory]")
{
	using namespace deckard;
	SECTION("arena ctor")
	{
		memory::arena frame(1024);

		CHECK(frame.capacity() == 1024);
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("arena reset")
	{
		memory::arena frame(1024);
		auto          ptr1 = frame.allocate(128);
		CHECK(not ptr1.empty());
		CHECK(frame.used() == 128);
		CHECK(frame.free() == 896);
		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("arena allocate")
	{
		memory::arena frame(1024);
		auto          ptr1 = frame.allocate(128);
		CHECK(frame.used() == 128);
		CHECK(frame.free() == 896);
		auto ptr2 = frame.allocate(256);
		CHECK(frame.used() == 384);
		CHECK(frame.free() == 640);

		auto ptr3 = frame.allocate(700); // Should fail
		CHECK(frame.used() == 384);
		CHECK(frame.free() == 640);

		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("arena allocate aligned")
	{
		memory::arena frame(1024);
		const auto    base = as<usize>(frame.data().data());

		auto ptr1 = frame.allocate(128, 32);
		CHECK(not ptr1.empty());
		CHECK(is_pointer_aligned(ptr1.data(), 32));
		CHECK(frame.used() == as<usize>(ptr1.data()) - base + 128u);
		CHECK(frame.used() + frame.free() == 1024);

		auto ptr2 = frame.allocate(256, 64);
		CHECK(not ptr2.empty());
		CHECK(is_pointer_aligned(ptr2.data(), 64));
		CHECK(frame.used() == as<usize>(ptr2.data()) - base + 256);
		CHECK(frame.used() + frame.free() == 1024);

		CHECK(ptr1.data() != ptr2.data());

		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}


	SECTION("arena allocate oversized")
	{
		memory::arena frame(1024);
		auto          ptr1 = frame.allocate(2048);
		CHECK(ptr1.size() == 0);
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("arena allocate zero size")
	{
		memory::arena frame(1024);
		auto          ptr1 = frame.allocate(0);
		CHECK(ptr1.size() == 0);
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}


	SECTION("allocate block")
	{
		memory::arena frame(1024);
		auto          ptr = frame.allocate<int>(8);

		ptr[0] = 0xDEAD'BEEF;

		CHECK(ptr.size() == 8);
		CHECK(ptr.size_bytes() == 8 * sizeof(int));

		auto data = frame.data();
		CHECK(data[0] == 0xEF_byte);
		CHECK(data[1] == 0xBE_byte);
		CHECK(data[2] == 0xAD_byte);
		CHECK(data[3] == 0xDE_byte);
	}

	SECTION("create particles")
	{
		struct Particle
		{
			f32 x, y, z;
		};

		memory::arena frame(1024);
		CHECK(frame.capacity() == 1024);


		auto* p = frame.create<Particle>(1.0f, 2.0f, 3.0f);

		CHECK(p != nullptr);
		CHECK(frame.used() == sizeof(Particle));
		CHECK(frame.free() == 1024 - sizeof(Particle));

		CHECK(p->x == 1.0f);
		CHECK(p->y == 2.0f);
		CHECK(p->z == 3.0f);
	}

	struct Particle
	{
		f32 x, y, z;
	};

	SECTION("create array of particles")
	{

		memory::arena frame(1024);
		CHECK(frame.capacity() == 1024);

		auto particles = frame.create_array<Particle>(5);

		CHECK(particles.size() == 5);
		CHECK(frame.used() == sizeof(Particle) * 5);
		CHECK(frame.free() == 1024 - sizeof(Particle) * 5);

		for (size_t i = 0; i < particles.size(); ++i)
		{
			particles[i].x = static_cast<f32>(i);
			particles[i].y = static_cast<f32>(i + 1);
			particles[i].z = static_cast<f32>(i + 2);
		}

		for (size_t i = 0; i < particles.size(); ++i)
		{
			CHECK(particles[i].x == static_cast<f32>(i));
			CHECK(particles[i].y == static_cast<f32>(i + 1));
			CHECK(particles[i].z == static_cast<f32>(i + 2));
		}

		auto particles2 = frame.create_array<Particle>(100); // Should fail
		CHECK(particles2.size() == 0);
		CHECK(frame.used() == sizeof(Particle) * 5);
		CHECK(frame.free() == frame.capacity() - sizeof(Particle) * 5);
	}

	SECTION("create array of particles with initial values")
	{
		memory::arena frame(1024);
		auto          particles = frame.create_array<Particle>(5, 1.0f, 2.0f, 3.0f);

		CHECK(particles.size() == 5);
		CHECK(frame.used() == sizeof(Particle) * 5);
		CHECK(frame.free() == frame.capacity() - sizeof(Particle) * 5);

		for (auto& p : particles) // #1
		{
			CHECK(p.x == 1.0f);
			CHECK(p.y == 2.0f);
			CHECK(p.z == 3.0f);
		}

		particles[4] = {}; // #2
		frame.reset();

		auto particles2 = frame.create_array<Particle>(3, 10.0f, 11.0f, 12.0f);
		CHECK(particles2.size() == 3);
		CHECK(frame.used() == sizeof(Particle) * 3);
		CHECK(frame.free() == frame.capacity() - sizeof(Particle) * 3);

		for (auto& p : particles2)
		{
			CHECK(p.x == 10.0f);
			CHECK(p.y == 11.0f);
			CHECK(p.z == 12.0f);
		}

		auto particles3 = frame.allocate<Particle>(2);
		CHECK(particles3.size() == 2);

		// #1
		CHECK(particles3[0].x == 1.0f);
		CHECK(particles3[0].y == 2.0f);
		CHECK(particles3[0].z == 3.0f);

		// #2
		CHECK(particles3[1].x == 0.0f);
		CHECK(particles3[1].y == 0.0f);
		CHECK(particles3[1].z == 0.0f);
	}
}
