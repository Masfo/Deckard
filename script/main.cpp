#include <Windows.h>

import std;
import Deckard;

using namespace std::string_view_literals;
using namespace deckard;
using namespace deckard::utils;

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

struct Test
{
	u64 data{};
	u64 data2;
	u64 data3;
	u64 data4;
	u64 data5;
};

int main()
{
	u8 C1 = 0b1000'1010;
	u8 L1 = 0b1100'1111;
	{
		std::string inp{"foob"};
		auto        foob_e = base64::encode_str(inp, base64::padding::no);
		auto        foob_d = base64::decode_str(foob_e);


		dbg::println("'{}': '{}' - '{}'", inp, foob_e, foob_d);
	}

	u64 a1 = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() &
			  0xFFFF'FFFF'FFFF)
			 << 16;
	u64 mask = 0x1234;

	u64 result = a1 | mask;

	dbg::println("{}/{}", sizeof(Test), sizeof(std::unique_ptr<Test>));


	dbg::println("{:0b} - {}", C1, std::countl_one(C1));
	dbg::println("{:0b} - {}", L1, std::countl_one(L1));

	auto nowTime = std::chrono::system_clock::now();

	u64 ms_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime.time_since_epoch()).count() & 0xFFFF'FFFF'FFFF;
	u32 s_epoch  = std::chrono::duration_cast<std::chrono::seconds>(nowTime.time_since_epoch()).count() & 0xFFFF'FFFF;


	dbg::println("ms {:>14} => {}", ms_epoch, from_epoch(ms_epoch, ConvertEpoch::Milliseconds));
	dbg::println(" s {:>14} => {}", s_epoch, from_epoch(s_epoch, ConvertEpoch::Seconds));
	dbg::println("v7 {:>14} => {}", 0x018f'0ad8'1600, from_epoch(0x018f'0ad8'1600, ConvertEpoch::Milliseconds));


	auto const time     = std::chrono::current_zone()->to_local(nowTime);
	auto       zone     = std::chrono::current_zone()->get_info(nowTime);
	auto       zonename = std::chrono::current_zone()->name();

	dbg::println(" n {:>14} => {} # {}", "", nowTime.time_since_epoch().count(), time);
	dbg::println(" 1 {0:>14} => {1:%Y}-{1:%m}-{1:%d} {1:%OH}:{1:%M}:{1:%OS}", "", time);
	dbg::println(" 2 {0:>14} => {1:%F} {1:%T}", "", time);
	dbg::println(" 3 {:>14} => {} - {}", "", zone.abbrev, zonename);


	u32 major = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x026C); // NtMajorVersion, 4.0+
	u32 minor = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x0270); // NtMinorVersion, 4.0+

	u32 build = 0;
	if (major >= 10)
		build = *(u32 *)(KUSER_SHARED_DATA_PTR + 0x0260); // NtBuildNumber, 10.0+

	dbg::println("Winver: {}.{}.{}", major, minor, build);


	dbg::println("{:<20f}", std::numeric_limits<float>::max());
	dbg::println("{:<20}", std::numeric_limits<u64>::max());


	i64 num1 = 922'3372'0368'5477'5807i64; // Max value of 64-bit signed integer
	i64 num2 = 1;

	if (multiplyOverflow(num1, num2))
		dbg::println("overflow");
	else
		dbg::println("ok");


	//	std::println("{}", readAndWrite);

	std::println("Script Compiler v5");

	return 0;
}
