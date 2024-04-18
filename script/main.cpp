#include <Windows.h>

import std;
import Deckard;

using namespace deckard;

bool multiplyOverflow(i64 a, i64 b)
{
	// Check if 'a' is zero
	if (a == 0)
		return false;

	// Check if 'b' is zero
	if (b == 0)
		return false;

	// Check if 'a' is greater than maximum value of 'b' / 'a'
	if (a > std::numeric_limits<i64>::max() / b)
		return true;

	// Check if 'b' is greater than maximum value of 'a' / 'b'
	if (b > std::numeric_limits<i64>::max() / a)
		return true;

	return false;
}

auto getex(int x) -> Result<int>
{
	if (x == 10)
		return Ok(x);
	else
		return Err("bad int {}", x);
	// return Err("bad int");
}

// Enums as flags
namespace Filesystem
{
	enum class Permission : u8
	{
		No      = 0x00,
		Read    = 0x01,
		Write   = 0x02,
		Execute = 0x04,
	};
	consteval void enable_bitmask_operations(Permission);


} // namespace Filesystem

using KUSER_SHARED_DATA_TYPE                           = u32;
constexpr KUSER_SHARED_DATA_TYPE KUSER_SHARED_DATA_PTR = 0x7FFE'0000u;

int main()
{


	char a = 0b0000'0001;
	char b = !a;

	char c = a xor 0b1111'1111;


	for (auto i : upto(10))
	{
		dbg::println("upto {}", i);
	}

	for (auto i : loop(5))
	{
		dbg::println("loop {}", i);

		if (i > 10)
			break;
	}


	u32 major = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x026C); // NtMajorVersion, 4.0+
	u32 minor = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x0270); // NtMinorVersion, 4.0+

	u32 build = 0;
	if (major >= 10)
		build = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x0260); // NtBuildNumber, 10.0+

	dbg::println("Winver: {}.{}.{}", major, minor, build);


	u64 MyTickCount = ((u64)(*(KUSER_SHARED_DATA_TYPE *)(KUSER_SHARED_DATA_PTR + 0x320)) *
					   (u64)(*(KUSER_SHARED_DATA_TYPE *)(KUSER_SHARED_DATA_PTR + 0x4))) >>
					  0x18;
	u64 TickCount = GetTickCount64();
	dbg::println("Ticks: {} vs {}\n", MyTickCount, TickCount);


	u64 ActiveCPUCount = (u64)(*(KUSER_SHARED_DATA_TYPE *)(KUSER_SHARED_DATA_PTR + 0x3C0));
	dbg::println("Processor count: {}", ActiveCPUCount);

	float X = 128.0f;
	auto  u = as<u16>(X);


	std::array<byte, 4> buff{'Z', 'E', 'U', 'S'};

	auto encoded_base64 = utils::base64::encode(buff);
	auto decoded_base64 = utils::base64::decode(encoded_base64);

	using Filesystem::Permission;
	Permission readAndWrite{Permission::Read | Permission::Write | Permission::Execute};

	readAndWrite &= ~Permission::Write; // Read | Execute
	readAndWrite |= Permission::Write;  // Read | Execute | Write

	// or alternatively using += and -=
	readAndWrite -= Permission::Execute; // Read | Write
	readAndWrite += Permission::Execute; // Read | Write | Execute


	dbg::println("{:<20f}", std::numeric_limits<float>::max());
	dbg::println("{:<20}", std::numeric_limits<u64>::max());


	auto xx = getex(0);


	if (auto v = getex(100); v)
	{
		dbg::trace("int {}", *v);
	}

	if (auto v = getex(10); v)
	{
		dbg::trace("int {}", *v);
	}


	constexpr auto ResultIntSize = sizeof(Result<int>);

	i64 num1 = 9223'37203'68547'75807i64; // Max value of 64-bit signed integer
	i64 num2 = 1;

	if (multiplyOverflow(num1, num2))
		dbg::println("overflow");
	else
		dbg::println("ok");


	// clang-format off
    graph::BinaryTree<char> tree[]
    {
                                    {'D', tree + 1, tree + 2},
        //                            │
        //            ┌───────────────┴────────────────┐
        //            │                                │
                    {'B', tree + 3, tree + 4},       {'F', tree + 5, tree + 6},
        //            │                                │
        //  ┌─────────┴─────────────┐      ┌───────────┴─────────────┐
        //  │                       │      │                         │
          {'A'},                  {'C'}, {'E'},                    {'G'}
    };
	// clang-format on


	for (const char x : tree->traverse_postorder())
	{
		dbg::println("{}", x);
	}


	//	std::println("{}", readAndWrite);

	std::println("Script Compiler v5");

	return 0;
}
