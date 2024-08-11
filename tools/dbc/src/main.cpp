#include <Windows.h>
#include <emmintrin.h>
#include <xmmintrin.h>


import std;
import deckard;
import deckard.as;
import deckard.types;
import deckard.helpers;
import deckard.win32;
import deckard.math;
import deckard.utf8;
import deckard.lexer;
import deckard.serializer;

import deckard.app;


#ifdef __cpp_multidimensional_subscript
#error("use multisubscript")
// const f32& operator[](std::size_t z, std::size_t y) const noexcept { return 0.0f; }
#endif


#ifndef _DEBUG
import dbc;
#endif
namespace fs = std::filesystem;
using namespace std::chrono_literals;

using namespace std::string_view_literals;
using namespace deckard;
using namespace deckard::utils;
using namespace deckard::system;
using namespace deckard::math;
using namespace deckard::utf8;


enum class ConvertEpoch : u64
{
	Microseconds = 10,
	Milliseconds = Microseconds * 1'000,
	Seconds      = Milliseconds * 1'000,
};

std::string from_epoch(u64 epoch, ConvertEpoch mul = ConvertEpoch::Seconds) noexcept
{
	u64 t{epoch};
	t *= as<u64>(mul);
	t += 116'444'736'000'000'000LL;

	FILETIME ft{};
	ft.dwLowDateTime  = t & 0xFFFF'FFFF;
	ft.dwHighDateTime = t >> 32;

	FILETIME local{};
	FileTimeToLocalFileTime(&ft, &local);

	SYSTEMTIME st{};
	FileTimeToSystemTime(&local, &st);

	if (mul == ConvertEpoch::Milliseconds)
		return std::format(
		  "{:04}-{:02}-{:02} {:02}:{:02}:{:#02}:{:0}", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	else
		return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

class IPv6Address
{

public:
	IPv6Address();

	bool fromString(const char* addrstr);
	void print();

private:
	unsigned char _address[16];
};

IPv6Address::IPv6Address() { memset(_address, 0, sizeof(_address)); }

#define MAX_IPV6_ADDRESS_STR_LEN 39

i8 asciiToHex(char c)
{
	c |= 0x20;

	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
	else if (c >= 'a' && c <= 'f')
	{
		return (c - 'a') + 10;
	}
	else
	{
		return -1;
	}
}

bool IPv6Address::fromString(const char* addrstr)
{
	u16 accumulator = 0;
	u8  colon_count = 0;
	u8  pos         = 0;

	memset(_address, 0, sizeof(_address));

	// Step 1: look for position of ::, and count colons after it
	for (uint8_t i = 1; i <= MAX_IPV6_ADDRESS_STR_LEN; i++)
	{
		if (addrstr[i] == ':')
		{
			if (addrstr[i - 1] == ':')
			{
				// Double colon!
				colon_count = 14;
			}
			else if (colon_count)
			{
				// Count backwards the number of colons after the ::
				colon_count -= 2;
			}
		}
		else if (addrstr[i] == '\0')
		{
			break;
		}
	}

	// Step 2: convert from ascii to binary
	for (uint8_t i = 0; i <= MAX_IPV6_ADDRESS_STR_LEN && pos < 16; i++)
	{
		if (addrstr[i] == ':' || addrstr[i] == '\0')
		{
			_address[pos]     = accumulator >> 8;
			_address[pos + 1] = (u8)accumulator;
			accumulator       = 0;

			if (colon_count && i && addrstr[i - 1] == ':')
			{
				pos = colon_count;
			}
			else
			{
				pos += 2;
			}
		}
		else
		{
			int8_t val = asciiToHex(addrstr[i]);
			if (val == -1)
			{
				// Not hex or colon: fail
				return 0;
			}
			else
			{
				accumulator <<= 4;
				accumulator |= val;
			}
		}

		if (addrstr[i] == '\0')
			break;
	}

	// Success
	return 1;
}

static void printPaddedHex(uint8_t byte)
{
	char str[2];
	str[0] = (byte >> 4) & 0x0f;
	str[1] = byte & 0x0f;

	for (int i = 0; i < 2; i++)
	{
		// base for converting single digit numbers to ASCII is 48
		// base for 10-16 to become lower-case characters a-f is 87
		if (str[i] > 9)
			str[i] += 39;
		str[i] += 48;
		dbg::print("{:c}", str[i]);
	}
}

void IPv6Address::print()
{
	for (int i = 0; i < 16; ++i)
	{
		printPaddedHex(_address[i]);
		if (i % 2 == 1 && i < 15)
			dbg::print(":");
	}
	dbg::println("");
}

class alignas(4) IpAddress
{
public:
	enum class Version : u8
	{
		v4 = 4,
		v6 = 6
	};

	IpAddress() = default;

	IpAddress(std::string_view) { }

	std::string as_string(size_t) const { return {}; }

	std::array<u16, 8> address{0};
	u16                port_num{0};
	Version            ver{Version::v4};
	u8                 flags{0};

	void version(u8 v) { ver = (v == 4_u8) ? Version::v4 : Version::v6; }

	void port(u16 p) { port_num = p; };

private:
	// Recommendation for IPv6 Address Text Representation: https://www.rfc-editor.org/rfc/rfc5952
};

static_assert(sizeof(IpAddress) == 20, "IpAddress is not 20-bytes");

class Chain
{
public:
	auto& set(int v)
	{
		dbg::println("{} = set({})", std::exchange(value, v), v);

		return *this;
	};

	auto& execute() const
	{
		dbg::println("execute({})", value);
		return *this;
	}

	auto& close() const
	{
		dbg::println("close");
		return *this;
	}

private:
	int value = 0;
};

std::string module_filename_from_function(void* function)
{
	std::string module_location(MAX_PATH, 0);
	HMODULE     module_handle{nullptr};

	if (GetModuleHandleExA(
		  GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (const char*)function, &module_handle) !=
		0)

	{
		auto size = GetModuleFileNameA(module_handle, module_location.data(), (DWORD)module_location.size());
		module_location.resize(size);
		return module_location;
	}
	return {};
}

template<typename T>
class Passkey
{
public:
	// Ensure Passkey is not implicitly convertible to any other type
	template<typename U, typename = std::enable_if_t<std::is_same_v<U, T>>>
	operator U() const = delete;
};

class Secret
{
public:
	void public_method() { }

private:
	template<typename T>
	void private_method(Passkey<T>)
	{
		// Access to private data or logic
	}

	friend class TrustedClass;
};

class TrustedClass
{
public:
	void call_private_method(Secret& secret)
	{
		secret.private_method(Passkey<int>{}); // Can create Passkey<int>
	}
};

int main()
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", dbc::build::version_string, dbc::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif


	lexer::tokenizer l2(R"(
[section.name]
key=value
key2=123

[section2]
width=1920
aspect=1.666
fullscreen=true
)"sv);
	auto             tokens = l2.tokenize({.dot_identifier = true, .output_eol = true});

	serializer s;
	s.write_bits(0b0000'1111, 4);

	std::array<std::array<u32, 3>, 3> grid{1, 2, 3, 4, 5, 6, 7, 8, 9};

	int rows = 3;
	int cols = 4;

	std::vector<std::vector<int>> grid2(rows, std::vector<int>(cols, 0));

	grid2[0][0] = 10;

#if 0
	{
		constexpr int r = 3;
		constexpr int c = 4;

		// Create a contiguous memory block
		std::vector<int> data(r * c);

		// Create an mdspan view over the data
		// std::mdspan<int, 2> grid(data.data(), r, c);
		auto ms2 = std::mdspan(data.data(), r, c);

		// Write data using 2D view
		for (std::size_t i = 0; i != ms2.extent(0); i++)
			for (std::size_t j = 0; j != ms2.extent(1); j++)
				ms2[std::array{i, j}] = i * 1'000 + j;

		// Read back using 3D view
		for (std::size_t i = 0; i != ms2.extent(0); i++)
		{
			for (std::size_t j = 0; j != ms2.extent(1); j++)
			{
				dbg::print("{:4} ", ms2[std::array{i, j}]);
			}
			dbg::println("");
		}

		// Access and modify elements
		// grid[1][2] = 5; // Set element at row 1, column 2 to 5
		//
		//// Print the grid (for demonstration)
		// for (int i = 0; i < rows; ++i)
		//{
		//	for (int j = 0; j < cols; ++j)
		//	{
		//		std::cout << grid[i][j] << " ";
		//	}
		//	std::cout << std::endl;
		// }
	}
#endif

	deckard::initialize();


	auto resolve = net::hostname_to_ip("www.taboobuilder.com", net::protocol::v6);
	dbg::println("www.taboobuilder.com: {}", resolve ? *resolve : "<failed>");

	resolve = net::hostname_to_ip("www.taboobuilder.com", net::protocol::v4);
	dbg::println("www.taboobuilder.com: {}", resolve ? *resolve : "<failed>");


	bignum a("13221321321321321321321321321321321321");
	bignum b("4654654654654654654654654654654654654654");

	bignum result = a.add(a, b);


	// TODO:formatter does not work yet
	dbg::println("{}, {}", a.print(), 0);
	dbg::println("{}, {}", b.print(), 0);
	dbg::println("{}, {}", result.print(), 0);
	dbg::println("{}", "4667875975975975975975975975975975975975");

	int k = 0;
	{
		app::app app01;

		app01.run();
	}

	k = 0;
#if 0

	float x1 = 0.0f;
	float x2 = 10.0f;

	for (float x = 0.0f; x < 1.0f; x += 0.01f)
	{
		float dt = x;
		dbg::println(
		  "dt({:.5f}). lerp({:.5f}), d({:.5f}), ss({:.5f}",
		  dt,
		  lerp(x1, x2, smoothstep(bounce_ease_out(dt))),
		  lerp(x1, x2, smoothstep(dt)),
		  lerp(x1, x2, bounce_ease_in(dt))

		);
	}


	utf8file file("func.txt");

	lexer::tokenizer initok(file.data());
	{
		ScopeTimer _("func tokenizer took");
		auto       initokens = initok.tokenize();
		dbg::println("ini tokens: {}", initokens.size());
	}

	using inivalue = std::variant<std::monostate, bool, i64, u64, double, std::string>;

	struct value
	{
		inivalue         v;
		std::string_view key; // section@key, section cannot have @ sign
		std::string_view comment;
	};

	std::vector<value> values;
	// read in-order to vector, store section@key
	// when writing back to file, for-loop, detect section name changes.


	lexer::tokenizer l1("float returning{0.0}"sv);
	tokens = l1.tokenize();


	// filemonitor fmon("folder");
	//
	// fmon.add("folder2");
	// fmon.callback([](const status s, const fs::path &file
	//  - status: created, deleted, modified,


	dbg::println("{:08b}", bitmask(4));
	dbg::println("{:08b}", bitmask(4, 1));
	dbg::println("{:08b}", bitmask(2));
	dbg::println("{:032b}", bitmask<u32>(5));


	// auto d = Chain().set(123).execute().close();
	// int  k = 0;
	/*
		enum { Startup, Exit, First, Last, PreUpdate, Update, PostUpdate, FixedUpdate}

		void setup()
		void load()
		vpod mainloop()
		void exit()

		auto app = deckard::app();

		app.add(App::Startup, setup)
		   .add(App::Startup, load)

		   .add(App::Startup, setup, load) <- variadic

		   .add(App::Update: mainloop)
		   .add(App::Exit, exit)
		   .run();




	*/

	std::vector<u32> adata;
	adata.push_back(0x1122'3344);
	adata.push_back(0x5566'7788);


	// auto bbs2 = std::as_bytes(std::span{adata});
	// auto bbs3 = std::as_writable_bytes(std::span{adata});
	//
	// auto header = std::as_bytes(std::span{bbs3.first(4)});
	// u64  hhe    = load_as<u32>(header);
	// u64  hhe2   = load_as<u32>(header.data());

	// load_as<char, 16>(header); //
	// subspan => string


	auto count_colons = [](std::string_view input) -> i32
	{
		i32 colons{0};
		i32 doublecolon{0};
		i32 i{0};
		while (i < input.size())
		{
			if (input[i] == ':')
			{
				if ((i + 1) < input.size() and input[i + 1] == ':')
				{
					doublecolon += 1;
					i += 1;
				}
				else
					colons += 1;
			}
			i += 1;
		}

		return colons;
	};

	dbg::println("0 == {}", count_colons("::1"));
	dbg::println("2 == {}", count_colons("00:00::1"));
	dbg::println("2 == {}", count_colons("0:0:0::1"));


	auto longest_zero_run = [](std::string_view input) -> i32
	{
		i32 max_zeros           = 0;
		i32 max_zeros_index     = -1;
		i32 current_zeros       = 0;
		i32 current_zeros_index = -1;

		for (auto [i, digit] : std::views::enumerate(input))
		{
			if (digit == '0')
			{
				if (current_zeros == 0)
					current_zeros_index = as<i32>(i);
				current_zeros += 1;
			}
			else
			{
				if (current_zeros > max_zeros)
				{
					max_zeros       = current_zeros;
					max_zeros_index = current_zeros_index;
				}
				current_zeros = 0;
			}
		}

		if (current_zeros > max_zeros)
		{
			max_zeros       = current_zeros;
			max_zeros_index = current_zeros_index;
		}

		return max_zeros_index;
	};

	cpuid::CPUID id;
	dbg::println("{}", id.as_string());

	dbg::println("RAM: {}", system::GetRAMString());

	dbg::println("OS: {}", system::GetOSVersionString());

	dbg::println("4 == {}", longest_zero_run("12340000"));
	dbg::println("6 == {}", longest_zero_run("120034000012"));
	dbg::println("0 == {}", longest_zero_run("0000100020010"));
	dbg::println("9 == {}", longest_zero_run("01002000300004"));
	//                                        0123456789
	dbg::println("6 == {}", longest_zero_run("1200340000560007"));
	dbg::println("2 == {}", longest_zero_run("12000055000021"));


	IpAddress ipv6;
	ipv6.version(0);
	ipv6.port(0xB97A);


	IpAddress ipv4;
	ipv4 = ipv6;


	std::array keywords{"if"sv, "while"sv, "for"sv};

	auto add_keywords = [](const std::span<std::string_view> words)
	{
		for (const auto& word : words)
			dbg::println("adding keyword '{}'", word);
	};

	add_keywords(keywords);

	dbg::println("ipaddress size {}", sizeof(IpAddress));

	IPv6Address addr;

	addr.fromString("2606:2800:220:1:248:1893:25c8:1946");
	addr.print();

	// 2606:2800:0220:0001:0248:1893:25c8:1946
	//     1    2    3    4    5    6    7
	// 2001:0db8:0000:0000:0001:0000:0000:0001
	// 2001:0db8::1:0:0:1
	//     6       5 4 3
	addr.fromString("2001:0db8::1:0:0:1");
	addr.print();

	addr.fromString("1000::1");
	addr.print();

	addr.fromString("::1");
	addr.print();


	dbg::println("{}", time_as_string());
	dbg::println("{}", date_as_string());
	dbg::println("{}", timezone());


	std::println("Script Compiler v5");
#endif
	deckard::deinitialize();

	return 0;
}
