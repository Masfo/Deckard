
#include <windows.h>
#include <Commctrl.h>
#include <cmath>
#include <gameinput.h>
#include <intrin.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstddef>


import std;
import deckard;
import deckard.app2;
import deckard.types;
import deckard.helpers;
import deckard.math;
import deckard.timers;
import deckard.vec;

using namespace deckard;
using namespace deckard::literals;

// using namespace std::string_literals;
namespace fs = std::filesystem;
using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace std::chrono_literals;

using namespace deckard::vec;


#ifndef _DEBUG
import window;
#endif

std::array<unsigned char, 256> previous_state{0};
std::array<unsigned char, 256> current_state{0};


#if 0
void keyboard_callback([[maybe_unused]]vulkanapp& app, [[maybe_unused]] i32 key, [[maybe_unused]] i32 scancode, [[maybe_unused]] bool action, [[maybe_unused]] i32 mods)
{
	dbg::println("key: {:#x} - {:#x}, {} - {}", key, scancode, action ? "UP" : "DOWN", mods);

	bool up = action;


	// ★☆
	//
	// num 6 -> 0x66, 0x4d
	// f     -> 0x46, 0x21

	if (up and key == Key::B)
	{
		current_state.swap(previous_state);
		{
			ScopeTimer<std::micro> _("state");
			GetKeyboardState(&current_state[0]);
		}

		// auto shift = current[Key::Shift];
	}

	if (key == Key::Escape and up)
	{
		dbg::println("quit");
		app.quit();
	}

	if (key == Key::F1 and up)
	{
		app.toggle_vsync();
	}

	if (key == Key::V and up)
	{
		app.toggle_vsync();
	}

	if (key == Key::F2 and up)
	{
		app.resize(1280, 720);
	}

	if (up and (key == Key::F11 or key == Key::F))
	{
		//	app.set(Attribute::togglefullscreen);
		app.toggle_fullscreen();
	}

	if (key == Key::Numpad1 and up)
	{
		// app.set(Attribute::gameticks, 60u);
		dbg::println("ticks 60");
	}
	if (key == Key::Numpad2 and up)
	{
		// app.set(Attribute::gameticks, 30u);
		dbg::println("ticks 30");
	}

	if (key == Key::Numpad3 and up)
	{
		// app.set(Attribute::gameticks, 5u);
		dbg::println("ticks 5");
	}

	if (key == Key::Add and up)
	{
		// u32 newticks = app.get(Attribute::gameticks) * 2;
		// app.set(Attribute::gameticks, newticks);
		// dbg::println("ticks {}", newticks);
	}

	if (key == Key::Subtract and up)
	{
		// u32 newticks = app.get(Attribute::gameticks) / 2;
		// app.set(Attribute::gameticks, newticks == 0 ? 1 : newticks);
		// dbg::println("ticks {}", newticks);
	}
}

void fixed_update(vulkanapp&, f32 /*fixed_delta*/)
{
	//
}

void update(vulkanapp&, f32 /*delta*/)
{
	//
}

void render(vulkanapp&)
{
	//
}

#endif
constexpr std::array<u32, 64> k_md5 = []
{
	std::array<u32, 64> table = {};
#ifdef __cpp_lib_constexpr_cmath
#error ("use std::sin")
	for (u32 i : table)
		table[i] = static_cast<u32>(std::floor(0x1'0000'0000 * std::fabs(std::sin(i + 1))));
#else
#endif
	return table;
}();


struct Coord
{
	int            x{};
	int            y{};
	constexpr bool operator==(const Coord& rhs) const = default;
};

struct Iter2D
{
public:
	Iter2D(int x, int y)
		: size(x, y)
	{
	}

	Coord current{};

	const Coord& operator*() const { return current; }

	constexpr bool operator==(const Iter2D& rhs) const = default;

	Iter2D& operator++()
	{
		++current.x;
		if (current.x >= size.x)
		{
			current.x = 0;
			++current.y;
		}
		return *this;
	}

	Iter2D begin() const { return Iter2D{size, {}}; }

	Iter2D end() const { return Iter2D{size, {0, size.y}}; }

private:
	Coord size{};

	Iter2D(Coord size_, Coord current_)
		: size(size_)
		, current(current_)
	{
	}
};

#if 1
template<typename T>
struct Tree
{
	T     value;
	Tree *left{}, *right{};

	std::generator<const T&> traverse_inorder() const
	{
		if (left)
			co_yield std::ranges::elements_of(left->traverse_inorder());

		co_yield value;

		if (right)
			co_yield std::ranges::elements_of(right->traverse_inorder());
	}
};
#endif


struct NtpPacket
{
	u8 leapIndicator{}; // 2 bits
	u8 version{};       // 3 bits
	u8 mode{};          // 3 bits

	u8                        stratum{};
	std::chrono::milliseconds poll{};
	std::chrono::nanoseconds  precision{};

	std::chrono::milliseconds root_delay;
	std::chrono::milliseconds root_dispersion{};

	u32         refId{};
	std::string ref_id_string;

	u32 unix_epoch{0};

	std::chrono::system_clock::time_point refTimestamp{};
	std::chrono::system_clock::time_point origTimestamp{};
	std::chrono::system_clock::time_point rxTimestamp{};
	std::chrono::system_clock::time_point txTimestamp{};


	std::chrono::milliseconds roundtrip_delay;
	std::chrono::milliseconds local_clock_offset;
};

std::chrono::system_clock::time_point ntp_to_chrono(u64 ntp_ts)
{

	using namespace std::chrono;
	constexpr u32 NTP_UNIX_OFFSET = 2'208'988'800u;

	u32 seconds  = static_cast<u32>(ntp_ts >> 32);
	u32 fraction = static_cast<u32>(ntp_ts & 0xFFFF'FFFFull);


	f64  frac_seconds = static_cast<f64>(fraction) / 4294967296.0;
	auto secs         = seconds - NTP_UNIX_OFFSET;

	auto ntp_time = std::chrono::system_clock::time_point{
	  std::chrono::seconds(secs) + duration_cast<std::chrono::system_clock::duration>(duration<f64>(frac_seconds))};

	return ntp_time;
}

u64 chrono_to_ntp(std::chrono::system_clock::time_point tp)
{
	using namespace std::chrono;
	constexpr u32 NTP_UNIX_OFFSET = 2'208'988'800u;

	auto duration = tp.time_since_epoch();
	auto secs     = duration_cast<seconds>(duration).count();
	auto frac     = duration - seconds(secs);

	u32 ntp_secs = static_cast<u32>(secs + NTP_UNIX_OFFSET);
	u32 ntp_frac = static_cast<u32>((static_cast<f64>(frac.count()) / seconds(1).count()) * 4294967296.0);

	return (static_cast<u64>(ntp_secs) << 32) | ntp_frac;
}

u32 to_unix_epoch(u64 ntp_ts)
{
	using namespace std::chrono;
	constexpr u32 NTP_UNIX_OFFSET = 2'208'988'800u;

	u32 seconds = static_cast<u32>(ntp_ts >> 32);
	u32 secs    = seconds - NTP_UNIX_OFFSET;
	return secs;
}

NtpPacket parse_ntp(
  std::span<const u8> raw, std::chrono::system_clock::time_point t1, std::chrono::system_clock::time_point t4)
{
	if (raw.size() < 48)
		return {};

	NtpPacket pkt{};

	u8 li_vn_mode     = raw[0];
	pkt.leapIndicator = (li_vn_mode >> 6) & 0x03;
	pkt.version       = (li_vn_mode >> 3) & 0x07;
	pkt.mode          = li_vn_mode & 0x07;

	pkt.stratum = raw[1];

	// Poll: signed exponent, interval = 2^poll seconds
	i8  poll         = static_cast<i8>(raw[2]);
	f64 poll_seconds = std::ldexp(1.0, poll);
	pkt.poll         = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<f64>(poll_seconds));

	// Precision: signed exponent, resolution = 2^raw_precision seconds
	i8  raw_precision     = static_cast<i8>(raw[3]);
	f64 precision_seconds = std::ldexp(1.0, raw_precision);
	pkt.precision = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<f64>(precision_seconds));


	// pkt.rootDelay =
	u32 root_delay = load_as_be<u32>(raw, 4).value_or(0);
	{
		i16 int_part  = root_delay >> 16;
		u16 frac_part = root_delay & 0xFFFF;

		f64 frac_seconds       = static_cast<f64>(frac_part) / 65536.0;
		f64 root_delay_seconds = int_part + frac_seconds;

		pkt.root_delay = std::chrono::duration_cast<std::chrono::milliseconds>(
		  std::chrono::duration<f64>(root_delay_seconds));
	}


	// Root dispersion
	u32 root_dispersion = load_as_be<u32>(raw, 8).value_or(0);
	{
		u16 int_part  = root_dispersion >> 16;
		u16 frac_part = root_dispersion & 0xFFFF;

		f64 frac_seconds            = static_cast<f64>(frac_part) / 65536.0;
		f64 root_dispersion_seconds = int_part + frac_seconds;
		pkt.root_dispersion         = std::chrono::duration_cast<std::chrono::milliseconds>(
		  std::chrono::duration<f64>(root_dispersion_seconds));
	}


	pkt.refId = load_as_be<u32>(raw, 12).value_or(0);

	if (pkt.stratum == 0 or pkt.stratum == 1)
	{
		// stratum 0 Kiss of Death,

		char chars[4] = {
		  static_cast<char>((pkt.refId >> 24) & 0xFF),
		  static_cast<char>((pkt.refId >> 16) & 0xFF),
		  static_cast<char>((pkt.refId >> 8) & 0xFF),
		  static_cast<char>(pkt.refId & 0xFF)};

		pkt.ref_id_string = std::string(chars, chars + 4);
		if (pkt.ref_id_string.ends_with('\0'))
			pkt.ref_id_string.resize(pkt.ref_id_string.size() - 1);
	}
	else if (pkt.stratum >= 2 and pkt.stratum <= 15)
	{
		u32 ip            = pkt.refId;
		pkt.ref_id_string = std::format(
		  "{}.{}.{}.{} ({:X})", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, ip);
	}
	else if (pkt.stratum > 16)
	{
		pkt.ref_id_string = "Reserved/Unknown";
	}


	pkt.unix_epoch = to_unix_epoch(load_as_be<u64>(raw, 40).value_or(0));

	pkt.refTimestamp  = ntp_to_chrono(load_as_be<u64>(raw, 16).value_or(0));
	pkt.origTimestamp = ntp_to_chrono(load_as_be<u64>(raw, 24).value_or(0));
	pkt.rxTimestamp   = ntp_to_chrono(load_as_be<u64>(raw, 32).value_or(0));
	pkt.txTimestamp   = ntp_to_chrono(load_as_be<u64>(raw, 40).value_or(0));

	auto t2 = pkt.rxTimestamp;
	auto t3 = pkt.txTimestamp;

	pkt.roundtrip_delay    = std::chrono::duration_cast<std::chrono::milliseconds>((t4 - t1) - (t3 - t2));
	pkt.local_clock_offset = std::chrono::duration_cast<std::chrono::milliseconds>(((t2 - t1) + (t3 - t4)) / 2);


	auto                          tz = std::chrono::current_zone();
	const std::chrono::zoned_time zt{tz, pkt.origTimestamp};
	dbg::println("Local zone: {}", zt);

	// auto local_time = zt.get_local_time();
	// std::chrono::system_clock::time_point local_tp{zt.get_local_time()};

	// Create a time point in a specific time zone, e.g., Tokyo
	// const auto*                   tz2 = std::chrono::locate_zone("Asia/Tokyo");
	// const std::chrono::zoned_time tokyo_time{tz, pkt.origTimestamp};
	//
	//// Extract the system_clock::time_point from the zoned_time
	// const auto sys_tp   = tokyo_time.get_sys_time();
	// const auto local_tp = tokyo_time.get_local_time();
	//
	//// Print the times to demonstrate the conversion
	// dbg::println("Original zoned_time (in Tokyo): {}", std::format("{:%Y-%m-%d %H:%M:%S %Z}", tokyo_time));
	//
	// dbg::println("Converted time_point (UTC): {}", sys_tp);
	// dbg::println("Converted time_point (UTC): {}", local_tp);
	//
	// std::chrono::local_time local_new = tokyo_time.get_local_time();
	// dbg::println("Converted time_point (UTC): {}", local_new);


	return pkt;
}

