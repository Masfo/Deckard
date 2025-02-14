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


using crt_callback = int(void);

int PreMain1(void)
{
	OutputDebugStringA("premain");
	return 0;
}

int PreMain2(void)
{
	OutputDebugStringA("premain");
	return 0;
}

#pragma data_seg(".CRT$XIAC")
static crt_callback* autostart[] = {PreMain1, PreMain2};
#pragma data_seg() /* reset data-segment */


enum class ConvertEpoch : u64
{
	Microseconds = 10,
	Milliseconds = Microseconds * 1000,
	Seconds      = Milliseconds * 1000,
};

std::string from_epoch(u64 epoch, ConvertEpoch mul = ConvertEpoch::Seconds)
{
	u64 t{epoch};
	t *= as<u64>(mul);
	t += 116444'736000'000000LL;

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

template<typename T, size_t SIZE>
class sso_buffer
{
private:
	T*  m_data{nullptr};
	T   m_ssodata[SIZE]{0};
	u32 m_capacity{SIZE};
	u32 m_length{1};

	void init_buffer(u32 capacity)
	{
		m_data = reinterpret_cast<T*>(std::malloc(capacity * sizeof(T)));
		assert::check(m_data != nullptr);

		m_capacity = capacity;
	}

	T* get_storage() const { return const_char<T*>(m_capacity == SIZE ? m_ssodata : m_data); }

	void resize(u32 new_capacity)
	{
		if (new_capacity == m_capacity)
			return;

		if (not m_data)
		{
			m_data = reinterpret_cast<T*>(::malloc(new_capacity * sizeof(T)));
			if (m_data)
				m_capacity = new_capacity;

			return;
		}
		assert::check(new_capacity >= m_length);

		m_data = reinterpret_cast<T*>(::realloc(m_data, new_capacity * sizeof(T)));
		if (m_data)
		{
			m_capacity = new_capacity;
		}
	}

	void release()
	{
		if (m_data)
		{
			::free(m_data);
			m_data = nullptr;
		}

		m_length   = 1;
		m_capacity = 1;
	}


public:
};

int deckard_main(std::string_view commandline)
{


#ifndef _DEBUG
	std::print("dbc {} ({}), ", dbc::build::version_string, dbc::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif


	// at, []
	// at<u32>, at<u8> template
	// size, count, size_in_bytes
	// iterator
	// contains, find
	// ==, !=
	// sizeof == 32

#if 0
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
#endif

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
				ms2[std::array{i, j}] = i * 1000 + j;

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


	auto resolve_ip = [](std::string_view url)
	{
		auto resolve = net::hostname_to_ip(url, net::protocol::v4);
		dbg::println("ipv4: {}, {}", url, resolve ? *resolve : "failed to resolve");

		resolve = net::hostname_to_ip(url, net::protocol::v6);
		dbg::println("ipv6: {}, {}", url, resolve ? *resolve : "failed to resolve");
	};

	resolve_ip("www.taboobuilder.com");
	resolve_ip("localhost");




	int k = 0;
	{
		app::vulkanapp app01;

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

	return 0;
}
