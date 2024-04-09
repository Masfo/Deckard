


import std;
import Deckard;

using namespace deckard;

namespace Filesystem
{
	enum class Permission : u8
	{
		NoPermissions = 0x00,
		Read          = 0x01,
		Write         = 0x02,
		Execute       = 0x04,
	};


	consteval void enable_bitmask_operations(Permission);


	/*
		using Filesystem::Permission;
		Permission readAndWrite{Permission::Read | Permission::Write};
	*/
} // namespace Filesystem

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

int main()
{
	u32  X = 126;
	auto u = as<float>(X);

	i8 a = X;


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

	using Filesystem::Permission;


	Permission readAndWrite{Permission::Read | Permission::Write | Permission::Execute};


	readAndWrite -= Permission::Write;

	readAndWrite -= Permission::Read;

	readAndWrite -= Permission::Execute;

	readAndWrite += Permission::Execute;
	readAndWrite += Permission::Write;


	//	std::println("{}", readAndWrite);

	std::println("Script Compiler v5");

	return 0;
}
