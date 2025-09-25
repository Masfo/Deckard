#include <windows.h>
#include <Commctrl.h>
#include <intrin.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>

import std;
import deckard;
import deckard.types;
import deckard.timers;
using namespace deckard;
using namespace deckard::app;
using namespace deckard::random;
// using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;


#ifndef _DEBUG
import window;
#endif

std::array<unsigned char, 256> previous_state{0};
std::array<unsigned char, 256> current_state{0};

void keyboard_callback(vulkanapp& app, i32 key, i32 scancode, Action action, i32 mods)
{
	dbg::println("key: {:#x} - {:#x}, {} - {}", key, scancode, action == Action::Up ? "UP" : "DOWN", mods);

	bool up = action == Action::Up;

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
		// app.set(Attribute::vsync);
	}

	if (key == Key::F2 and up)
	{
		app.resize(1280, 720);
	}

	if (up and (key == Key::F11 or key == Key::F))
	{
		// app.set(Attribute::togglefullscreen);
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


template<typename T>
std::vector<std::pair<u32, char>> compress_rle(const T input)
{
	// WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW
	// 12W 1B 12W 3B 24W 1B 14W
	std::vector<std::pair<u32, char>> ret;

	ret.reserve(input.size() * 2);

	for (size_t i = 0; i < input.size(); i++)
	{
		u32 run = 1;
		while (i < input.size() - 1 and input[i] == input[i + 1])
		{
			i++;
			run++;
		}
		ret.push_back({run, input[i]});
	}

	ret.shrink_to_fit();

	return ret;
}

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

#if 0
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


template<typename T>
struct Noisy
{
	Noisy() { dbg::println("{default-ctor}"); }

	explicit Noisy(T t)
		: value_{std::move(t)}
	{
		dbg::println("[default-ctor] {}", value_);
	}

	~Noisy() { dbg::println("[dtor] {}", value_); }

	Noisy(const Noisy& other)
		: value_{other.value_}
	{

		dbg::println("[copy-ctor] {}", other.value_);
	}

	Noisy(Noisy&& other) noexcept
	{
		dbg::println("[move-ctor] {} = {}", value_, other.value_);
		std::swap(value_, other.value_);
	}

	Noisy& operator=(const Noisy& other)
	{
		dbg::println("[copy-assign] {} = {}", value_, other.value_);
		value_ = other.value_;
		return *this;
	}

	Noisy& operator=(Noisy&& other) noexcept
	{
		dbg::println("[move-assign] {} = {}", value_, other.value_);
		value_ = std::move(other.value_);
		return *this;
	}

	T value_{0};
};

u32 BinaryToGray(u32 num) { return num ^ (num >> 1); }

std::generator<u32> gen()
{
	u32 num = 0;
	while (true)
	{
		co_yield ++num;
		if (num == 10)
			break;
	}
}

using m128 = __m128;

union fvec4data
{
	struct xyz
	{
		f32 x, y, z, w;
	} c;

	f32 element[4]{0.0f};

	m128 reg;
};

struct alignas(16) fvec4
{
private:
	fvec4data data;

public:
	fvec4() { data.c.x = data.c.y = data.c.z = data.c.w = 0.0f; }

	f32 operator[](i32 index) const
	{
		switch (index)
		{
			case 0: return data.c.x;
			case 1: return data.c.y;
			case 2: return data.c.z;
			case 3: return data.c.w;
		}
	}
};

class Secret
{
private:
	int secret_value{42};

private:
	friend int operator+(const Secret& lhs, const Secret& rhs);
	friend int operator-(const Secret& lhs, const Secret& rhs);


public:
	Secret() = default;

	Secret(int value)
		: secret_value(value)
	{
	}

	int get() const { return secret_value; }

	void set(int value) { secret_value = value; }
};

auto operator+(const Secret& lhs, const Secret& rhs) -> int { return lhs.secret_value + rhs.secret_value; }

auto operator-(const Secret& lhs, const Secret& rhs) -> int { return lhs.secret_value - rhs.secret_value; }

int function_call(int input) { return input * 10; }

struct boolflag
{
	boolflag(std::string_view name, bool b)
		: name(name)
		, flag(b)
	{
	}

	std::string name;
	bool        flag{false};
};

class clitest
{
private:
	std::vector<boolflag> boolflags;

public:
	void add_flag(std::string_view str, bool& flag) { boolflags.push_back({str, flag}); }

	void dump()
	{
		for (const auto& bf : boolflags)
		{
			dbg::println("{}: {}", bf.name, bf.flag ? "true" : "false");
		}
	}
};

struct CmdOptions
{
	std::unordered_map<std::string, std::string> options;    // --option=value or --option value
	std::unordered_set<std::string>              flags;      // --flag or -f
	std::vector<std::string>                     positional; // positional arguments
};

CmdOptions parse_command_line(std::string_view cmdl)
{
	CmdOptions               result;
	std::vector<std::string> tokens;
	std::string              current;
	bool                     in_quotes = false;

	// Tokenize, handling quotes
	for (size_t i = 0; i < cmdl.size(); ++i)
	{
		char c = cmdl[i];
		if (c == '"')
		{
			in_quotes = !in_quotes;
		}
		else if (std::isspace(static_cast<unsigned char>(c)) && !in_quotes)
		{
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
		}
		else
		{
			current += c;
		}
	}
	if (!current.empty())
		tokens.push_back(current);

	constexpr int min_level = 0;
	constexpr int max_level = 3;

	for (size_t i = 0; i < tokens.size(); ++i)
	{
		const std::string& token = tokens[i];
		if (token.starts_with("--"))
		{
			auto eq = token.find('=');
			if (eq != std::string::npos)
			{
				std::string key     = token.substr(2, eq - 2);
				std::string value   = token.substr(eq + 1);
				result.options[key] = value;
			}
			else
			{
				std::string key = token.substr(2);
				// Check if next token is a value (not starting with -)
				if (i + 1 < tokens.size() && !tokens[i + 1].starts_with('-'))
				{
					result.options[key] = tokens[i + 1];
					++i;
				}
				else
				{
					result.flags.insert(key);
				}
			}
		}
		else if (token.starts_with('-') && token.size() > 1)
		{
			// Special case: -O2, -O3, etc. (optimization level)
			if (token[1] == 'O' && token.size() > 2 && std::isdigit(token[2]))
			{
				int level           = token[2] - '0';
				level               = std::clamp(level, min_level, max_level);
				result.options["O"] = std::to_string(level);
				continue;
			}
			// Short flags, possibly grouped: -abc
			for (size_t j = 1; j < token.size(); ++j)
			{
				std::string key(1, token[j]);
				result.flags.insert(key);
			}
		}
		else
		{
			result.positional.push_back(token);
		}
	}
	return result;
}

struct ComponentBase
{
	virtual std::unique_ptr<ComponentBase> clone() const = 0;

	virtual ~ComponentBase() { }
};

struct Transform : public ComponentBase
{
	Transform() = default;
	float x, y, z;

	std::unique_ptr<ComponentBase> clone() const { return std::make_unique<Transform>(*this); }

	Transform copy() const { return *static_cast<Transform*>(clone().get()); }
};

struct Velocity : public ComponentBase
{
	Velocity() = default;

	Velocity(f32 x, f32 y, f32 z)
		: x(x)
		, y(y)
		, z(z)
	{
	}

	std::unique_ptr<ComponentBase> clone() const { return std::make_unique<Velocity>(*this); }

	Velocity copy() const { return *static_cast<Velocity*>(clone().get()); }

	float x, y, z;
};

struct Name : public ComponentBase
{
	Name() = default;

	Name(std::string_view s)
		: name(s)
	{
	}

	std::string name;

	std::unique_ptr<ComponentBase> clone() const override { return std::make_unique<Name>(*this); }

	Name copy() const { return *static_cast<Name*>(clone().get()); }
};

struct Health : public ComponentBase
{
	Health() = default;

	Health(f32 f)
		: value(f)
	{
	}

	f32 value{1.0f};

	std::unique_ptr<ComponentBase> clone() const override { return std::make_unique<Health>(*this); }

	Health copy() const { return *static_cast<Health*>(clone().get()); }
};

enum class Components : u8
{
	Transform,
	Velocity,
	Name,
	Health,


	Count,
};


template<typename T>
concept ComponentHasCount = requires {
	{ T::Count } -> std::convertible_to<T>;
};

template<typename T>
class ECS
{
	static_assert(std::is_scoped_enum_v<T> and ComponentHasCount<T>, "ECS enum must have a count. T::Count");

private:
	std::array<std::vector<std::unique_ptr<ComponentBase>>, std::to_underlying(T::Count)> dense_components{};

public:
	template<typename R, typename... Args>
	void insert(const T index, Args... args)
	{
		assert::check(std::to_underlying(index) < std::to_underlying(T::Count), "Index out-of-bounds");

		dense_components[std::to_underlying(index)].emplace_back(std::make_unique<R>(args...));
	}
};

class test_span_operator
{
private:
	std::vector<u8> buffer;

public:
	test_span_operator(const std::vector<u8>& b)
		: buffer(b)
	{
	}

	std::expected<std::span<u8>, std::string> operator[](size_t index, size_t count) const
	{

		if (index + count > buffer.size())
			return std::unexpected(std::format("cannot index to {}, only has {} items", index + count, buffer.size()));

		return std::span<u8>{(u8*)buffer.data() + index, count};
	};
};

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
	constexpr u32 NTP_UNIX_OFFSET = 2'208'988'800U;

	u32 seconds  = static_cast<u32>(ntp_ts >> 32);
	u32 fraction = static_cast<u32>(ntp_ts & 0xFFFF'FFFFULL);


	f64  frac_seconds = static_cast<f64>(fraction) / 4294967296.0;
	auto secs         = seconds - NTP_UNIX_OFFSET;

	auto ntp_time = std::chrono::system_clock::time_point{
	  std::chrono::seconds(secs) + duration_cast<std::chrono::system_clock::duration>(duration<f64>(frac_seconds))};

	return ntp_time;
}

u64 chrono_to_ntp(std::chrono::system_clock::time_point tp)
{
	using namespace std::chrono;
	constexpr u32 NTP_UNIX_OFFSET = 2'208'988'800U;

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
	constexpr u32 NTP_UNIX_OFFSET = 2'208'988'800U;

	u32 seconds = static_cast<u32>(ntp_ts >> 32);
	u32 secs    = seconds - NTP_UNIX_OFFSET;
	return secs;
}

NtpPacket parse_ntp(std::span<const u8> raw, std::chrono::system_clock::time_point t1, std::chrono::system_clock::time_point t4)
{
	if (raw.size() < 48)
		return {};

	NtpPacket pkt{};

	uint8_t li_vn_mode = raw[0];
	pkt.leapIndicator  = (li_vn_mode >> 6) & 0x03;
	pkt.version        = (li_vn_mode >> 3) & 0x07;
	pkt.mode           = li_vn_mode & 0x07;

	pkt.stratum = raw[1];

	// Poll
	i8  poll         = raw[2];
	f64 poll_seconds = 0;
	if (poll < 0)
		poll_seconds = 1.0 / (1ULL << -poll);
	else
	{
		if (poll < 64)
			poll_seconds = static_cast<f64>(1ULL << poll);
		else
			poll_seconds = std::pow(2.0, poll);
	}

	pkt.poll = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<f64>(poll_seconds));

	// Precision
	i8  raw_precision     = raw[3];
	f64 precision_seconds = 1.0 / (1ULL << (std::abs(raw_precision)));
	pkt.precision         = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<f64>(precision_seconds));


	// pkt.rootDelay =
	u32 root_delay = load_as<u32>(raw, 4).value_or(0);
	{
		i16 int_part  = root_delay >> 16;
		u16 frac_part = root_delay & 0xFFFF;

		f64 frac_seconds       = static_cast<f64>(frac_part) / 65536.0;
		f64 root_delay_seconds = int_part + frac_seconds;

		pkt.root_delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<f64>(root_delay_seconds));
	}


	// Root dispersion
	u32 root_dispersion = load_as<u32>(raw, 8).value_or(0);
	{
		u16 int_part  = root_dispersion >> 16;
		u16 frac_part = root_dispersion & 0xFFFF;

		f64 frac_seconds            = static_cast<f64>(frac_part) / 65536.0;
		f64 root_dispersion_seconds = int_part + frac_seconds;
		pkt.root_dispersion = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<f64>(root_dispersion_seconds));
	}


	pkt.refId = load_as<u32>(raw, 12).value_or(0);

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
		pkt.ref_id_string = std::format("{}.{}.{}.{} ({:X})", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, ip);
	}
	else if (pkt.stratum > 16)
	{
		pkt.ref_id_string = "Reserved/Unknown";
	}


	pkt.unix_epoch = to_unix_epoch(load_as<u64>(raw, 32).value_or(0));


	pkt.refTimestamp = ntp_to_chrono(load_as<u64>(raw, 16).value_or(0));

	auto origtime = load_as<u64>(raw, 24);

	pkt.origTimestamp = ntp_to_chrono(load_as<u64>(raw, 24).value_or(0));
	pkt.rxTimestamp   = ntp_to_chrono(load_as<u64>(raw, 32).value_or(0));
	pkt.txTimestamp   = ntp_to_chrono(load_as<u64>(raw, 40).value_or(0));

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
	const auto*                   tz2 = std::chrono::locate_zone("Asia/Tokyo");
	const std::chrono::zoned_time tokyo_time{tz, pkt.origTimestamp};

	// Extract the system_clock::time_point from the zoned_time
	const auto sys_tp   = tokyo_time.get_sys_time();
	const auto local_tp = tokyo_time.get_local_time();

	// Print the times to demonstrate the conversion
	dbg::println("Original zoned_time (in Tokyo): {}", std::format("{:%Y-%m-%d %H:%M:%S %Z}", tokyo_time));

	dbg::println("Converted time_point (UTC): {}", sys_tp);
	dbg::println("Converted time_point (UTC): {}", local_tp);

	std::chrono::local_time local_new = tokyo_time.get_local_time();
	dbg::println("Converted time_point (UTC): {}", local_new);


	return pkt;
}