/*
* The message type field is decomposed further into the following
   structure:

						0                 1
						2  3  4 5 6 7 8 9 0 1 2 3 4 5

					   +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
					   |M |M |M|M|M|C|M|M|M|C|M|M|M|M|
					   |11|10|9|8|7|1|6|5|4|0|3|2|1|0|
					   +--+--+-+-+-+-+-+-+-+-+-+-+-+-+

				Figure 3: Format of STUN Message Type Field

   Here the bits in the message type field are shown as most significant (M11)
   through least significant (M0).  M11 through M0 represent a 12-
   bit encoding of the method.  C1 and C0 represent a 2-bit encoding of
   the class.  A class of 0b00 is a request, a class of 0b01 is an
   indication, a class of 0b10 is a success response, and a class of
   0b11 is an error response.  This specification defines a single
   method, Binding.  The method and class are orthogonal, so that for
   each method, a request, success response, error response, and
   indication are possible for that method.  Extensions defining new
   methods MUST indicate which classes are permitted for that method.

   For example, a Binding request has class=0b00 (request) and
   method=0b000000000001 (Binding) and is encoded into the first 16 bits
   as 0x0001.  A Binding response has class=0b10 (success response) and
   method=0b000000000001, and is encoded into the first 16 bits as
   0x0101.
*/


