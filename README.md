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

	assert::check(1==2, "Wrong");
	
	```
  - **Basic types**
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
  - **Result**
	```cpp
	// Default error is std::string
	Ok(<value>);
	Err("error value");

	// Return value or error
	auto getex(int x) -> Result<int>
	{
		if (x == 10)
			return Ok(x); 
		else
			return Err("bad int {}", x);
	}

	if (auto v = getex(100); v)
	{
		dbg::trace("int {}", *v);
	}
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
  - **SHA256**
	```cpp
	import deckard.sha2;

	using namespace deckard;
	sha256::hasher hasher256;
	hasher256.update("abc");
	auto digest = hasher256.finalize().to_string();
	// digest = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"


	sha512::hasher hasher512;
	hasher512.update("abc");
	auto digest = hasher512.finalize().to_string();
	// digest = "ddaf35a193617abacc417349ae20...d454d4423643ce80e2a9ac94fa54ca49f"
	````
  - **ZSTD**
	```cpp
	import deckard.zstd;

	using namespace deckard;
	std::vector<u8> buffer;
	buffer.resize(1'024);
	std::ranges::fill(buffer, 'X');

	// Compress
	std::vector<u8> comp;
	comp.resize(zstd::bound(buffer));
	auto ok = zstd::compress(buffer, comp);
	if (!ok)
	{
		dbg::println("Failed to compress");
	}
	else
		comp.resize(*ok);


	// Uncompress
	std::vector<u8> uncomp;
	uncomp.resize(buffer.size());
	ok = zstd::uncompress(comp, uncomp);
	if (!ok)
		dbg::println("Failed to uncompress");
	else
	{
		auto kss = *ok;
		uncomp.resize(*ok);
	}
	````
  - **Serialization**
	```cpp
	import deckard.serialization;
	using namespace deckard;

	bitwriter writer;

	writer.write(0xAABB'CCDD);           // 32
	writer.write(0xEEFF'0011'2233'4455); // 96
	writer.write(2.0f);                  // 128
	writer.write(0b1111'0000, 8);        // 136
	writer.write("abcd");                // 168

	std::array<u8, 21> correct{
		  0xAA, 0xBB, 0xCC, 0xDD,					// AABBCCDD
		  0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,		// EEFF001122334455
		  0x40, 0x00, 0x00, 0x00,					// 2.0f
		  0xF0,								// 0b1111'0000
		  0x61, 0x62, 0x63, 0x64};					// abcd

	std::ranges::equal(correct, writer.data()) == true;

	// reader
	bitreader reader(writer.data());


	u32 v32       = reader.read<u32>();		// 0xAABCCDD
	u64 v64       = reader.read<u64>();		// 0xEEFF001122334455
	f32 f         = reader.read<f32>();		// 2.0f
	u8 u          = reader.read<u32>(8);		// 0b1111'0000
	std::string s = reader.read_string(4 * 8);	// "abcd"
	````
    - **Big integer**
    ```cpp
	// operators: + - * / %
	// functions: pow sqrt abs gcd lcm random_range popcount
	// bitwise ops: & | ^ ~ (and or xor not)
	// comparison: == != < > <= >=
	// conversion: to_string to_integer

	import deckard.bigint;
	using namespace deckard;

	bigint p(6);
	bigint q(7);

	assert::check(p + q == bigint{13}, "add");
	assert::check(p - q == bigint{-1}, "subtract");
	assert::check(p * q == bigint{42}, "multiply");
	assert::check((p * q) / q == p, "divide");
	assert::check(((p * q) % (p + q)) == bigint{3}, "mod");
	assert::check((p ^ q) == bigint{279'936}, "power");


	p = "641352894770715802787901901705773890848250147429434472081168596"
		"32024532344630238623598752668347708737661925585694639798853367";

	q = "333720275949781565562260106053551142279407603447675546667845209"
		"87023841729210037080257448673296881877565718986258036932062711";

	bigint n = p * q;

	bigint rsa250(
	  "214032465024074496126442307283933356300861471514475501779775492088141
	  "802344714013664334551909580467961099285187247091458768739626192155736
	  "304745477052080511905649310668769159001975940569345745223058932597669
	  "7471681738069364894699871578494975937497937");

	assert::check(n == rsa250, "RSA250 calculation is wrong");

	assert::check(sqrt(bigint(9)) == bigint(3), "sqrt");
	assert::check(abs(bigint(-9)) == bigint(9), "abs");
	assert::check(mod(bigint(42), bigint(13)) == bigint(3), "mod");
	assert::check(pow(bigint(6), bigint(7)) == bigint(279'936), "pow");
	assert::check(gcd(bigint(68), bigint(42)) == bigint(2), "gcd");
	assert::check(lcm(bigint(68), bigint(42)) == bigint(1428), "lcm");

	assert::check(bigint(42).to_string() == "42");
	assert::check(bigint("42").to_integer<u32>() == 42);

	auto rnd = random_bigint(42); // 42 digit (base 10) random number

	rnd.to_string(16, true);      // base 16, uppercase
	rnd.to_string(24, false);     // base 24, lowercase

	std::format("{}", rnd);       // base 10
	std::format("{:x}", rnd);     // base 16 (:x, :X, :h, :H)
	std::format("{:b2}", rnd);    // base 2
	std::format("{:B36}", rnd);   // base 36
	```