template<size_t N>
struct xkey
{
	std::array<u8, N> data;
};

i32 deckard_main([[maybe_unused]] utf8::view commandline)
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", window::build::version_string, window::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif

	// ########################################################################
	std::array<u32, 4> state{0xDEAD'BEEF, 0xCAFE'BABE, 0x0BAD'F00D, 0xB00b'C0DE};

	auto byts = std::as_bytes(std::span(state));


	// Fixed‑size array of two uint32_t values.
	std::array<std::uint32_t, 8> numbers = {0x1122'3344u, 0xAABB'CCDDu};

	// Create a span that covers the whole array.

	auto bytes_wr = std::span<u8>((u8*)numbers.data(), numbers.size() * sizeof(u32));

	std::span<std::uint32_t> num_span{numbers};
	auto                     bytes_rd = std::as_bytes(num_span);

	_ = 0;

	// ########################################################################


	auto test_sha1 = sha256::quickhash(""sv);
	// e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855

	// ########################################################################

	std::array<u8, 4>  key{0x4a, 0x65, 0x66, 0x65};
	std::array<u8, 28> data{0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6f, 0x20, 0x79, 0x61, 0x20, 0x77, 0x61, 0x6e,
							0x74, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x6e, 0x6f, 0x74, 0x68, 0x69, 0x6e, 0x67, 0x3f};


	auto hmac1 = hmac::sha1::hash(key, data);
	// effcdf6ae5eb2fa2d27416d5f184df9c259a7c79

	auto sha256 = sha256::hash("abc");
	// ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad

	sha256::digest sha256_n("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

	auto sha512 = sha512::quickhash("abc");
	// ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f
	auto sha512_n = sha512::hash("abc");

	auto hmac256_ne = hmac::sha512::hash("Jefe", "what do ya want for nothing?");
	// 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
	// 164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737


	auto hmac256_o = hmac::sha256::hash(key, data).to_string();

	_ = 0;

	// ########################################################################


	std::array<u8, 32> saas{0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e, 0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
							0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83, 0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43};

	generic_sha_digest<32> d256("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
	generic_sha_digest<32> d2562(saas);
	generic_sha_digest<32> d2563{0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e, 0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
								 0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83, 0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43};

	dbg::println("d256:  {} {}", d256 == d2562, d2562 == d2563);

	auto d122 = d256.data();

	// ########################################################################

		const std::array<std::string, 5> names{"Alice", "Bob", "Eve", "David", "Carl"};

		dbg::println("Names: {}", names);
		_ = 0;

	// ########################################################################
	{
		std::vector<u8> binr;
		binr.resize(256);
		for (int i = 0; i < binr.size(); ++i)
			binr[i] = as<u8>(i);

		test_span_operator spo(binr);

		auto view = spo[128, 128];

		_ = 0;
	}
	{
		std::array<std::pair<std::string, std::string>, 4> hostnames{
		  //
		  std::pair{"stun.fbsbx.com", "3478"},
		  std::pair{"stun2.l.google.com", "19302"},
		  std::pair{"stun.l.google.com", "19302"},
		  std::pair{"stun.cloudflare.com", "3478"},
		};
		u8 hostname_index = random::randu8(0, as<u8>(hostnames.size() - 1));
		hostname_index    = 0;

		std::string_view hostname = hostnames[hostname_index].first;
		std::string_view service  = hostnames[hostname_index].second;


		// ----------------------- Resolve host ---------------------------------
		addrinfo hints{}, *result = nullptr;
		hints.ai_family   = AF_UNSPEC;  // IPv4 or IPv6
		hints.ai_socktype = SOCK_DGRAM; // UDP
		hints.ai_protocol = IPPROTO_UDP;

		int rc = getaddrinfo(hostname.data(), service.data(), &hints, &result);
		if (rc != 0)
		{
			dbg::println("getaddrinfo: {}", gai_strerrorA(rc));
		}

		// ip
		std::string resolved_ip;
		char        ip_str[INET6_ADDRSTRLEN]; // Buffer for IPv4 or IPv6
		auto        addr = result->ai_addr;

		if (addr->sa_family == AF_INET)
		{
			struct sockaddr_in* sin = (struct sockaddr_in*)addr;
			if (inet_ntop(AF_INET, &sin->sin_addr, ip_str, INET_ADDRSTRLEN) == nullptr)
				dbg::println("Invalid IP");
		}
		else if (addr->sa_family == AF_INET6)
		{
			struct sockaddr_in6* sin6 = (struct sockaddr_in6*)addr;
			if (inet_ntop(AF_INET6, &sin6->sin6_addr, ip_str, INET6_ADDRSTRLEN) == nullptr)
				dbg::println("Invalid IP");
		}
		else
		{
			dbg::println("Unsupported address family");
		}

		resolved_ip = std::string(ip_str);
		dbg::println("{} hosted @ {}", hostname, resolved_ip);


		// ----------------------- Create socket ---------------------------------
		SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			dbg::println("socket() failed: {}", WSAGetLastError());
			freeaddrinfo(result);
		}

		std::array<uint8_t, 20> packet{
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

		  // Transaction ID: A random, 96-bit (12-byte) identifier.
		  // This example uses a placeholder, but should be generated randomly for a real application.
		  // For example: `0x6f, 0x4c, 0x3a, 0x6e, 0x5a, 0x7b, 0x73, 0x9c, 0x2d, 0x82, 0x4f, 0x3a`
		  0xDE,
		  0xAD,
		  0xBE,
		  0xEF,
		  0x69,
		  0xCA,
		  0xFE,
		  0xBA,
		  0xBE,
		  0x11,
		  0x22,
		  0x33};

		DWORD timeoutMs = 5000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));


		int sent = sendto(
		  sock,
		  reinterpret_cast<const char*>(packet.data()),
		  static_cast<int>(packet.size()),
		  0,
		  result->ai_addr,
		  static_cast<int>(result->ai_addrlen));

		if (sent == SOCKET_ERROR)
		{
			dbg::println("sendto() failed: {}", WSAGetLastError());
			closesocket(sock);
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}
		dbg::println("send = {}", sent);

		// ----------------------- Set receive timeout (5 seconds) ---------------
		// ----------------------- Receive reply ---------------------------------

		std::vector<u8> incoming{};
		incoming.resize(1024);

		int len = recvfrom(sock, reinterpret_cast<char*>(incoming.data()), static_cast<int>(incoming.size()), 0, nullptr, nullptr);

		if (len == SOCKET_ERROR)
		{
			dbg::println("recvfrom() failed: {}", WSAGetLastError());
		}
		else
		{

			dbg::println("received {} bytes:", len);
			incoming.resize(len);

			for (const auto& c : incoming)
				dbg::print("{:02X} ", c);
			dbg::println("\n\n");
		}

		// FB stun
		// 01 01
		// 00 18
		// 21 12 A4 42
		// DE AD BE EF 69 CA FE BA BE 11 22 33
		// 00 01
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


		// Cloudflare
		// 01 01									0x0101 STUN response
		// 00 18									Length
		// 21 12 A4 42								MAGIC cookie
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

		_ = 0;
	}

	///
	{
		constexpr size_t NTP_PACKET_SIZE = 48;

		std::array<std::string_view, 5> hostnames{
		  //
		  "time.google.com"sv,
		  "fi.pool.ntp.org"sv,
		  "pool.ntp.org"sv,
		  "time.cloudflare.com"sv,
		  "time.windows.com"sv};

		u8 hostname_index = random::randu8(0, as<u8>(hostnames.size() - 1));
		// hostname_index    = 0;

		std::string_view hostname = hostnames[hostname_index];

		const char* service = "123"; // NTP uses UDP port 123

									 // ----------------------- Resolve host ---------------------------------
		addrinfo hints{}, *result = nullptr;
		hints.ai_family   = AF_UNSPEC;  // IPv4 or IPv6
		hints.ai_socktype = SOCK_DGRAM; // UDP
		hints.ai_protocol = IPPROTO_UDP;

		int rc = getaddrinfo(hostname.data(), service, &hints, &result);
		if (rc != 0)
		{
			dbg::println("getaddrinfo: {}", gai_strerrorA(rc));
		}

		// ip
		std::string resolved_ip;
		char        ip_str[INET6_ADDRSTRLEN]; // Buffer for IPv4 or IPv6
		auto        addr = result->ai_addr;

		if (addr->sa_family == AF_INET)
		{
			struct sockaddr_in* sin = (struct sockaddr_in*)addr;
			if (inet_ntop(AF_INET, &sin->sin_addr, ip_str, INET_ADDRSTRLEN) == nullptr)
				dbg::println("Invalid IP");
		}
		else if (addr->sa_family == AF_INET6)
		{
			struct sockaddr_in6* sin6 = (struct sockaddr_in6*)addr;
			if (inet_ntop(AF_INET6, &sin6->sin6_addr, ip_str, INET6_ADDRSTRLEN) == nullptr)
				dbg::println("Invalid IP");
		}
		else
		{
			dbg::println("Unsupported address family");
		}

		resolved_ip = std::string(ip_str);
		dbg::println("{} hosted @ {}", hostname, resolved_ip);


		// ----------------------- Create socket ---------------------------------
		SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			dbg::println("socket() failed: {}", WSAGetLastError());
			freeaddrinfo(result);
		}

		DWORD timeoutMs = 5000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));


		// ----------------------- Build NTP request packet -----------------------
		std::array<uint8_t, NTP_PACKET_SIZE> packet{};

		// clang-format off
		//                                2   3   3   
		//                               LI  VN   Mode
		//                                |   |   |   
		constexpr u8 ntp_config_byte = 0b00'100'011;
		// clang-format on

		// LI  : 0
		// VN  : 011 v3, 100 v4
		// Mode: 011 (3) client
		//


		auto chrono_to_poll = [](std::chrono::seconds duration) -> u8
		{
			if (duration.count() <= 1.0)
				return 0;


			f64 log2_value = std::log2(duration.count());
			u8  poll       = static_cast<u8>(std::floor(log2_value));

			return std::clamp<u8>(poll, 0, 10); // Max 2^10 seconds- 1024
		};

		packet[0] = ntp_config_byte;
		packet[1] = 1; // stratum
		packet[2] = chrono_to_poll(8s);
		// packet[3] = static_cast<u8>(-20) & 0xFF; // Precision


		auto t1 = std::chrono::system_clock::now();

		u64 t1_packet = chrono_to_ntp(t1);


		// Origin timestamp
		for (int i = 0; i < 8; ++i)
			packet[24 + i] = static_cast<u8>(t1_packet >> (56 - 8 * i));

		// Transmit timestamp (optional)
		for (int i = 0; i < 8; ++i)
			packet[40 + i] = static_cast<u8>(t1_packet >> (56 - 8 * i));


		// ----------------------- Send request ----------------------------------
		int sent = sendto(
		  sock,
		  reinterpret_cast<const char*>(packet.data()),
		  static_cast<int>(packet.size()),
		  0,
		  result->ai_addr,
		  static_cast<int>(result->ai_addrlen));

		if (sent == SOCKET_ERROR)
		{
			dbg::println("sendto() failed: {}", WSAGetLastError());
			closesocket(sock);
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}
		dbg::println("send = {}", sent);

		// ----------------------- Set receive timeout (5 seconds) ---------------
		// ----------------------- Receive reply ---------------------------------
		int recvLen = recvfrom(sock, reinterpret_cast<char*>(packet.data()), static_cast<int>(packet.size()), 0, nullptr, nullptr);

		auto t4 = std::chrono::system_clock::now();

		if (recvLen == SOCKET_ERROR)
		{
			dbg::println("{} recvfrom() failed (timeout?): {}", hostname, WSAGetLastError());
			closesocket(sock);
			freeaddrinfo(result);
			WSACleanup();
		}

		if (recvLen < static_cast<int>(NTP_PACKET_SIZE))
		{
			dbg::println("Received packet too short ({}) bytes", recvLen);
			closesocket(sock);
			if (not result)
				freeaddrinfo(result);
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
		freeaddrinfo(result);
	}

	// ########################################################################

	std::array<u8, 256> binr;
	for (int i = 0; i < binr.size(); ++i)
	{
		binr[i] = as<u8>(i);
	}

	std::filesystem::path binfile("data/../256.bin");

	auto res = file::v2::write(binfile, binr, true);
	if (res)
		dbg::println("binr ok {}", *res);
	else
		dbg::println("{}", res.error());


	std::span<u8> view{};

	HANDLE handle = CreateFileW(
	  binfile.wstring().c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
	{
		dbg::println("Could not open file: {}", system::from_wide(binfile.wstring().c_str()));
	}

	u64 size = file::v2::filesize(binfile).value_or(0);


	HANDLE mapping = CreateFileMapping(handle, 0, PAGE_READWRITE, 0, 0, nullptr);
	if (mapping == nullptr)
	{
		FlushViewOfFile(view.data(), 0);
		UnmapViewOfFile(view.data());
		view = {};
	}


	u8* raw_address = as<u8*>(MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0));
	if (raw_address == nullptr)
	{
		FlushViewOfFile(view.data(), 0);
		UnmapViewOfFile(view.data());
		view = {};
	}
	else
	{
		// Resize
		u32 newsize = 512;
		SetFilePointerEx(
		  handle,
		  LARGE_INTEGER{
			.LowPart  = newsize,
			.HighPart = 0,
		  },
		  nullptr,
		  FILE_BEGIN);
		SetEndOfFile(handle);

		LARGE_INTEGER filesize{0};
		GetFileSizeEx(handle, &filesize);

		CloseHandle(handle);
		handle = nullptr;

		CloseHandle(mapping);
		mapping = nullptr;

		size = filesize.LowPart;
		view = std::span<u8>{as<u8*>(raw_address), size};


		//
		view[0] = random::randu8();


		if (0 == FlushViewOfFile(view.data(), 0))
		{
			dbg::println("flush :{}", system::get_windows_error(GetLastError()));
		}
		if (0 == UnmapViewOfFile(view.data()))
		{
			dbg::println("unmap: {}", system::get_windows_error(GetLastError()));
		}
	}

	_ = 0;

	// ########################################################################

	/*
	 *  vector<vector<InterfaceComponent>> components
	 *
	 *  class Transform: InterfaceComponent
	 *  class Velocity: InterfaceComponentsd
	 *
	 *  enum: Transform = 0, Velocity = 1
	 *
	 *  auto all_transforms = &components[Transform];
	 *  auto all_velocity = &components[Velocity];
	 *
	 */

	{
		ECS<Components> ecs;
		ecs.insert<Velocity>(Components::Velocity, 1.0f, 1.0f, 6.0f);
		ecs.insert<Velocity>(Components::Velocity, 1.0f, 2.0f, 7.0f);
		ecs.insert<Velocity>(Components::Velocity, 1.0f, 3.0f, 8.0f);
		ecs.insert<Velocity>(Components::Velocity, 1.0f, 4.0f, 0.0f);


		ecs.insert<Name>(Components::Name, "P1");
		ecs.insert<Name>(Components::Name, "P2");
		ecs.insert<Name>(Components::Name, "P3");
		ecs.insert<Name>(Components::Name, "P4");

		ecs.insert<Health>(Components::Health, 1.0f);
		ecs.insert<Health>(Components::Health, 0.5f);
		ecs.insert<Health>(Components::Health, 0.2f);
		ecs.insert<Health>(Components::Health, 0.0f);

		// entity X


		_ = 0;


		// components[std::to_underlying(Components::Name)].push_back(std::make_unique<Name>("Player1"));
		// components[std::to_underlying(Components::Name)].push_back(std::make_unique<Name>("Player2"));
		//
		//
		// Name* p1name = reinterpret_cast<Name*>(components[std::to_underlying(Components::Name)][0].get());
		//
		//
		// auto p1copy = p1name->clone();
		//
		// p1name->name.assign("New Player 2");
		// Name p2copy = p1name->copy();
		//
		// p2copy.name = "extend";
		//
		//
		// for (const auto& name : components[std::to_underlying(Components::Name)])
		//{
		//
		//	Name& ptr = *reinterpret_cast<Name*>(name.get());
		//	dbg::println("player {}", ptr.name);
		// }
		//
		// for (const auto& name : components[std::to_underlying(Components::Velocity)])
		//{
		//
		//	Velocity& ptr = *reinterpret_cast<Velocity*>(name.get());
		//
		//	dbg::println("velocity {} {} {}", ptr.x, ptr.y, ptr.z);
		// }
	}

	_ = 0;

	// ########################################################################

	std::array<u8, 1024> buffer{};

	// auto rpass = random::password(32);
	// std::ranges::copy(rpass, buffer.begin());
	std::string npass;
	npass.resize(32);
	std::string_view spass{npass};

	random::digit(spass, 32);

	auto ret = file::v2::write("test_file.txt", spass, true);

	buffer.fill(0);

	std::string smallbuf;
	smallbuf.resize(16);
	std::string_view smallbufv{smallbuf};

	ret = file::v2::read("test_file.txt", smallbuf, 16);

	dbg::println("before thread");

	std::atomic<u64> log_index{0};
	std::vector<u8>  log_buffer(3_MiB);
	std::ranges::fill(log_buffer, '\n');

	constexpr u32 message_count = 10000;


	atomic_ringbuffer<u32, 8> arb;

	for (int i = 0; i < 16; ++i)
		arb.try_push(i);


	auto arb_pop = arb.try_pop();
	arb_pop      = arb.try_pop();


	_ = 0;

	auto pthread = std::thread(
	  [&]()
	  {
		  std::string msg(128, '\0');
		  dbg::println("thread 1 start");

		  for (int i = 0; i < message_count; ++i)
		  {
			  auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

			  msg = std::format("[{:%F %T}] {:<32} ; {}", time, random::alphanum(8), std::this_thread::get_id());

			  auto old_index = log_index.fetch_add(msg.size() + 1);
			  std::ranges::copy(msg, log_buffer.begin() + old_index);

			  std::this_thread::yield();
		  }
		  dbg::println("thread 1 end");
	  });

	auto pthread2 = std::thread(
	  [&]()
	  {
		  dbg::println("thread 2 start");
		  std::string msg(128, '\0');

		  for (int i = 0; i < message_count; ++i)
		  {
			  auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

			  msg = std::format("[{:%F %T}] {:<32} ; {}", time, random::digit(16), std::this_thread::get_id());

			  auto old_index = log_index.fetch_add(msg.size() + 1);
			  std::ranges::copy(msg, log_buffer.begin() + old_index);

			  std::this_thread::yield();
		  }
		  dbg::println("thread 2 end");
	  });

	auto pthread3 = std::jthread(
	  [&]()
	  {
		  std::string msg(128, '\0');

		  for (int i = 0; i < message_count; ++i)
		  {
			  auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

			  msg = std::format("[{:%F %T}] {:<32} ; {}", time, random::alpha(24), std::this_thread::get_id());


			  auto old_index = log_index.fetch_add(msg.size() + 1);


			  std::ranges::copy(msg, log_buffer.begin() + old_index);
			  std::this_thread::yield();
		  }
	  });

	pthread.join();
	pthread2.join();
	pthread3.join();
	dbg::println("log_indeX: {}", log_index.load() - 1);
	ret = file::v2::write("log.txt", log_buffer, log_index.load() - 1, true);

	_ = 0;


	// ########################################################################

	// ########################################################################


