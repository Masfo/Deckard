#include <Windows.h>

import std;
import Deckard;

#ifndef _DEBUG
import scbuild;
#endif

using namespace std::string_view_literals;
using namespace deckard;
using namespace deckard::utils;

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


enum class ConvertEpoch : u64
{
	Microseconds = 10,
	Milliseconds = Microseconds * 1000,
	Seconds      = Milliseconds * 1000,
};

std::string from_epoch(u64 epoch, ConvertEpoch mul = ConvertEpoch::Seconds) noexcept
{
	u64 t{epoch};
	t *= as<u64>(mul);
	t += 11'6444'7360'0000'0000LL;

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

	bool fromString(const char *addrstr);
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

bool IPv6Address::fromString(const char *addrstr)
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
			_address[pos + 1] = accumulator;
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

class IpAddress
{
public:
	explicit IpAddress(std::string_view address) { }

private:
	std::array<u16, 8> address{};
	u16                port{0};
};

constexpr auto IpSize = sizeof(IpAddress);

int main()
{
#ifndef _DEBUG
	std::print("sc {} ({}), ", scbuild::version_string, scbuild::calver);
	std::println("deckard {} ({})", DeckardBuild::version_string, DeckardBuild::calver);
#endif


	std::array keywords{"if"sv, "while"sv, "for"sv};

	auto add_keywords = [](std::span<std::string_view> words)
	{
		for (const auto &word : words)
			dbg::println("adding word '{}'", word);
	};

	add_keywords(keywords);


	dbg::println("{}", sizeof(IpAddress));

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

	{
		std::string inp{"foob"};
		auto        foob_e = base64::encode_str(inp, base64::padding::no);
		auto        foob_d = base64::decode_str(foob_e);

		dbg::println("'{}': '{}' - '{}'", inp, foob_e, foob_d);
	}

	u8 C1 = 0b1000'1010;
	u8 L1 = 0b1100'1111;
	dbg::println("{:0b} - {}", C1, std::countl_one(C1));
	dbg::println("{:0b} - {}", L1, std::countl_one(L1));

	auto nowTime = std::chrono::system_clock::now();

	u64 ms_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime.time_since_epoch()).count() & 0xFFFF'FFFF'FFFF;
	u32 s_epoch  = std::chrono::duration_cast<std::chrono::seconds>(nowTime.time_since_epoch()).count() & 0xFFFF'FFFF;


	dbg::println("ms {:>14} => {}", ms_epoch, from_epoch(ms_epoch, ConvertEpoch::Milliseconds));
	dbg::println(" s {:>14} => {}", s_epoch, from_epoch(s_epoch, ConvertEpoch::Seconds));
	dbg::println("v7 {:>14} => {}", 0x018f'0ad8'1600, from_epoch(0x018f'0ad8'1600, ConvertEpoch::Milliseconds));
	dbg::println();

	auto const time     = std::chrono::current_zone()->to_local(nowTime);
	auto       zone     = std::chrono::current_zone()->get_info(nowTime);
	auto       zonename = std::chrono::current_zone()->name();

	dbg::println(" n {:>14} => {} # {}", "", nowTime.time_since_epoch().count(), time);
	dbg::println(" 1 {0:>14} => {1:%Y}-{1:%m}-{1:%d} {1:%OH}:{1:%M}:{1:%OS}", "", time);
	dbg::println(" 2 {0:>14} => {1:%F} {1:%T}", "", time);
	dbg::println(" 3 {:>14} => {} - {}", "", zone.abbrev, zonename);


	std::println("Script Compiler v5");

	return 0;
}