struct STUNHeader
{
	u16                type{0};
	u16                length{0};
	u32                cookie{0x2112'A442};
	std::array<u8, 12> transaction_id{};

	bool is_zero() const { return (type >> 14) == 0; }

	u8 class_bits() const { return ((type >> 4) & 0x02) | ((type >> 7) & 0x01); }

	u16 method_bits() const { return ((type >> 2) & 0xF80) | ((type >> 1) & 0x70) | (type & 0x0F); }
};

template<std::integral T = u64>
[[nodiscard]] auto encode_integer_to_array(T value) -> std::pair<u8, std::array<u8, (sizeof(T) * 8 + 6) / 7>>
{
	static constexpr u64 capacity = (sizeof(T) * 8 + 6) / 7;

	std::array<u8, capacity> temp{};

	u8 len{};
	while (value > 0x7Fu)
	{
		temp[len++] = static_cast<u8>((value & 0x7Fu) | 0x80u);
		value >>= 7;
	}
	temp[len++] = static_cast<u8>(value);
	return {len, temp};
}

template<std::integral T = u64>
[[nodiscard]] auto encode_integer(T value, std::span<u8> output) -> u64
{
	std::array<u8, 10> temp{};

	u64 len{};

	while (value > 0x7Fu)
	{
		temp[len++] = static_cast<u8>((value & 0x7Fu) | 0x80u);
		value >>= 7;
	}
	temp[len++] = static_cast<u8>(value);

	assert::check(
	  output.size() >= temp.size(),
	  std::format(
		"Output buffer too small for encoded integer ({} bytes needed, {} bytes available)", temp.size(), output.size()));

	std::ranges::copy_n(temp.begin(), len, output.begin());

	return len;
}

template<std::integral T = u64>
T decode_integer(std::span<const u8> input)
{
	T value{};
	for (u64 i{}; i < input.size(); ++i)
	{
		value |= static_cast<T>(input[i] & 0x7F) << (7 * i);
		if ((input[i] & 0x80) == 0)
			break;
	}
	return value;
}

void generate(std::span<u8> buffer, u64 percentage_how_compressable) noexcept
{
	percentage_how_compressable = std::min<u64>(percentage_how_compressable, 100);

	std::mt19937_64                    rng{std::random_device{}()};
	std::uniform_int_distribution<u32> byte_dist(0, 255);

	constexpr usize block_size = 4096;

	usize offset = 0;
	while (offset < buffer.size())
	{
		const usize this_block         = std::min(block_size, buffer.size() - offset);
		const usize compressible_bytes = (this_block * percentage_how_compressable) / 100;

		auto block = buffer.subspan(offset, this_block);

		std::ranges::fill(block.first(compressible_bytes), u8{0xAA});

		for (u8& byte : block.subspan(compressible_bytes))
			byte = static_cast<u8>(byte_dist(rng));

		offset += this_block;
	}
}

struct alignas(u64) TestStruct
{
	i64 d{std::byteswap(0x1122'3344'5566'7788)};
	u32 c{0xAAAA'AAAA};
	i16 b{0x5555};
	u8  a{0xFF};
};

class test_class
{
public:
	auto& flag(this auto&& self, int i)
	{
		self.flags.push_back(i);
		return self;
	};

	auto& option(this auto&& self, int i)
	{
		self.flags.push_back(i);
		return self;
	};


private:
	std::vector<int> flags;
};

[[nodiscard]] auto label_device_family(GameInputDeviceFamily family) noexcept -> std::string
{
	std::string result;
	if (family == GameInputFamilyVirtual)
		result += "Virtual, ";
	if (family == GameInputFamilyAggregate)
		result += "Aggregate, ";
	if (family == GameInputFamilyXboxOne)
		result += "Xbox One, ";
	if (family == GameInputFamilyXbox360)
		result += "Xbox 360, ";
	if (family == GameInputFamilyHid)
		result += "HID, ";
	if (family == GameInputFamilyI8042)
		result += "I8042, ";
	if (result.empty())
		return "None";

	return result.substr(0, result.size() - 2); // Remove trailing ", "
}

[[nodiscard]] auto label_device_capability(GameInputDeviceCapabilities cap) noexcept -> std::string
{

	std::string result;
	if (cap & GameInputDeviceCapabilityAudio)
		result += "Audio, ";
	if (cap & GameInputDeviceCapabilityPluginModule)
		result += "Plugin Module, ";
	if (cap & GameInputDeviceCapabilityPowerOff)
		result += "Power Off, ";
	if (cap & GameInputDeviceCapabilitySynchronization)
		result += "Synchronization, ";
	if (cap & GameInputDeviceCapabilityWireless)
		result += "Wireless, ";
	if (result.empty())
		return "None";

	return result.substr(0, result.size() - 2); // Remove trailing ", "
}

[[nodiscard]] auto label_name(GameInputLabel label) noexcept -> std::string_view
{
	switch (label)
	{
		case GameInputLabelUnknown: return "Unknown";
		case GameInputLabelNone: return "None";

		case GameInputLabelXboxGuide: return "Xbox Guide";
		case GameInputLabelXboxBack: return "Xbox Back";
		case GameInputLabelXboxStart: return "Xbox Start";
		case GameInputLabelXboxMenu: return "Xbox Menu";
		case GameInputLabelXboxView: return "Xbox View";
		case GameInputLabelXboxA: return "Xbox A";
		case GameInputLabelXboxB: return "Xbox B";
		case GameInputLabelXboxX: return "Xbox X";
		case GameInputLabelXboxY: return "Xbox Y";
		case GameInputLabelXboxDPadUp: return "Xbox DPad Up";
		case GameInputLabelXboxDPadDown: return "Xbox DPad Down";
		case GameInputLabelXboxDPadLeft: return "Xbox DPad Left";
		case GameInputLabelXboxDPadRight: return "Xbox DPad Right";
		case GameInputLabelXboxLeftShoulder: return "Xbox Left Shoulder";
		case GameInputLabelXboxLeftTrigger: return "Xbox Left Trigger";
		case GameInputLabelXboxLeftStickButton: return "Xbox Left Stick Click";
		case GameInputLabelXboxRightShoulder: return "Xbox Right Shoulder";
		case GameInputLabelXboxRightTrigger: return "Xbox Right Trigger";
		case GameInputLabelXboxRightStickButton: return "Xbox Right Stick Click";
		case GameInputLabelXboxPaddle1: return "Xbox Paddle 1";
		case GameInputLabelXboxPaddle2: return "Xbox Paddle 2";
		case GameInputLabelXboxPaddle3: return "Xbox Paddle 3";
		case GameInputLabelXboxPaddle4: return "Xbox Paddle 4";

		case GameInputLabelLetterA: return "A";
		case GameInputLabelLetterB: return "B";
		case GameInputLabelLetterC: return "C";
		case GameInputLabelLetterD: return "D";
		case GameInputLabelLetterE: return "E";
		case GameInputLabelLetterF: return "F";
		case GameInputLabelLetterG: return "G";
		case GameInputLabelLetterH: return "H";
		case GameInputLabelLetterI: return "I";
		case GameInputLabelLetterJ: return "J";
		case GameInputLabelLetterK: return "K";
		case GameInputLabelLetterL: return "L";
		case GameInputLabelLetterM: return "M";
		case GameInputLabelLetterN: return "N";
		case GameInputLabelLetterO: return "O";
		case GameInputLabelLetterP: return "P";
		case GameInputLabelLetterQ: return "Q";
		case GameInputLabelLetterR: return "R";
		case GameInputLabelLetterS: return "S";
		case GameInputLabelLetterT: return "T";
		case GameInputLabelLetterU: return "U";
		case GameInputLabelLetterV: return "V";
		case GameInputLabelLetterW: return "W";
		case GameInputLabelLetterX: return "X";
		case GameInputLabelLetterY: return "Y";
		case GameInputLabelLetterZ: return "Z";
		case GameInputLabelNumber0: return "0";
		case GameInputLabelNumber1: return "1";
		case GameInputLabelNumber2: return "2";
		case GameInputLabelNumber3: return "3";
		case GameInputLabelNumber4: return "4";
		case GameInputLabelNumber5: return "5";
		case GameInputLabelNumber6: return "6";
		case GameInputLabelNumber7: return "7";
		case GameInputLabelNumber8: return "8";
		case GameInputLabelNumber9: return "9";

		case GameInputLabelArrowUp: return "Arrow Up";
		case GameInputLabelArrowUpRight: return "Arrow Up Right";
		case GameInputLabelArrowRight: return "Arrow Right";
		case GameInputLabelArrowDownRight: return "Arrow Down Right";
		case GameInputLabelArrowDown: return "Arrow Down";
		case GameInputLabelArrowDownLLeft: return "Arrow Down Left";
		case GameInputLabelArrowLeft: return "Arrow Left";
		case GameInputLabelArrowUpLeft: return "Arrow Up Left";
		case GameInputLabelArrowUpDown: return "Arrow Up Down";
		case GameInputLabelArrowLeftRight: return "Arrow Left Right";
		case GameInputLabelArrowUpDownLeftRight: return "Arrow Up Down Left Right";
		case GameInputLabelArrowClockwise: return "Arrow Clockwise";
		case GameInputLabelArrowCounterClockwise: return "Arrow Counter Clockwise";
		case GameInputLabelArrowReturn: return "Arrow Return";

		case GameInputLabelIconBranding: return "Icon Branding";
		case GameInputLabelIconHome: return "Icon Home";
		case GameInputLabelIconMenu: return "Icon Menu";
		case GameInputLabelIconCross: return "Icon Cross";
		case GameInputLabelIconCircle: return "Icon Circle";
		case GameInputLabelIconSquare: return "Icon Square";
		case GameInputLabelIconTriangle: return "Icon Triangle";
		case GameInputLabelIconStar: return "Icon Star";
		case GameInputLabelIconDPadUp: return "Icon DPad Up";
		case GameInputLabelIconDPadDown: return "Icon DPad Down";
		case GameInputLabelIconDPadLeft: return "Icon DPad Left";
		case GameInputLabelIconDPadRight: return "Icon DPad Right";
		case GameInputLabelIconDialClockwise: return "Icon Dial Clockwise";
		case GameInputLabelIconDialCounterClockwise: return "Icon Dial Counter Clockwise";
		case GameInputLabelIconSliderLeftRight: return "Icon Slider Left Right";
		case GameInputLabelIconSliderUpDown: return "Icon Slider Up Down";
		case GameInputLabelIconWheelUpDown: return "Icon Wheel Up Down";
		case GameInputLabelIconPlus: return "Icon Plus";
		case GameInputLabelIconMinus: return "Icon Minus";
		case GameInputLabelIconSuspension: return "Icon Suspension";

		case GameInputLabelHome: return "Home";
		case GameInputLabelGuide: return "Guide";
		case GameInputLabelMode: return "Mode";
		case GameInputLabelSelect: return "Select";
		case GameInputLabelMenu: return "Menu";
		case GameInputLabelView: return "View";
		case GameInputLabelBack: return "Back";
		case GameInputLabelStart: return "Start";
		case GameInputLabelOptions: return "Options";
		case GameInputLabelShare: return "Share";
		case GameInputLabelUp: return "Up";
		case GameInputLabelDown: return "Down";
		case GameInputLabelLeft: return "Left";
		case GameInputLabelRight: return "Right";

		case GameInputLabelLB: return "LB";
		case GameInputLabelLT: return "LT";
		case GameInputLabelLSB: return "LSB";
		case GameInputLabelL1: return "L1";
		case GameInputLabelL2: return "L2";
		case GameInputLabelL3: return "L3";
		case GameInputLabelRB: return "RB";
		case GameInputLabelRT: return "RT";
		case GameInputLabelRSB: return "RSB";
		case GameInputLabelR1: return "R1";
		case GameInputLabelR2: return "R2";
		case GameInputLabelR3: return "R3";
		case GameInputLabelP1: return "P1";
		case GameInputLabelP2: return "P2";
		case GameInputLabelP3: return "P3";
		case GameInputLabelP4: return "P4";

		default: return "Other/unlabeled";
	}
}

void print_gamepad_info(const GameInputGamepadInfo& info) noexcept
{
	dbg::println("GameInputGamepadInfo:");
	dbg::println("  Menu               = {}", label_name(info.menuButtonLabel));
	dbg::println("  View               = {}", label_name(info.viewButtonLabel));
	dbg::println("  A                  = {}", label_name(info.aButtonLabel));
	dbg::println("  B                  = {}", label_name(info.bButtonLabel));
	dbg::println("  X                  = {}", label_name(info.xButtonLabel));
	dbg::println("  Y                  = {}", label_name(info.yButtonLabel));
	dbg::println("  DPad Up            = {}", label_name(info.dpadUpLabel));
	dbg::println("  DPad Down          = {}", label_name(info.dpadDownLabel));
	dbg::println("  DPad Left          = {}", label_name(info.dpadLeftLabel));
	dbg::println("  DPad Right         = {}", label_name(info.dpadRightLabel));
	dbg::println("  Left Shoulder      = {}", label_name(info.leftShoulderButtonLabel));
	dbg::println("  Right Shoulder     = {}", label_name(info.rightShoulderButtonLabel));
	dbg::println("  Left Stick Click   = {}", label_name(info.leftThumbstickButtonLabel));
	dbg::println("  Right Stick Click  = {}", label_name(info.rightThumbstickButtonLabel));
}

void destroy() noexcept { dbg::println("destroy() called"); }

/*
class Entity {
public:
	void update_physics(float fixed_dt) noexcept {
		// 1. Save state before modifying
		prev_position_ = curr_position_;

		// 2. Advance state for the new tick
		curr_position_.x += velocity_.x * fixed_dt;
		curr_position_.y += velocity_.y * fixed_dt;
	}

	// Called during the variable rendering phase
	[[nodiscard]] Vec2 get_render_position(float alpha) const noexcept {
		return prev_position_.lerp(curr_position_, alpha);
	}

	void set_velocity(Vec2 v) noexcept { velocity_ = v; }

private:
	Vec2 prev_position_{};
	Vec2 curr_position_{};
	Vec2 velocity_{50.0f, 0.0f}; // 50 units/sec
};
*/

struct grid_cell
{
	u32            x{};
	u32            y{};
	constexpr bool operator==(const grid_cell&) const noexcept = default;
};

struct grid_layout
{
	f32         origin_x{50.0f};
	f32         origin_y{50.0f};
	f32         margin{5.0f};
	u16         block_size{50};
	extent<u16> grid{8, 8};

	[[nodiscard]] constexpr f32 stride() const noexcept { return static_cast<f32>(block_size) + margin; }

	[[nodiscard]] constexpr vec2 cell_pos(grid_cell c) const noexcept
	{
		return {origin_x + static_cast<f32>(c.x) * stride(), origin_y + static_cast<f32>(c.y) * stride()};
	}

	[[nodiscard]] constexpr std::optional<grid_cell> cell_at(f32 px, f32 py) const noexcept
	{
		const f32 lx = px - origin_x;
		const f32 ly = py - origin_y;

		if (lx < 0.0f or ly < 0.0f)
			return std::nullopt;

		const f32 s  = stride();
		const u32 cx = static_cast<u32>(lx / s);
		const u32 cy = static_cast<u32>(ly / s);

		if (cx >= grid.width or cy >= grid.height)
			return std::nullopt;

		const f32 bs = static_cast<f32>(block_size);
		if (lx - static_cast<f32>(cx) * s >= bs or ly - static_cast<f32>(cy) * s >= bs)
			return std::nullopt;

		return grid_cell{cx, cy};
	}
};

enum class direction : u8
{
	north,
	east,
	south,
	west
};

constexpr f32 min_speed = 1.5f; // tiles per second
constexpr f32 max_speed = 5.0f;

struct entity
{
	enum class phase : u8
	{
		pausing,
		moving
	};

	vec2 position{0.0f, 0.0f};
	vec2 size{50.0f, 50.0f};

	grid_cell cell{};
	grid_cell target{};
	direction dir{direction::north};
	phase     state{phase::pausing};
	f32       timer{0.0f};
	f32       pause_time{0.5f};
	f32       speed{1.0f}; // tiles per second, set once at creation
	rgb       color{0, 255, 255};
};

constexpr f32 move_duration = 1.0f;
constexpr f32 min_turn_time = 0.5f;
constexpr f32 max_turn_time = 2.0f;

[[nodiscard]] constexpr std::optional<grid_cell> step_cell(grid_cell from, direction dir, const grid_layout& layout) noexcept
{
	i32 dx = 0;
	i32 dy = 0;

	switch (dir)
	{
		case direction::north: dy = -1; break;
		case direction::east: dx = 1; break;
		case direction::south: dy = 1; break;
		case direction::west: dx = -1; break;
	}

	const i32 nx = static_cast<i32>(from.x) + dx;
	const i32 ny = static_cast<i32>(from.y) + dy;

	if (nx < 0 || ny < 0 || nx >= layout.grid.width || ny >= layout.grid.height)
		return std::nullopt;

	return grid_cell{static_cast<u32>(nx), static_cast<u32>(ny)};
}

// Turn to a random direction (90 degree steps) and pick a random pause length
inline void begin_pause(entity& e, std::mt19937& rng) noexcept
{
	e.dir        = static_cast<direction>(std::uniform_int_distribution<int>{0, 3}(rng));
	e.pause_time = std::uniform_real_distribution<f32>{min_turn_time, max_turn_time}(rng);
	e.state      = entity::phase::pausing;
}

[[nodiscard]] inline entity make_entity(const grid_layout& layout, grid_cell cell, std::mt19937& rng) noexcept
{
	entity e{.position = layout.cell_pos(cell), .cell = cell, .target = cell};
	e.speed = std::uniform_real_distribution<f32>{min_speed, max_speed}(rng);
	e.color = to_rgb(std::uniform_real_distribution<f32>{0.0f, 360.0f}(rng), 1.0f, 1.0f);
	begin_pause(e, rng);
	return e;
}

// Pick a random in-bounds neighbour (4-directional)
[[nodiscard]] grid_cell random_neighbour(grid_cell from, const grid_layout& layout, std::mt19937& rng) noexcept
{
	struct offset
	{
		i32 dx;
		i32 dy;
	};

	constexpr std::array<offset, 4> dirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

	std::array<grid_cell, 4> valid{};
	usize                    count = 0;

	for (const auto [dx, dy] : dirs)
	{
		const i32 nx = static_cast<i32>(from.x) + dx;
		const i32 ny = static_cast<i32>(from.y) + dy;

		if (nx < 0 || ny < 0 || nx >= layout.grid.width || ny >= layout.grid.height)
			continue;

		valid[count++] = {static_cast<u32>(nx), static_cast<u32>(ny)};
	}

	if (count == 0)
		return from;

	return valid[std::uniform_int_distribution<usize>{0, count - 1}(rng)];
}

void update_entity(entity& e, f32 delta, const grid_layout& layout, std::mt19937& rng) noexcept
{
	e.timer += delta;

	if (e.state == entity::phase::pausing and e.timer >= e.pause_time)
	{
		e.timer -= e.pause_time;

		if (const auto next = step_cell(e.cell, e.dir, layout))
		{
			e.target = *next;
			e.state  = entity::phase::moving;
		}
		else
		{
			// facing the edge: turn again instead of moving
			begin_pause(e, rng);
		}
	}

	if (e.state == entity::phase::moving)
	{
		const f32  move_time = 1.0f / e.speed;
		const f32  t         = std::min(e.timer / move_time, 1.0f);
		const vec2 from      = layout.cell_pos(e.cell);
		const vec2 to        = layout.cell_pos(e.target);

		e.position = {std::lerp(from.x, to.x, t), std::lerp(from.y, to.y, t)};

		if (e.timer >= move_time)
		{
			e.timer -= move_time;
			e.cell     = e.target;
			e.position = to;
			begin_pause(e, rng);
		}
	}
}

// Returns the cell under the point, or nullopt if outside the grid or in a margin gap
[[nodiscard]] constexpr std::optional<grid_cell> cell_at(
  f32 px, f32 py, f32 origin_x, f32 origin_y, extent<u16> grid, f32 margin, u16 block_size) noexcept
{
	const f32 stride = static_cast<f32>(block_size) + margin;

	const f32 lx = px - origin_x;
	const f32 ly = py - origin_y;

	if (lx < 0.0f || ly < 0.0f)
		return std::nullopt;

	const u32 cx = static_cast<u32>(lx / stride);
	const u32 cy = static_cast<u32>(ly / stride);

	if (cx >= grid.width || cy >= grid.height)
		return std::nullopt;

	// reject the margin gap between blocks
	if (lx - static_cast<f32>(cx) * stride >= static_cast<f32>(block_size)
		|| ly - static_cast<f32>(cy) * stride >= static_cast<f32>(block_size))
		return std::nullopt;

	return grid_cell{cx, cy};
}

void render_grid(deckard::app2::renderer2* r, const grid_layout& l, f32 mx, f32 my) noexcept
{
	const f32  stride  = l.stride();
	const f32  bs      = static_cast<f32>(l.block_size);
	const auto hovered = l.cell_at(mx, my);

	const f32 total_w = static_cast<f32>(l.grid.width) * stride + l.margin;
	const f32 total_h = static_cast<f32>(l.grid.height) * stride + l.margin;

	r->draw_sprite(l.origin_x - l.margin, l.origin_y - l.margin, {total_w, total_h}, {0, 0, 0});

	for (u32 y = 0; y < l.grid.height; ++y)
	{
		for (u32 x = 0; x < l.grid.width; ++x)
		{
			const auto [px, py] = l.cell_pos({x, y});

			if (hovered == grid_cell{x, y})
			{
				constexpr f32 pad = 4.0f;
				r->draw_sprite(px - pad, py - pad, {bs + pad * 2.0f, bs + pad * 2.0f}, {255, 0, 255});
			}

			r->draw_sprite(px, py, {bs, bs}, {255, 255, 0});
		}
	}
}

void render_entity(deckard::app2::renderer2* render, const entity& e) noexcept
{
	render->draw_sprite(e.position.x, e.position.y, {e.size.x, e.size.y}, {e.color.r, e.color.g, e.color.b});

	constexpr f32 m  = 12.0f; // marker size
	const f32     cx = e.position.x + (e.size.x - m) * 0.5f;
	const f32     cy = e.position.y + (e.size.y - m) * 0.5f;

	f32 mx = cx;
	f32 my = cy;

	switch (e.dir)
	{
		case direction::north: my = e.position.y; break;
		case direction::south: my = e.position.y + e.size.y - m; break;
		case direction::west: mx = e.position.x; break;
		case direction::east: mx = e.position.x + e.size.x - m; break;
	}

	render->draw_sprite(mx, my, {m, m}, {255, 255, 255});
}

void render_entities(deckard::app2::renderer2* render, std::span<const entity> entities) noexcept
{
	for (const auto& e : entities)
		render_entity(render, e);
}

struct world
{
	grid_layout           layout{};
	std::mt19937          rng{std::random_device{}()};
	std::array<entity, 3> entities{};
};

void render_world(deckard::app2::renderer2* r, const world& w, f32 mx, f32 my) noexcept
{
	render_grid(r, w.layout, mx, my);
	render_entities(r, w.entities);
}

[[nodiscard]] world make_world()
{
	world w;
	w.entities = {
	  make_entity(w.layout, {0, 0}, w.rng),
	  make_entity(w.layout, {4, 4}, w.rng),
	  make_entity(w.layout, {7, 2}, w.rng),
	};
	return w;
}

world g_world = make_world();

// simulation only, no renderer, no input polling
void update(world& w, f32 delta) noexcept
{
	for (auto& e : w.entities)
		update_entity(e, delta, w.layout, w.rng);
}

grid_layout  layout{};
std::mt19937 rng{std::random_device{}()};

std::array entities{
  make_entity(layout, {0, 0}, rng),
  make_entity(layout, {4, 4}, rng),
  make_entity(layout, {7, 2}, rng),
};

void render(deckard::app2::renderer2* render, [[maybe_unused]] f32 delta) noexcept
{
	POINT p{};
	GetCursorPos(&p);
	ScreenToClient(render->get_handle(), &p);

	const auto size = render->get_size();
	const f32  mx   = static_cast<f32>(p.x);
	const f32  my   = static_cast<f32>(p.y);

	// background

	render->draw_text(mx + 20, my, std::format("Mouse: ({:.2f}, {:.2f})", mx, my), {255, 255, 255}, 40);

	// lines
	render->draw_line(0.0f, 0.0f, mx, my, {255, 0, 0}, 5.0f);
	render->draw_line(size.width, 0.0f, mx, my, {0, 255, 0}, 5.0f);
	render->draw_line(0.0f, size.height, mx, my, {0, 0, 255}, 5.0f);
	render->draw_line(size.width, size.height, mx, my, {255, 0, 255}, 5.0f);


	render->draw_sprite(mx + 50.0f, my + 50.0f, {100, 100}, {0, 255, 0});

	// for (auto& e : entities)
	//	update_entity(e, delta, layout, rng);


	// render_grid(render, delta, mx, my, {8, 8}, 5.0f, 50);
	// render_grid(, layout, mx, my);
	render_world(render, g_world, mx, my);
	// render_entities(render, entities);
}

bool initialize() noexcept
{
	dbg::println("initialize() called");
	return true;
}

i32 deckard_main([[maybe_unused]] utf8::view commandline)
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", window::build::version_string, window::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
	std::print("{}\n{}\n", big_text("DECKARD"), big_text(deckard_build::build::version_string));
#endif
	// ########################################################################


	// usage

	enum class TestCounter : u32
	{
		fps,
		ui,
		network,
		count
	};

	constexpr std::array intervals{
	  1.0f / 60.0f, // fps
	  1.0f / 30.0f, // ui
	  1.0f / 5.0f,  // network
	};

	fixed_timers<TestCounter, intervals> timer{};


	// #########################################################################
#if 1

	app::on_initialize(initialize);
	app::on_destroy(destroy);

	app::on_render(render);

	app::options options{
	  .size          = {1280, 720},
	  .fullscreen    = false,
	  .resizable     = true,
	  .vsync         = true,
	  .allow_win_key = true,
	  .monitor       = 0,
	  .title         = "Deckard - Vulkan - Window  1",
	};
	options.load_and_save();


	if (auto result = app::open(options); not result)
	{
		dbg::println("Failed to open app: {}", result.error());
		return -1;
	}

	app::on_tick(
	  [&](const f32 delta) noexcept
	  {
		  app::title(std::format(
			"Delta: {:<3.5f} - FPS: {:<8.2f} - Client: {} - Tick {:8.2f}",
			app::delta_time(),
			app::fps(),
			app::size(),
			1.0f / delta));

		  if (app::was_key_pressed(VK_F11))
			  app::fullscreen(not app::fullscreen());

		  if (app::was_key_pressed(VK_ESCAPE))
			  app::close();
	  });

	u32 uiticks  = 0;
	u32 netticks = 0;
	while (app::running())
	{
		const f32 delta = app::delta_time();

		timer.update(delta);
		update(g_world, delta);

		while (timer.tick(TestCounter::ui))
		{

			uiticks++;
		}


		while (timer.tick(TestCounter::network))
		{

			netticks++;
		}


		// app::close();

		if (app::was_key_pressed(VK_F11))
		{
			app::fullscreen(not app::fullscreen());
		}


		if (app::was_key_pressed(VK_ESCAPE))
		{
			app::close();
		}
	}

	dbg::println("ratio: {}", uiticks / static_cast<f32>(netticks));


	_ = 0;

#endif
	// ########################################################################
	// ########################################################################


	GameInputCallbackToken callback_token{};
	IGameInput*            gameinput = nullptr;
	if (not SUCCEEDED(GameInputCreate(&gameinput)))
	{
		dbg::println("Failed to initialize GameInput.");
		return -1;
	}


	auto callback =
	  [](GameInputCallbackToken,
		 void*,
		 IGameInputDevice*     device,
		 u64                   timestamp,
		 GameInputDeviceStatus status,
		 GameInputDeviceStatus prev_status)
	{
		const GameInputDeviceInfo* info      = device->GetDeviceInfo();
		const auto                 supported = info->supportedInput;
		const auto                 connected = static_cast<bool>(status & GameInputDeviceConnected);
		bool                       wireless  = static_cast<bool>(status & GameInputDeviceWireless);

		dbg::println(
		  "[{}] Device status changed: connected={}, wireless={}, supported={:#x}, previous={:#x}",
		  timestamp,
		  connected,
		  wireless,
		  static_cast<unsigned>(supported),
		  static_cast<unsigned>(prev_status));

		dbg::println(
		  "family: {}, capabilities: {:#x}", std::to_underlying(info->deviceFamily), std::to_underlying(info->capabilities));

		GameInputBatteryState batterystate{};
		device->GetBatteryState(&batterystate);
		dbg::println(
		  "Charge rate: {}, maxChargeRate: {}, remaining capacity: {}, full charge capacity: {}",
		  batterystate.chargeRate,
		  batterystate.maxChargeRate,
		  batterystate.remainingCapacity,
		  batterystate.fullChargeCapacity);
		switch (batterystate.status)
		{
			case GameInputBatteryStatus::GameInputBatteryUnknown: dbg::println("Battery status: Unknown"); break;
			case GameInputBatteryStatus::GameInputBatteryNotPresent: dbg::println("Battery status: Not Present"); break;
			case GameInputBatteryStatus::GameInputBatteryDischarging: dbg::println("Battery status: Discharging"); break;
			case GameInputBatteryStatus::GameInputBatteryIdle: dbg::println("Battery status: Idle"); break;
			case GameInputBatteryStatus::GameInputBatteryCharging: dbg::println("Battery status: Charging"); break;
		}

		// supportedInput is a bitmask, not a single value — a real gamepad sets
		// both GameInputKindGamepad and GameInputKindController at once, so
		// check the most-specific/standardized kind first.
		if (not connected)
		{
			if (supported & GameInputKindGamepad)
				dbg::println("[Gamepad] disconnected: supported={:#x}", static_cast<unsigned>(supported));
			else if (supported & GameInputKindKeyboard)
				dbg::println("[Keyboard] disconnected: supported={:#x}", static_cast<unsigned>(supported));
			else if (supported & GameInputKindMouse)
				dbg::println("[Mouse] disconnected: supported={:#x}", static_cast<unsigned>(supported));
			else if (supported & GameInputKindArcadeStick)
				dbg::println("[Arcade stick] disconnected: supported={:#x}", static_cast<unsigned>(supported));
			else if (supported & GameInputKindFlightStick)
				dbg::println("[Flight stick] disconnected: supported={:#x}", static_cast<unsigned>(supported));
			else if (supported & GameInputKindRacingWheel)
				dbg::println("[Racing wheel] disconnected: supported={:#x}", static_cast<unsigned>(supported));
			else if (supported & GameInputKindController)
				dbg::println("[Generic controller] disconnected: supported={:#x}", static_cast<unsigned>(supported));
			else
				dbg::println("[Device] disconnected: supported={:#x}", static_cast<unsigned>(supported));

			return;
		}


		if (supported & GameInputKindGamepad)
		{
			dbg::println("[Gamepad] {}", connected ? "connected" : "disconnected");


			dbg::println(
			  "Device string count{}: connected={}, wireless={}, supported={:#x}",
			  info->deviceStringCount,
			  connected,
			  wireless,
			  static_cast<unsigned>(supported));

			if (info->gamepadInfo)
				print_gamepad_info(*info->gamepadInfo);

			dbg::println("Family: {}", label_device_family(info->deviceFamily));

			dbg::println("Capabilities: {}", label_device_capability(info->capabilities));

			const GameInputRumbleMotors motors             = info->supportedRumbleMotors;
			u32                         rumble_motor_count = std::popcount(static_cast<u32>(motors));
			dbg::println("  Supported rumble motors: {:#x}", static_cast<unsigned>(motors));
			dbg::println(
			  "Rumble motors: {} (low={} high={} leftTrig={} rightTrig={})",
			  rumble_motor_count,
			  (motors & GameInputRumbleLowFrequency) != 0,
			  (motors & GameInputRumbleHighFrequency) != 0,
			  (motors & GameInputRumbleLeftTrigger) != 0,
			  (motors & GameInputRumbleRightTrigger) != 0);

			dbg::println();

			f32 lowFrequency  = 0.0f;
			f32 highFrequency = 0.0f;
			f32 trigger_left  = 0.0f;
			f32 trigger_right = 0.0f;

			GameInputRumbleParams params{
			  .lowFrequency  = (motors & GameInputRumbleLowFrequency) ? lowFrequency : 0.0f,
			  .highFrequency = (motors & GameInputRumbleHighFrequency) ? highFrequency : 0.0f,
			  .leftTrigger   = (motors & GameInputRumbleLeftTrigger) ? trigger_left : 0.0f,
			  .rightTrigger  = (motors & GameInputRumbleRightTrigger) ? trigger_right : 0.0f,
			};

			device->SetRumbleState(&params);
		}
		else if (supported & GameInputKindKeyboard)
			dbg::println("[Keyboard] {}", connected ? "connected" : "disconnected");
		else if (supported & GameInputKindMouse)
			dbg::println("[Mouse] {}", connected ? "connected" : "disconnected");
		else if (supported & GameInputKindArcadeStick)
			dbg::println("[Arcade stick] {}", connected ? "connected" : "disconnected");
		else if (supported & GameInputKindFlightStick)
			dbg::println("[Flight stick] {}", connected ? "connected" : "disconnected");
		else if (supported & GameInputKindRacingWheel)
			dbg::println("[Racing wheel] {}", connected ? "connected" : "disconnected");
		else if (supported & GameInputKindController)
			dbg::println("[Generic controller] {}", connected ? "connected" : "disconnected");
		else
			dbg::println(
			  "[Unknown] supported={:#x} {}", static_cast<unsigned>(supported), connected ? "connected" : "disconnected");

		u32 vendor_product = info->vendorId << 16 | info->productId;
		dbg::println("  Vendor/Product: {:#x}", vendor_product);
		dbg::println("  Vendor ID: {:#x}", info->vendorId);
		dbg::println("  Product ID: {:#x}", info->productId);
		dbg::println("  Device ID: {}", info->deviceId.value);
	};


	gameinput->RegisterDeviceCallback(
	  nullptr, // no device filter
	  GameInputKindGamepad | GameInputKindKeyboard | GameInputKindMouse | GameInputKindArcadeStick | GameInputKindFlightStick
		| GameInputKindRacingWheel,
	  GameInputDeviceConnected,
	  GameInputAsyncEnumeration,
	  nullptr, // context
	  callback,
	  &callback_token);


	IGameInputReading* reading{};

	while (!true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
		if (SUCCEEDED(gameinput->GetCurrentReading(GameInputKindGamepad, nullptr, &reading)))
		{
			GameInputGamepadState state{};
			if (reading->GetGamepadState(&state))
			{
				IGameInputDevice* device{};
				reading->GetDevice(&device);


				const bool connected = (device->GetDeviceStatus() & GameInputDeviceConnected) != 0;

				if (connected)
				{
					dbg::println(
					  "[{:X}] buttons={:014b} | LS=({:+1.5f},{:+1.5f}) RS=({:+1.5f},{:+1.5f}) | LT={:+1.5f} RT={:+1.5f}",
					  reading->GetTimestamp(),
					  as<u32>(state.buttons),
					  state.leftThumbstickX,
					  state.leftThumbstickY,
					  state.rightThumbstickX,
					  state.rightThumbstickY,
					  state.leftTrigger,
					  state.rightTrigger);
				}
				device->Release();
			}
			reading->Release();
		}
	}
	gameinput->UnregisterCallback(callback_token, /*timeoutMs=*/0);


#if 0


	vulkanapp app01{{.title = "hello"}};
	app01.set_keyboard_callback(keyboard_callback);
	app01.set_fixed_update_callback(fixed_update);
	app01.set_update_callback(update);
	app01.set_render_callback(render);
	

	return app01.run();

#endif

	test_class tc;

	tc.flag(1).flag(2).flag(3).option(42).flag(4);


	// static_assert(offsetof(TestStruct, a) == 0);
	// static_assert(offsetof(TestStruct, b) == offsetof(TestStruct, a) + sizeof(u8) + Padding<u8>{1});
	// static_assert(offsetof(TestStruct, c) == offsetof(TestStruct, b) + sizeof(i16));
	// static_assert(offsetof(TestStruct, d) == offsetof(TestStruct, c) + sizeof(u32));

	u64 secret = random::randu64();
	for (int i = 12; i < 12; ++i)
	{
		auto start = std::chrono::high_resolution_clock::now();
		for (u64 nonce = 0;; ++nonce)
		{
			u64 combined = secret + nonce;
			u64 hash     = utils::chibihash64(&combined, sizeof(combined));
			if (std::countl_zero(hash) == i)
			{
				auto end = std::chrono::high_resolution_clock::now();
				dbg::println(
				  "nonce: {} - hash: {:0X} ({} leading zeros) - elapsed: {:.5f} ms",
				  nonce,
				  hash,
				  std::countl_zero(hash),
				  std::chrono::duration<double, std::milli>(end - start).count());
				dbg::println("{:064B}", hash);
				break;
			}
		}
	}


	TestStruct ts{};

	auto tstr = as_byte_span(ts);

	TestStruct ts2 = from_byte_array<TestStruct>(tstr);
	_              = 0;


	// ###############################

	constexpr u8  archive_version = 255;
	constexpr u64 archive_magic   = std::byteswap(0x4445'434B'4152'4400 | archive_version); // "DECKARD" + version byte

	(void)file::write({.filename = "archive_test.dat",
					   .buffer   = std::span<const u8>(reinterpret_cast<const u8*>(&archive_magic), sizeof(archive_magic))});


	std::vector<fs::path> files_to_archive;

	fs::path data_dir = "data";
	for (const fs::directory_entry& path : fs::recursive_directory_iterator(data_dir))
	{
		if (path.is_regular_file() or path.is_directory())
			files_to_archive.push_back(path);
	}

	std::ranges::sort(files_to_archive, std::ranges::greater());
	// std::ranges::sort(files_to_archive);

	// greater: 585
	//        : 596
	// files_to_archive.clear();


	std::vector<u8> paths_buffer;
	paths_buffer.reserve(1024 * 128);

	auto gen_random_file = [&](const fs::path path)
	{
		u64             size = random::randu64(10_KiB, 1_MiB);
		std::vector<u8> buffer(size);

		generate(buffer, random::randu64(40, 100));
		(void)file::write({.filename = path, .buffer = buffer});
	};

	u64 sizes_total = 0;

	u64 compressed_total_bytes = 0;

	for (const fs::path& file : files_to_archive | std::views::take(1))
	{

		if (fs::is_directory(file))
			continue;

		u64 file_size = fs::file_size(file);

		sizes_total += file_size;

		if (file_size == 0)
		{
			gen_random_file(file);
		}

		std::vector<u8> file_data;
		file_data.resize(file_size);
		(void)file::read({.filename = file, .buffer = file_data, .size = file_size});

		std::vector<u8> compressed_data;
		compressed_data.resize(zstd::bound(file_data.size()));
		auto csize = zstd::compress(file_data, compressed_data);

		compressed_total_bytes += csize.value_or(0);

		std::string p = file.string();


		paths_buffer.insert(paths_buffer.end(), p.begin(), p.end());
		paths_buffer.push_back('\0');

		dbg::println("file: {} ({} bytes)", p, fs::file_size(file));
	}

	std::vector<u8> compressed_paths_buffer;
	compressed_paths_buffer.resize(zstd::bound(paths_buffer.size()));
	auto size = zstd::compress(paths_buffer, compressed_paths_buffer);
	compressed_paths_buffer.resize(size.value_or(0));
	dbg::println("paths_buffer size: {}, compressed size: {}",
				 human_readable_bytes(paths_buffer.size()),
				 human_readable_bytes(size.value_or(0)));


	dbg::println("Total size of files: {}", human_readable_bytes(sizes_total));
	dbg::println("Total compressed size of files: {}", human_readable_bytes(compressed_total_bytes));
	dbg::println(
	  "Compressed ratio: {:.2f}%", 100.0 - (math::safe_divide(compressed_total_bytes, sizes_total).value_or(0) * 100.0));
	_ = 0;


	// ###############################

	std::vector<u8> rnd_data;
	rnd_data.resize(16_MiB);
	{
		ScopeTimer time("random data");

		random::bytes_quick(rnd_data);
	}

	_ = 0;


	{
		ScopeTimer time("chibihash");

		u64 hash = utils::chibihash64(rnd_data);
		dbg::println("chibihash64: {:0X}", hash);
	}

	{
		ScopeTimer time("rapidhash");

		u64 hash = utils::rapidhash(rnd_data);
		dbg::println("rapidhash: {:0X}", hash);
	}

	{
		ScopeTimer time("xxhash64");

		u64 hash = utils::xxhash64(rnd_data);
		dbg::println("xxhash64: {:0X}", hash);
	}

	{
		ScopeTimer time("xxh64");

		auto hash = utils::xxh64(rnd_data);
		dbg::println("xxh64: {:0X}", hash);
	}

	(void)file::write({.filename = "archive_test.dat", .buffer = rnd_data, .offset = sizeof(archive_magic)});

	auto fview = file::map("archive_test.dat");

	auto fview_sub = fview.subspan(0, 64);


	_ = 0;
	// ########################################################################

	std::array<u8, 10> encoded{};
	// u64                value = 0xFFFF'FFFF'FFFF'FFFFu;
	u32 value = 0x0000'1234;

	auto [ilen, ibuf] = encode_integer_to_array(value);

	_ = 0;

	// u64 decoded = decode_integer(encoded);


	_;
	// ##############################

#if 0
	Tree<char> tree[]{
	  {'D', tree + 1, tree + 2}, {'B', tree + 3, tree + 4}, {'F', tree + 5, tree + 6}, {'A'}, {'C'}, {'E'}, {'G'}};

	for (char x : tree->traverse_inorder())
		dbg::print("{} ", x);
	dbg::println();

	_ = 0;
#endif


	// ########################################################################

	config cfg(fs::path{"config.txt"});

	//	dbg::println("Version: {}", cfg["version"].as<std::string>());
	dbg::println("width: {}", cfg["window.width"].as<u32>());
	dbg::println("height: {}", cfg["window.height"].as<u32>());
	dbg::println("fullscreen: {}", cfg["window.fullscreen"].as<bool>());


	cfg["window.fullscreen"] = not cfg["window.fullscreen"].as<bool>();

	cfg["version"] = random::id(6);
	cfg.set_comment("version", std::format("This is the random id: '{}'", random::id(3)));


	cfg.set_comment("window.width", "new comment for window.width");

	dbg::println("id from config: {}", cfg["version"].as<std::string>());

	// cfg[std::format("new_{}", random::id(3))] = "hello world";

	(void)cfg.save();


	// file::write(
	//   {.filename = "config.txt",
	//    .buffer   = cfg.data()}); // complete rewrite, should be identical to original config.txt except for the
	//    modified fullscreen value
	//


	_ = 0;
	// ########################################################################


	std::array<u8, 128> buf128{};
	for (const auto& [i, c] : buf128 | std::views::enumerate)
		c = (char)i;

	(void)file::write({.filename = "bin128.dat", .buffer = buf128});

	std::array<u8, 64> buf64{};
	(void)file::read({.filename = "bin128.dat", .buffer = buf64, .size = buf64.size(), .offset = 64});

	auto vi = file::map("bin128.dat", 16);

	for (const auto& [i, c] : vi.data() | std::views::enumerate)
	{
		dbg::print("{:02X} ", c);
		if ((i + 1) % 16 == 0)
			dbg::println();
	}

	auto dvi = vi.data();

	vi.close();

	config cfg2(fs::path{"config2.txt"});


	dbg::println("window.fullscreen: {}", cfg2["window.fullscreen"].as<bool>());
	dbg::println("window.hello: {}", cfg2["window.fullscreen"].as<utf8::string>());


	(void)cfg2.save();

	// ########################################################################

	[[maybe_unused]] u32 fbtui = float_bits_to_uint(1.0f);
	_                          = 0;


	// ########################################################################

	constexpr u64 buffer_size = 1_KiB;
	f32           offset      = -1.0f;
	for (int i = 100; i >= 0; i -= 10)
	{
		std::vector<u8> rnd{};
		rnd.resize(buffer_size);

		generate(rnd, i);

		std::vector<u8> out_rnd{};
		out_rnd.resize(buffer_size);

		auto result = zstd::compress_if_smaller(rnd, out_rnd);

		offset += result ? 0.0f : 1.0f;

		f32 percent = 100.0f + offset - (static_cast<f32>(result.value_or(0)) / static_cast<f32>(rnd.size())) * 100.0f;

		percent = result ? percent : 0.0f;

		dbg::println(
		  "compress_if_smaller: {}% compressible, result: {}, compressed size: {} bytes ({:.2f})",
		  i,
		  result.has_value() ? "compressed" : "not compressed",
		  result.value_or(0),
		  percent);
	}


	_ = 0;

	// ########################################################################

	read_png_info("xor_texture.png");

	// ########################################################################

#if 0


	image_rgb  xortexture(1920, 1080);
	ScopeTimer timer("timer");

	timer.start();
	for (int y = 0; y < xortexture.height(); ++y)
	{
		for (int x = 0; x < xortexture.width(); ++x)
		{
			// Create a colorful XOR-based pattern with bit shifts and rotations
			const u8 r = static_cast<u8>((x ^ y) & 0xFF);
			const u8 g = static_cast<u8>(((x >> 2) ^ (y << 1)) & 0xFF);
			const u8 b = static_cast<u8>(((x << 1) ^ (y >> 2)) & 0xFF);

			xortexture[x, y] = rgb(r, g, b);
		}
	}
	timer.stop("xor texture generation");

	info("hash: {:#16X}", utils::xxhash64(xortexture.raw_data()));


	_ = 0;

// time save/load bmp
#if 0
	timer.start();
	save_bmp("xor_texture.bmp", xortexture);
	timer.stop("bmp save");

	timer.start();
	auto loaded_xor = load_bmp("xor_texture.bmp");
	timer.stop("bmp load");

	if (loaded_xor == xortexture)
	{
		info("xor texture bmp save/load successful");
	}
	else
	{
		info("xor texture bmp save/load failed");
	}

	save_bmp("xor_texture_copy.bmp", *loaded_xor);


	timer.start();
	save_tga("xor_texture.tga", xortexture);
	timer.stop("tga save");

	timer.start();
	auto loaded_tga = load_tga("xor_texture.tga");
	timer.stop("tga load");

	if (loaded_tga == xortexture)
	{
		info("xor texture tga save/load successful");
	}
	else
	{
		info("xor texture tga save/load failed");
	}

	save_tga("xor_texture_copy.tga", *loaded_tga);


	// qoi

	timer.start();
	save_qoi("xor_texture.qoi", xortexture);
	timer.stop("qoi save");

	timer.start();
	auto loaded_qoi = load_qoi("xor_texture.qoi");
	timer.stop("qoi load");

	if (loaded_qoi == xortexture)
	{
		info("xor texture qoi save/load successful");
	}
	else
	{
		info("xor texture qoi save/load failed");
	}

	save_qoi("xor_texture_copy.qoi", *loaded_qoi);

	
	timer.start();
	save_dif("xor_texture.dif0", xortexture);
	timer.stop("dif save");
	timer.start();
	auto loaded_dif = load_dif("xor_texture.dif0");
	timer.stop("dif load");
	if (loaded_dif == xortexture)
	{
		info("xor texture dif save/load successful");
	}
	else
	{
		info("xor texture dif save/load failed");
	}

	save_bmp("xor_texture_copy_from_dif.bmp", *loaded_dif);
#endif
	//

	timer.start();

	auto viking_bmp = load_bmp("data/viking_room.bmp");
	timer.stop("load viking bmp");

	timer.start();
	save_dif("viking.dif1", viking_bmp);
	timer.stop("save viking dif");

	timer.start();
	auto loaded_viking_dif = load_dif("viking.dif1");
	timer.stop("load viking dif");

	if (loaded_viking_dif == viking_bmp)
	{
		info("viking dif save/load successful");
	}
	else
	{
		info("viking dif save/load failed");
	}

	timer.start();
	save_tga("convert_viking_dif_tga.tga", loaded_viking_dif);
	timer.stop("save viking dif to tga");

	timer.start();
	save_qoi("convert_viking_dif_tga.qoi", loaded_viking_dif);
	timer.stop("save viking dif to qoi");

	timer.start();
	(void)zstd::compress_file_to("convert_viking_dif_tga.qoi", "vikingqoi_recompressed.dat");
	timer.stop("recompress qoi");

	// read back


#endif

	info("heloo");
	// ########################################################################


	info("{}", file::get_temp_path().string());
	info("{}", file::get_temp_file("spv").string());


	// ########################################################################

	_ = 0;

	// ########################################################################

#if 1
	if (auto ip_result = net::resolve_ips("api.taboobuilder.com"); ip_result)
	{
		for (const auto& ip : *ip_result)
		{
			dbg::println("{}: {}", ip.is_ipv4() ? "ipv4" : "ipv6", ip);
		}
	}
	else
	{
		dbg::println("Failed to get IP addresses: {}", ip_result.error());
	}
#endif
	_ = 0;

	// ########################################################################


	auto resolve_stun = net::resolve_ips("stun.l.google.com");
	for (const auto& ip : resolve_stun.value_or({}))
		dbg::println("Resolved STUN IP: {}", ip);

	_ = 0;
	{
		config stunservers("stuns.txt"_path);

		(void)stunservers.save("stuns_out.txt"_path);

		auto servers = stunservers["servers.host"].as_vector<net::endpoint>();


		_ = 0;
		if (not servers.empty())
		{
			u8 hostname_index = random::randu8(0, as<u8>(servers.size() - 1));

			if (stunservers["servers.index"].as<i8>() >= 0)
				hostname_index = std::clamp(stunservers["servers.index"].as<u8>(), 0_u8, as<u8>(servers.size() - 1));
			// server_index = 0;

			std::string hostname = servers[hostname_index].hostname ? *servers[hostname_index].hostname : "";
			std::string service  = std::to_string(servers[hostname_index].port);
			// ----------------------- Resolve host ---------------------------------
			const auto& domain = hostname;

			dbg::println("Resolving '{}'...", domain);
			auto resolved = net::resolve_ips(domain);
			if (not resolved or resolved->empty())
			{
				dbg::println("Failed to resolve '{}'", domain);
				return 1;
			}

			for (const auto& ip : *resolved)
				dbg::println("  {} (IPv{})", ip, ip.version());

			// auto [storage, addrlen] = resolved->front().to_sockaddr();

			// select random resolved ip
			u32 ip_index = random::randu32(0, as<u32>(resolved->size() - 1));

			auto [storage, addrlen] = resolved->at(ip_index).to_sockaddr();


			// Set the port on the storage
			if (resolved->at(ip_index).is_ipv6())
				reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(servers[hostname_index].port);
			else
				reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(servers[hostname_index].port);

			// ----------------------- Create socket ---------------------------------
			SOCKET sock = socket(storage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
			if (sock == INVALID_SOCKET)
			{
				dbg::println("socket() failed: {}", WSAGetLastError());
			}

			std::array<u8, 20> stunpacket{
			  // STUN Message Type: 0x0001 (Binding Request)
			  0x00,
			  0x01,

			  // Message Length: 0x0000 (No attributes for a basic request)
			  0x00,
			  0x00,

			  // Magic Cookie: 0x2112A442
			  // This value helps differentiate STUN from legacy protocols.
			  0x21,
			  0x12,
			  0xA4,
			  0x42,

			  // Transaction ID: 12 random bytes
			  0x00,
			  0x00,
			  0x00,
			  0x00,
			  0x00,
			  0x00,
			  0x00,
			  0x00,
			  0x00,
			  0x00,
			  0x00,
			  0x00};

			std::array<u8, 12> transaction_id_bytes{};
			random::bytes(transaction_id_bytes);

			write_be<u8>(stunpacket, transaction_id_bytes, 8);

			DWORD timeoutMs = 2500;
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));


			int sent = sendto(
			  sock,
			  reinterpret_cast<const char*>(stunpacket.data()),
			  static_cast<int>(stunpacket.size()),
			  0,
			  reinterpret_cast<sockaddr*>(&storage),
			  static_cast<int>(addrlen));

			if (sent == SOCKET_ERROR)
			{
				dbg::println("sendto() failed: {}", WSAGetLastError());
				closesocket(sock);
				WSACleanup();
				return 1;
			}
			dbg::println("STUN send = {}", sent);

			// ----------------------- Set receive timeout (5 seconds) ---------------
			// ----------------------- Receive reply ---------------------------------

			std::vector<u8> incoming{};
			incoming.resize(1024);

			int len = recvfrom(
			  sock, reinterpret_cast<char*>(incoming.data()), static_cast<int>(incoming.size()), 0, nullptr, nullptr);

			if (len == SOCKET_ERROR)
			{
				dbg::println("recvfrom() failed: {}", WSAGetLastError());
				incoming.clear();
			}
			else
			{

				dbg::println("STUN received {} bytes:", len);
				incoming.resize(len);

				for (const auto& c : incoming)
					dbg::print("{:02X} ", c);
				dbg::println("\n\n");
			}

			STUNHeader stun_header{};
			if (incoming.size() < 20)
			{
				dbg::println("STUN response too small: {}", incoming.size());
			}

			if (incoming.size() >= 20)
			{
				deckard::serializer packetx(incoming);

				stun_header.type   = packetx.read<u16>();
				stun_header.length = packetx.read<u16>();
				stun_header.cookie = packetx.read<u32>();
				packetx.read<u8, 12>(stun_header.transaction_id);

				auto class_bits  = [](u16 type) -> u8 { return ((type >> 4) & 0x02) | ((type >> 7) & 0x01); };
				auto method_bits = [](u16 type) -> u16
				{ return ((type >> 2) & 0xF80) | ((type >> 1) & 0x70) | (type & 0x0F); };


				switch (class_bits(stun_header.type))
				{
					case 0b00: dbg::println("  Attribute Class: Request (0b00)"); break;
					case 0b01: dbg::println("  Attribute Class: Indication (0b01)"); break;
					case 0b10: dbg::println("  Attribute Class: Success Response (0b10)"); break;
					case 0b11: dbg::println("  Attribute Class: Error Response (0b11)"); break;
					default: dbg::println("  Attribute Class: Unknown"); break;
				}

				switch (method_bits(stun_header.type))
				{
					case 0x0001: dbg::println("  Attribute Method: Binding (0x0001)"); break;
					default: dbg::println("  Attribute Method: Unknown"); break;
				}


				if (not stun_header.is_zero())
				{
					dbg::println("Invalid STUN message type (most significant 2 bits must be 0)");
				}
				// Validate message length
				if (stun_header.length + 20 != incoming.size())
				{
					dbg::println("STUN message length mismatch: header length = {}, actual length = {}",
								 stun_header.length,
								 incoming.size() - 20);
				}

				// Validate magic cookie
				if (stun_header.cookie != 0x2112'A442)
				{
					dbg::println("Invalid STUN magic cookie: 0x{:08X}", stun_header.cookie);
				}

				// Validate transaction ID matches what we sent
				if (not std::ranges::equal(stun_header.transaction_id, std::span{stunpacket}.subspan<8, 12>()))
				{
					dbg::println("STUN transaction ID mismatch");
				}


				// Binding request: class 00, method 000000000001
				// Binding response: class 10, method 000000000001

				dbg::println("STUN Message Type: 0b{:014b}", as<u16>(stun_header.type));
				dbg::println("  Class:    0b{:02b}", stun_header.class_bits());
				dbg::println("  Method:   0b{:012b}", stun_header.method_bits());
				dbg::println("Message Length: {}", stun_header.length);
				dbg::println("Magic Cookie: 0x{:04X}", stun_header.cookie);
				dbg::println(
				  "Transaction ID: {}",
				  to_hex_string(std::span<u8>{stun_header.transaction_id}, {.delimiter = " ", .show_hex = false}));

				std::span<u8> rest = std::span<u8>{incoming}.subspan(20, incoming.size() - 20);
				dbg::println("Rest: {}", to_hex_string(rest, {.delimiter = " ", .show_hex = false}));

				packetx.reset(rest);

				u32 timeout = 0;
				while (packetx.remaining() >= 4 or timeout > 10)
				{
					timeout++;

					u16 attr_type = packetx.read<u16>();


					u8  class_encoding = class_bits(attr_type);
					u16 method         = method_bits(attr_type);

					dbg::println("  Attribute Class: 0b{:02b}", class_encoding);
					switch (class_encoding)
					{
						case 0b00: dbg::println("  Attribute Class: Request (0b00)"); break;
						case 0b01: dbg::println("  Attribute Class: Indication (0b01)"); break;
						case 0b10: dbg::println("  Attribute Class: Success Response (0b10)"); break;
						case 0b11: dbg::println("  Attribute Class: Error Response (0b11)"); break;
						default: dbg::println("  Attribute Class: Unknown"); break;
					}
					dbg::println("  Attribute Method: 0b{:012b}", method);
					switch (method)
					{
						case 0x0001: dbg::println("  Attribute Method: Binding (0x0001)"); break;
						default: dbg::println("  Attribute Method: Unknown"); break;
					}

					u16 attr_length = packetx.read<u16>();
					if (packetx.remaining() < attr_length)
					{
						dbg::println("Attribute length exceeds remaining packet size");
						break;
					}

					switch (attr_type)
					{

						case 0x0001: dbg::println("Attribute Type: MAPPED-ADDRESS (0x0001)"); break;
						case 0x0006: dbg::println("Attribute Type: USERNAME (0x0006)"); break;
						case 0x0008: dbg::println("Attribute Type: MESSAGE-INTEGRITY (0x0008)"); break;
						case 0x0009: dbg::println("Attribute Type: ERROR-CODE (0x0009)"); break;
						case 0x000A: dbg::println("Attribute Type: UNKNOWN-ATTRIBUTES (0x000A)"); break;
						case 0x0014: dbg::println("Attribute Type: REALM (0x0014)"); break;
						case 0x0015: dbg::println("Attribute Type: NONCE (0x0015)"); break;

						case 0x0020: dbg::println("Attribute Type: XOR-MAPPED-ADDRESS (0x0020)"); break;

						// Optional
						case 0x8022: dbg::println("Attribute Type: SOFTWARE (0x8022) "); break;
						case 0x8023: dbg::println("Attribute Type: ALTERNATE-SERVER (0x8023)"); break;
						case 0x8028: dbg::println("Attribute Type: FINGERPRINT (0x8028)"); break;
						default: dbg::println("Unhandled type: {:02X}", attr_type); break;
					}

					packetx.read<u8>(); // PADDING
					dbg::println("Attribute Type: 0x{:04X}, Length: {}", attr_type, attr_length);

					u8  ip_version = packetx.read<u8>();
					u16 port       = packetx.read<u16>();

					dbg::println("  IP Version: {}, Port: {}", ip_version, port);

					u32 ip = 0;
					if (ip_version == 1)
						ip = packetx.read<u32>();
					std::array<u8, 16> ip6{};
					if (ip_version == 2)
						packetx.read<u8, 16>(ip6);

					if (attr_type == 0x0001 and ip_version == 1)
					{
						// MAPPED-ADDRESS
						// The port is in network byte order (big-endian).
						// The IP address is also in network byte order.
						// To get the actual port number, you may need to convert it from network byte order to host byte
						// order. On most systems, you can use the ntohs function for this purpose.
						u16 real_port = port;
						dbg::println("Real Port: {}", real_port);
						dbg::println("{}.{}.{}.{}", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
					}

					if (attr_type == 0x0001 and ip_version == 2)
					{
						u16 real_port = port;
						dbg::println("Real Port: {}", real_port);
						dbg::println("IPv6: {}", to_hex_string(std::span<u8>{ip6}, {.delimiter = ":", .show_hex = false}));
					}


					if (attr_type == 0x0020 and ip_version == 1)
					{
						u16 xport = port ^ (stun_header.cookie >> 16);
						// XOR-MAPPED-ADDRESS
						u32 xip = ip ^ stun_header.cookie;
						dbg::println(
						  "X-Port: {}, X-IP: {}.{}.{}.{}",
						  xport,
						  (xip >> 24) & 0xFF,
						  (xip >> 16) & 0xFF,
						  (xip >> 8) & 0xFF,
						  xip & 0xFF);
					}

					if (attr_type == 0x0020 and ip_version == 2)
					{
						u16 xport = port ^ (stun_header.cookie >> 16);

						std::array<u8, 16> xip6{};
						for (size_t i = 0; i < 16; ++i)
						{
							if (i < 4)
								xip6[i] = ip6[i] ^ ((stun_header.cookie >> (24 - i * 8)) & 0xFF);
							else
								xip6[i] = ip6[i] ^ stun_header.transaction_id[i - 4];
						}
						dbg::println("X-Port: {}, X-IP6: {}",
									 xport,
									 to_hex_string(std::span<u8>{xip6}, {.delimiter = ":", .show_hex = false}));
					}

					dbg::println("Remaining: {}", packetx.remaining());
					//// Align to 4-byte boundary
					// size_t padding = (4 - (attr_length % 4)) % 4;
					// if (padding > 0 && packetx.remaining() >= padding)
					//{
					//	packetx.read<u8>(padding);
					// }
				}
			}

			// Another packet
			{

				std::string username("");
				std::string realm("realm");
				std::string nonce;
				std::string pass("pass");


				std::vector<u8> packet{};


				// STUN Header (20 bytes)
				// Message Type

				//                   Y    X    1
				u16 type = 0b0000'0000'0000'0001;     // Binding Request


				packet.push_back((type >> 8) & 0xFF); // MSB of message typ)e
				packet.push_back(type & 0xFF);

				packet.push_back(0x00);               // Message Length (to be filled later)
				packet.push_back(0x00);               // LSB of message length

				// Magic Cookie
				packet.push_back(0x21);
				packet.push_back(0x12);
				packet.push_back(0xA4);
				packet.push_back(0x42);

				// Transaction ID (12 bytes)
				for (int i = 0; i < 12; ++i)
					packet.push_back(random::randu8());

#if 1
				// Add NONCE attribute if provided
				if (!nonce.empty())
				{
					// Attribute header: Type + Length
					packet.push_back((0x0015 >> 8) & 0xFF);
					packet.push_back(0x0015 & 0xFF);

					uint16_t nonce_length = static_cast<uint16_t>(nonce.length());
					packet.push_back((nonce_length >> 8) & 0xFF);
					packet.push_back(nonce_length & 0xFF);

					// Attribute value
					for (auto& c : nonce)
						packet.push_back(static_cast<u8>(c));

					// Padding to 4-byte boundary
					size_t padding = (4 - (nonce_length % 4)) % 4;
					for (size_t i = 0; i < padding; ++i)
					{
						packet.push_back(0x00);
					}
				}
#endif

				// Add USERNAME attribute if provided
				if (!username.empty())
				{
					// Attribute header: Type + Length
					packet.push_back((0x0006 >> 8) & 0xFF);
					packet.push_back(0x0006 & 0xFF);

					uint16_t username_length = static_cast<uint16_t>(username.length());
					packet.push_back((username_length >> 8) & 0xFF);
					packet.push_back(username_length & 0xFF);

					// Attribute value
					for (auto& c : username)
						packet.push_back(static_cast<u8>(c));

					// Padding to 4-byte boundary
					size_t padding = (4 - (username_length % 4)) % 4;
					for (size_t i = 0; i < padding; ++i)
					{
						packet.push_back(0x00);
					}
				}
#if 0
			// Add REALM attribute if provided
			if (!realm.empty())
			{
				// Attribute header: Type + Length
				packet.push_back((0x0014 >> 8) & 0xFF);
				packet.push_back(0x0014 & 0xFF);

				uint16_t realm_length = static_cast<uint16_t>(realm.length());
				packet.push_back((realm_length >> 8) & 0xFF);
				packet.push_back(realm_length & 0xFF);

				// Attribute value
				for(auto& c : realm)
					packet.push_back(static_cast<u8>(c));

				// Padding to 4-byte boundary
				size_t padding = (4 - (realm_length % 4)) % 4;
				for (size_t i = 0; i < padding; ++i)
				{
					packet.push_back(0x00);
				}
			}
#endif

#if 0
			{
				std::array<u8, 16> key{0x84, 0x93, 0xFB, 0xC5, 0x3B, 0xA5, 0x82, 0xFB, 0x4C, 0x04, 0x4C, 0x45, 0x6B, 0xDC, 0x40, 0xEB};
				uint16_t           keylen = static_cast<uint16_t>(key.size());

				auto hmac = hmac::sha1::hash(key, std::span<u8>{packet});


				// Message Integrity (HMAC-SHA1)
				// Attribute header: Type + Length
				packet.push_back((0x0008 >> 8) & 0xFF);
				packet.push_back(0x0008 & 0xFF);

				u16 hmaclen = static_cast<u16>(hmac.data().size());
				packet.push_back((hmaclen >> 8) & 0xFF);
				packet.push_back(hmaclen & 0xFF);



				for (auto& c : hmac.data())
					packet.push_back(static_cast<u8>(c));

				// Padding to 4-byte boundary
				size_t padding = (4 - (key.size() % 4)) % 4;
				for (size_t i = 0; i < padding; ++i)
				{
					packet.push_back(0x00);
				}
			}
#endif

#if 0
#endif
				{
					size_t padding = (4 - (packet.size() % 4)) % 4;
					for (size_t i = 0; i < padding; ++i)
					{
						packet.push_back(0x00);
					}
				}

				// Update message length in header
				uint16_t total_length = static_cast<uint16_t>(packet.size() - 20);
				packet[2]             = (total_length >> 8) & 0xFF;
				packet[3]             = total_length & 0xFF;

				// Create final array with actual size


				sent = sendto(
				  sock,
				  reinterpret_cast<const char*>(packet.data()),
				  static_cast<int>(packet.size()),
				  0,
				  reinterpret_cast<sockaddr*>(&storage),
				  static_cast<int>(addrlen));

				if (sent == SOCKET_ERROR)
				{
					dbg::println("sendto() failed: {}", WSAGetLastError());
					closesocket(sock);
					WSACleanup();
					return 1;
				}
				dbg::println("send 2 = {}", sent);

				// ----------------------- Set receive timeout (5 seconds) ---------------
				// ----------------------- Receive reply ---------------------------------
				std::vector<u8> incoming2{};
				incoming2.resize(1024);

				len = recvfrom(
				  sock, reinterpret_cast<char*>(incoming2.data()), static_cast<int>(incoming2.size()), 0, nullptr, nullptr);

				if (len == SOCKET_ERROR)
				{
					dbg::println("recvfrom() failed: {}", WSAGetLastError());
				}
				else
				{

					dbg::println("received {} bytes:", len);
					incoming2.resize(len);

					for (const auto& c : incoming2)
						dbg::print("{:02X} ", c);
					dbg::println("\n\n");
				}
			}

			// 01 01 00 18 21 12 A4 42 DD 0C 8D CE 70 1F 76 17 54 5A B3 0C 00 01 00 08 00 01 EE 6E D5 98 A1 0A 00 20 00
			// 08 00 01 CF 7C F4 8A 05 48


			// PAcket 2:
			// 01 01
			// 00 18
			// 21 12 A4 42
			// DE AD BE EF 69 CA FE BA BE 11 22 33
			//
			// 00 01  Mapped address
			// 00 08
			// 00 01 DC 6C D5 98 A1 0A 00 20 00 08 00 01 FD 7E F4 8A 05 48

			// 2a00:1678:2470:38:172e:6bff:5dbb:b0ae

			_ = 0;
			// Cloudflare
			// 01 01									0x0101 STUN response
			// 00 18									Length
			// 21 12 A4 42						ipv6.src == fd7d:76ee:e68f:a993:977:ca24:f8af:5038		MAGIC cookie
			// DE AD BE EF 69 CA FE BA BE 11 22 33		Transaction ID (same as sent)
			//
			// 00 20			XOR-mapped-address
			// 00 14			length
			// 00 02			padding byte, version byte (01 ipv4, 02 ipv6)
			// CD B2			port xor'd with MAGIC
			//
			// 01 13 B0 F8		xor with magic
			//					xor rest with transaction id
			// 98 AC BE EF 6D B3 3C 8D 02 7A C9 11


			// FB stun
			// 01 01			STUN Message type
			// 00 18			Message length
			// 21 12 A4 42		Magic cookie
			// DE AD BE EF 69 CA FE BA BE 11 22 33
			//
			// 00 01		 Attribute: MAPPED-ADDRESS
			// 00 08 0x0008: MESSAGE-INTEGRITY
			// 00 01 len
			// E6 14 D5 98 A1 EA 00 20 00 08 00 01 C7 06 F4 8A 05 A8


			// Google stun
			// 01 01
			// 00 0C
			// 21 12 A4 42
			// DE AD BE EF 69 CA FE BA BE 11 22 33
			// 00 20
			// 00 08						0x0008: MESSAGE-INTEGRITY
			// 00 01 DC 42 F4 8A 05 A8


			_ = 0;
		}

		// The plan:
		//
		//
		// net::address ntp_google("time.google.com", 123);
		// net::udpsocket sock(ntp_google);
		//
		// endpoint ep("time.google.com", 123);
		// net::udpsocket sock(ep);
		//
		// net::udpsocket sock("time.google.com", 123);
		// sock.timeout(500ms);
		//
		// sock.connect("time.google.com", 123);
		//
		// sock.bind, sock.listen, sock.accept
		//
		// sock.send(....) -> std::expected<u32, std::string>
		// sock.receive(...) -> std::expected<std::vector<u8>, std::string>
		// sock.receive_into(buffer) -> std::expected<size_t, std::string> (returns number of bytes received)
		//		- span<u8> buffer
		//
		// sock.send("hello", 5);
		// sock.send("hello"sv)
		//
		// std::array<u8, 48> ntp_request{};		-
		// sock.send(ntp_request); -> std::expected<u32, std::string>


		{
			constexpr size_t NTP_PACKET_SIZE = 48;

			config    ntpservers("ntp.txt"_path);
			const u16 default_port = ntpservers["servers.port"].as<u16>();
			auto      ntp_servers  = ntpservers["servers.host"].as_vector<net::endpoint>();
			u8        server_index = random::randu8(0, as<u8>(ntp_servers.size() - 1));

			if (not ntp_servers.empty())
			{

				if (ntpservers["servers.index"].as<i8>() >= 0)
					server_index = std::clamp(ntpservers["servers.index"].as<u8>(), 0_u8, as<u8>(ntp_servers.size() - 1));
				// server_index = 0;

				auto&       ntp_server = ntp_servers[server_index];
				u16         ntp_port   = (ntp_server.port != 0) ? ntp_server.port : default_port;
				std::string hostname   = ntp_server.hostname ? *ntp_server.hostname : "";

				// ----------------------- Resolve host ---------------------------------
				dbg::println("Resolving '{}'...", hostname);
				auto resolved = net::resolve_ips(hostname);
				if (not resolved or resolved->empty())
				{
					dbg::println("Failed to resolve '{}'", hostname);
					return 1;
				}


				for (const auto& ip : *resolved)
					dbg::println("  {} (IPv{})", ip, ip.version());

				auto [ntp_storage, ntp_addrlen] = resolved->front().to_sockaddr();

				if (resolved->front().is_ipv6())
					reinterpret_cast<sockaddr_in6&>(ntp_storage).sin6_port = htons(ntp_port);
				else
					reinterpret_cast<sockaddr_in&>(ntp_storage).sin_port = htons(ntp_port);

				// ----------------------- Create socket ---------------------------------
				SOCKET sock = socket(ntp_storage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
				if (sock == INVALID_SOCKET)
				{
					dbg::println("socket() failed: {}", WSAGetLastError());
				}

				DWORD timeoutMs = 5000;
				setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));


				// ----------------------- Build NTP request packet -----------------------
				std::array<u8, NTP_PACKET_SIZE> packet{};

				//                               2  3  3
				//                              LI VN  Mode
				//                               | |   |
				constexpr u8 ntp_config_byte = 0b0010'0011;

				// LI  : 0
				// VN  : 011 v3, 100 v4
				// Mode: 011 (3) client
				//


				auto chrono_to_poll = [](std::chrono::seconds duration) -> u8
				{
					if (duration.count() <= 0)
						return 0;

					auto poll = static_cast<int>(std::floor(std::log2(static_cast<f64>(duration.count()))));
					return static_cast<u8>(std::clamp(poll, 0, 10));
				};

				packet[0] = ntp_config_byte;
				packet[1] = 1; // stratum
				packet[2] = chrono_to_poll(8s);
				// packet[3] = static_cast<u8>(-20) & 0xFF; // Precision


				auto t1 = std::chrono::system_clock::now();

				u64 t1_packet = chrono_to_ntp(t1);


				// Origin timestamp
				write_be<u64>(packet, 24, t1_packet);

				// Transmit timestamp (optional)
				write_be<u64>(packet, 40, t1_packet);


				// ----------------------- Send request ----------------------------------
				int sent = sendto(
				  sock,
				  reinterpret_cast<const char*>(packet.data()),
				  static_cast<int>(packet.size()),
				  0,
				  reinterpret_cast<sockaddr*>(&ntp_storage),
				  static_cast<int>(ntp_addrlen));

				if (sent == SOCKET_ERROR)
				{
					dbg::println("sendto() failed: {}", WSAGetLastError());
					closesocket(sock);
					WSACleanup();
					return 1;
				}
				dbg::println("send = {}", sent);

				// ----------------------- Set receive timeout (5 seconds) ---------------
				// ----------------------- Receive reply ---------------------------------
				int recvLen = recvfrom(
				  sock, reinterpret_cast<char*>(packet.data()), static_cast<int>(packet.size()), 0, nullptr, nullptr);

				auto t4 = std::chrono::system_clock::now();

				if (recvLen == SOCKET_ERROR)
				{
					dbg::println("{} recvfrom() failed (timeout?): {}", hostname, WSAGetLastError());
					closesocket(sock);
					WSACleanup();
				}

				if (recvLen < static_cast<int>(NTP_PACKET_SIZE))
				{
					dbg::println("Received packet too short ({}) bytes", recvLen);
					closesocket(sock);
				}

				if (recvLen == NTP_PACKET_SIZE)
				{


					auto ntp = parse_ntp(packet, t1, t4);

					dbg::println("{:<20}: {}", "Leap", ntp.leapIndicator);

					dbg::println("{:<20}: {}", "Precision", ntp.precision);
					dbg::println("{:<20}: {}", "Root delay", ntp.root_delay);
					dbg::println("{:<20}: {}", "Reference ID", ntp.ref_id_string);
					dbg::println("{:<20}: {}", "Root dispersion", ntp.root_dispersion);
					dbg::println("{:<20}: {}", "Reference time", ntp.refTimestamp);
					dbg::println("{:<20}: {}", "Origin time", ntp.origTimestamp);
					dbg::println("{:<20}: {}", "Receive time", ntp.rxTimestamp);
					dbg::println("{:<20}: {}", "Transmit time", ntp.txTimestamp);

					dbg::println("{:<20}: {}", "Round trip", ntp.roundtrip_delay);
					dbg::println("{:<20}: {}", "Clock offset", ntp.local_clock_offset);
					dbg::println("{:<20}: {}", "Unix Epoch", ntp.unix_epoch);
					dbg::println("{:<20}: {}", "Local Epoch", epoch());
					dbg::println();
					dbg::println("NTP from {}", hostname);
					dbg::println("{:<20}: {}", "Version", ntp.version);
					dbg::println("{:<20}: {}", "Mode", ntp.mode);
					dbg::println("{:<20}: {}", "Stratum", ntp.stratum);
					dbg::println("{:<20}: {}", "Poll", ntp.poll);
				}
				_ = 0;

				// ---------------------


				closesocket(sock);
			}
		}

		// ########################################################################


		// ########################################################################


		// ########################################################################
		if constexpr (false)
		{
			constexpr size_t mof_size  = sizeof(std::move_only_function<void()>);
			constexpr size_t stdf_size = sizeof(std::function<void()>);
			constexpr size_t fr_size   = sizeof(function_ref<void()>);
			dbg::println(
			  "move_only_function size: {} / std::function size: {}, function_ref size: {}", mof_size, stdf_size, fr_size);

			using function_t = function_ref<void()>;

			std::mutex              mtx;
			std::mutex              vfr_mutex;
			std::deque<function_t>  vfr;
			std::condition_variable cv;
			std::condition_variable cv1;

			std::atomic_int counter{0}, threader_tasks{0};

			std::atomic_bool stop{false};

			bool ready{false};
			for (int i = 0; i < 20; i++)
			{
				vfr.push_back(
				  [i, &counter]
				  {
					  std::this_thread::sleep_for(std::chrono::seconds(2));
					  counter++;
				  });
			}

			vfr.emplace_back([] { dbg::println("hello from function_ref 1!"); });
			vfr.emplace_back(
			  []
			  {
				  dbg::println("hello from function_ref 2!");
				  std::this_thread::sleep_for(10s);
			  });
			vfr.emplace_back(
			  []
			  {
				  dbg::println("hello from function_ref 3!");
				  std::this_thread::sleep_for(5s);
			  });
			vfr.emplace_back([] { dbg::println("hello from function_ref 4!"); });


			auto tasker = [&]()
			{
				dbg::println("\ttasker started");


				dbg::println("\ttasker loop started");


				while (true)
				{
					function_t task{[] { }};
					{

						std::unique_lock vlock(vfr_mutex);
						cv1.wait(vlock, [&] { return not vfr.empty(); });

						if (vfr.empty())
							break;

						task = vfr.front();
						vfr.pop_front();
					}
					threader_tasks++;
					task();
				}
				dbg::println("\ttasker loop ended");
			};

			std::vector<std::thread> tasks;
			tasks.reserve(10);
			for (int i = 0; i < 10; ++i)
				tasks.emplace_back(tasker);

			dbg::println("tasks waiting...");

			cv.notify_all();
			ready = true;

			dbg::println("tasks started");

			for (int i = 0; i < 5; i++)
			{
				dbg::println("main thread {}", i);
				std::this_thread::sleep_for(1s);
			}

			dbg::println("joining...");
			cv1.notify_all();

			for (auto& t : tasks)
				t.join();

			dbg::println("joined");
			dbg::println("tasks run {} / {}", counter.load(), threader_tasks.load());
			_;
		}

		// ########################################################################
		if constexpr (false)
		{
			auto               tpool_start = clock_now();
			taskpool::taskpool tpool;

			clock_delta("threadpool init", tpool_start);

			auto task1 = []
			{
				dbg::println("Hello from threadpool!");
				return 42;
			};

			auto task2 = []
			{
				std::this_thread::sleep_for(2s);
				dbg::println("long task");
				return std::string("long done");
			};

			auto t2 = tpool.enqueue(task2);
			clock_delta("q2", tpool_start);

			auto t1 = tpool.enqueue(task1);
			clock_delta("q1", tpool_start);

			auto task3 = []
			{
				std::this_thread::sleep_for(1s);
				dbg::println("short task");
				return 666.314;
			};

			auto t3 = tpool.enqueue(task3);

			for (int i = 0; i < 5; i++)
			{
				tpool.enqueue(
				  [i]
				  {
					  dbg::println("{} task", i);
					  std::this_thread::sleep_for(2s);
					  return i * 2;
				  });
			}


			clock_delta("300 enq", tpool_start);


			{
				dbg::println("work on main thread");

				for (int i = 0; i < 5; i++)
				{
					dbg::println("main thread working... {}", i);
					std::this_thread::sleep_for(1s);
				}
				dbg::println("done main thread");
			}

			dbg::println("waiting for tasks...");
			int rt1 = t1.get();
			f64 rt3 = t3.get();
			clock_delta("get 1/3", tpool_start);
			dbg::println("results: {} / {}", rt1, rt3);

			std::string rt2 = t2.get();
			clock_delta("get 2 long", tpool_start);
			dbg::println("result2: {}", rt2);

			tpool.join();
			clock_delta<std::chrono::seconds>("join", tpool_start);
		}

		_;


		//
		// std::string ipv6("2001:0db8:85a3::8a2e:0370:7334");

		//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
		// 20 01 0d b8 00 00 00 00 00 01 00 00 00 00 00 01
		// 192.168.1.1 - c0.ab.01.01
		//  ::ffff:c0ab:0101
		//
		//  127.0.0.1 - ::ffff:7f00:1


		// ########################################################################
		// ✅ ❌


		// Lexer       lexer(input);
		// Parser      parser(lexer);
		// int         result = parser.expression();
		// dbg::println("Result: {}", result);


		// Environment
		//	std::unordered_map<std::string, int> env;
		//
		//	// Create an AST
		//	std::unique_ptr<Node> ast =
		//	  create_assign_node("x", create_bin_op_node('+', create_num_node(2), create_bin_op_node('*',
		// create_num_node(3),
		// create_num_node(6))));
		//
		//	std::unordered_map<std::string, int> constants;
		//
		//	print_ast(ast);
		//	auto code = generate_bytecode(ast, constants);
		//	interpret_ast(ast, env);
		//	dbg::println("x = {}", env["x"]);
		//


		// TODO: register key bindings to apps own enum
		//
		// enum player_movement
		// up,down,left,right, fire
		// bind(KEY_LEFT, left)
		// bind(KEY_SPACE, fire)  // both space and pad_a fires
		// bind(PAD_A, fire)

		// special enter textmode for input
		// keys.enter_text_mode()
		// end_text_mode(), // inputs keys as text?
	}
}
