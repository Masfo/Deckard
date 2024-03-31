
#include <signal.h>
#include <stdlib.h>

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

int main()
{


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
