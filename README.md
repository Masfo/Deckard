# Deckard - Utility Modules

1. in your CMakelists.txt add:
```
include(FetchContent)
FetchContent_Declare(
  Deckard
  GIT_REPOSITORY https://github.com/Masfo/Deckard.git
  GIT_TAG main
)
FetchContent_MakeAvailable(Deckard)

target_link_libraries(${CMAKE_PROJECT_NAME} Deckard)
```


## Modules:
  - **namespace**
	```cpp
	// Everything is under deckard namespace
	using namespace deckard;
	```
  - **Debug / Assert**
	```cpp
	import deckard;

	dbg::trace("Hello {}", "World"); // winmain.cpp(15): Hello World
	dbg::println("Similar to trace");  // Similar to trace
	dbg::print("Like dbgln but ");     // 
	dbg::print("w/o newline\n");       // Like dbgln but w/o newline 

	dbg::panic("reason");	

	assert::if_true_(true, "Message");
	assert::if_false(true);

	assert::if_equal(0==0);_
	assert::if_not_equal(0==1);

	assert::if_close_enough(3.14f, 3.0);

	assert::if_null(...);
	assert::if_not_null(...);
	```
  - **Types**
	  ```cpp
	  import deckard;

	  // i8,u8,i16,u16,i32,u32,i64,u64

	  u8 u  = 64_u8;
	  i64 i = 112233_i64;

	  // auto value = as<To>(From)
	  float X = 128.0f;
	  auto  u = as<i8>(X);
	  //  : Could not convert value '128' safely. Target too small: -128 < 128 < 127 
	  //  : Loss of precision casting '128.000000' to '-128'

	  float X = 128.0f;
	  auto  u = as<u16>(X);  // No warning, value fits

	  auto value = as<u32>(256);

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
  - **Fileview**
	```cpp 
	import deckard;

	Fileview f;
	f.open("dice.qoi", FileAccess::ReadWrite);

	f[f.size()-1] = 0x01;
	```
  - **Base64**
	```cpp
	import deckard.base64;

	// Encode
	std::string encode(const std::span<const u8> input)
	std::string encode_str(std::string_view input)

	// Decode
	std::optional<std::vector<u8>> decode(std::string_view encoded_input)
	std::string decode_str(std::string_view encoded_input)

	// example:
	std::array<byte, 4> buff{'Z', 'E', 'U', 'S'};

	auto encoded_base64 = utils::base64::encode(buff); // WkVVUw==
	auto decoded_base64 = utils::base64::decode(encoded_base64); // Optional vector with original data
	````
