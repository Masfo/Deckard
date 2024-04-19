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

	u32 major = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x026C); // NtMajorVersion, 4.0+
	u32 minor = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x0270); // NtMinorVersion, 4.0+

	u32 build = 0;
	if (major >= 10)
		build = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x0260); // NtBuildNumber, 10.0+

	dbg::println("Winver: {}.{}.{}", major, minor, build);


	dbg::println("{:<20f}", std::numeric_limits<float>::max());
	dbg::println("{:<20}", std::numeric_limits<u64>::max());


	i64 num1 = 9223'37203'68547'75807i64; // Max value of 64-bit signed integer
	i64 num2 = 1;

	if (multiplyOverflow(num1, num2))
		dbg::println("overflow");
	else
		dbg::println("ok");


	//	std::println("{}", readAndWrite);

	std::println("Script Compiler v5");

	return 0;
}
