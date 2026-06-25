#include <catch2/catch_test_macros.hpp>
// #include <catch2/catch_approx.hpp>
//  #include <catch2/matchers/catch_matchers_floating_point.hpp>


import std;
import deckard.types;
import deckard.memory;

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
		CHECK(ptr1 != nullptr);
		CHECK(frame.used() == 128);
		CHECK(frame.free() == 896);
		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("arena allocate")
	{
		memory::arena frame(1024);
		void*         ptr1 = frame.allocate(128);
		CHECK(ptr1 != nullptr);
		CHECK(frame.used() == 128);
		CHECK(frame.free() == 896);
		void* ptr2 = frame.allocate(256);
		CHECK(ptr2 != nullptr);
		CHECK(frame.used() == 384);
		CHECK(frame.free() == 640);

		void* ptr3 = frame.allocate(700); // Should fail
		CHECK(ptr3 == nullptr);
		CHECK(frame.used() == 384);
		CHECK(frame.free() == 640);

		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("arena allocate with alignment")
	{
		memory::arena frame(1024);
		void*         ptr1 = frame.allocate(128, 16);
		CHECK(ptr1 != nullptr);
		CHECK(frame.used() == 128);
		CHECK(frame.free() == 896);
		CHECK(reinterpret_cast<std::uintptr_t>(ptr1) % 16 == 0);
		void* ptr2 = frame.allocate(256, 32);
		CHECK(ptr2 != nullptr);

		auto base          = reinterpret_cast<std::uintptr_t>(frame.data().data());
		auto expected_used = reinterpret_cast<std::uintptr_t>(ptr2) - base + 256;
		CHECK(frame.used() == expected_used);
		CHECK(frame.free() == 1024 - expected_used);
		CHECK(reinterpret_cast<std::uintptr_t>(ptr2) % 32 == 0);

		frame.reset();
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("arena allocate oversized")
	{
		memory::arena frame(1024);
		void*         ptr1 = frame.allocate(2048);
		CHECK(ptr1 == nullptr);
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}

	SECTION("arena allocate zero size")
	{
		memory::arena frame(1024);
		void*         ptr1 = frame.allocate(0);
		CHECK(ptr1 != nullptr);
		CHECK(frame.used() == 0);
		CHECK(frame.free() == 1024);
	}


	SECTION("allocate block")
	{
		memory::arena frame(1024);
		auto          ptr = frame.allocate_block<int>(8);

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

		auto particles3 = frame.allocate_block<Particle>(2);
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
