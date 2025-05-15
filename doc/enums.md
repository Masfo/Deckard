## deckard.enums

```cpp

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

		// Enable flags by adding the following to your enum
		consteval void enable_bitmask_operations(Permission);
	} // namespace Filesystem
	  
	// Usage:
	  
	using Filesystem::Permission;
	Permission readAndWrite{Permission::Read | Permission::Write | Permission::Execute};
	  
	readAndWrite &= ~Permission::Write; // Read | Execute
	readAndWrite |= Permission::Write;  // Read | Execute | Write
	  
	// or alternatively using += and -=
	readAndWrite -= Permission::Execute; // Read | Write
	readAndWrite += Permission::Execute; // Read | Write | Execute
```