#if 0
Tree<char> tree[]{{'D', tree + 1, tree + 2}, {'B', tree + 3, tree + 4}, {'F', tree + 5, tree + 6}, {'A'}, {'C'}, {'E'}, {'G'}};

for (char x : tree->traverse_inorder())
dbg::print("{} ", x);
dbg::println();
#endif
	// ########################################################################
	if constexpr (false)
	{
		constexpr size_t mof_size  = sizeof(std::move_only_function<void()>);
		constexpr size_t stdf_size = sizeof(std::function<void()>);
		constexpr size_t fr_size   = sizeof(function_ref<void()>);
		dbg::println("move_only_function size: {} / std::function size: {}, function_ref size: {}", mof_size, stdf_size, fr_size);

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
		auto       tpool_start = clock_now();
		threadpool tpool;

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

	// ########################################################################


	// ########################################################################

	struct argument_def
	{
		std::string short_name;
		std::string long_name;
		bool        is_flag{false};
		bool        requires_value{false};
		int         int_value{0};
		std::string str_value{}; // utf8
		f64         float_value{0.0};
	};

	std::unordered_map<std::string, bool> flags; // key is normalized from long or short name
												 // verbose or v

	flags["--verbose"] = true;
	flags["-v"]        = true;

	auto argparser = [&](std::string_view) { };

	// add<int>("count", 'c', "Number of items", 42);				// long, short, description, default
	// add<bool>("verbose", 'v', "Enable verbose output", false);
	// add<std::string>("out", 'o', "Output file", "out.txt");

	_;

	// ########################################################################


	std::string cmdl = "deckard --version --verbose -O2 --path=./test.txt -d";

	auto opts = parse_command_line(cmdl);
	dbg::println("Flags:");
	for (const auto& f : opts.flags)
		dbg::println("  {}", f);
	dbg::println("Options:");
	for (const auto& [k, v] : opts.options)
		dbg::println("  {} = {}", k, v);
	dbg::println("Positional:");
	for (const auto& p : opts.positional)
		dbg::println("  {}", p);


	_ = 0;

	// ###############################################


	clitest ct;

	bool flag1{false};
	bool flag2{true};

	ct.add_flag("flag1", flag1);
	ct.add_flag("flag2", flag2);

	ct.dump();

	_ = 0;


	// ###############################################


	// ###############################################


	//
	// std::string ipv6("2001:0db8:85a3::8a2e:0370:7334");

	//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	// 20 01 0d b8 00 00 00 00 00 01 00 00 00 00 00 01
	// 192.168.1.1 - c0.ab.01.01
	//  ::ffff:c0ab:0101
	//
	//  127.0.0.1 - ::ffff:7f00:1


	// ###################

	dbg::println("{}", string::match("*X*", "qH1") ? "match!" : "not found");

	// ###################


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
	//	  create_assign_node("x", create_bin_op_node('+', create_num_node(2), create_bin_op_node('*', create_num_node(3),
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

	vulkanapp app01(
	  {.title  = "Example 01", //
	   .width  = 1280,
	   .height = 720,
	   .flags  = Attribute::vsync | Attribute::resizable});


	// app01.set_keyboard_callback(keyboard_callback);
	app01.set_fixed_update_callback(fixed_update);
	app01.set_update_callback(update);
	app01.set_render_callback(render);


	return app01.run();
}
