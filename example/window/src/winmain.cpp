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

i32 deckard_main([[maybe_unused]] utf8::view commandline)
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", window::build::version_string, window::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif

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

	///
	{
		constexpr size_t   NTP_PACKET_SIZE = 48;
		constexpr uint32_t NTP_DELTA       = 2'208'988'800U; // seconds between 1900‑01‑01 and 1970‑01‑01
		constexpr double   TWO_POW_32      = 4294967296.0;   // 2^32, used for fractional conversion

		//const char* hostname = "pool.ntp.org";
		const char* hostname = "time.cloudflare.com";
		const char* service  = "123";                        // NTP uses UDP port 123

															 // ----------------------- Resolve host ---------------------------------
		addrinfo hints{}, *result = nullptr;
		hints.ai_family   = AF_UNSPEC;  // IPv4 or IPv6
		hints.ai_socktype = SOCK_DGRAM; // UDP
		hints.ai_protocol = IPPROTO_UDP;

		int rc = getaddrinfo(hostname, service, &hints, &result);
		if (rc != 0)
		{
			dbg::println("getaddrinfo: {}", gai_strerrorA(rc));
		}

		// ----------------------- Create socket ---------------------------------
		SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			dbg::println("socket() failed: {}", WSAGetLastError());
			freeaddrinfo(result);
		}

		// ----------------------- Build NTP request packet -----------------------
		std::vector<uint8_t> packet(NTP_PACKET_SIZE, 0);

		// clang-format off

		//                 2   3   3   
		//                LI  VN   Mode
		//                 |   |   |   
		constexpr u8 ntp_config = 0b00'011'011;
		//  LI : 0 No warning
		// VN  : 011 v3, 100 v4
		// Mode: 011 (3) client 
		// 
		// clang-format on

		packet[0] = ntp_config;                           // LI = 0, VN = 3 (or 4), Mode = 3 (client)
													// ----------------------- Record t1 (client send time) ------------------
		auto t1 = std::chrono::steady_clock::now(); // high‑resolution monotonic clock

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

		// ----------------------- Set receive timeout (5 seconds) ---------------
		DWORD timeoutMs = 50000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

		// ----------------------- Receive reply ---------------------------------
		int recvLen = recvfrom(sock, reinterpret_cast<char*>(packet.data()), static_cast<int>(packet.size()), 0, nullptr, nullptr);

		// Record t4 (client receive time) as soon as possible
		auto t4 = std::chrono::steady_clock::now();

		if (recvLen == SOCKET_ERROR)
		{
			dbg::println("recvfrom() failed (timeout?): {}", WSAGetLastError());
			closesocket(sock);
			freeaddrinfo(result);
			WSACleanup();
		}

		if (recvLen < static_cast<int>(NTP_PACKET_SIZE))
		{
			dbg::println("Received packet too short ({}) bytes", recvLen);
			closesocket(sock);
			freeaddrinfo(result);
			WSACleanup();
		}

		// ---------------------------------------------------------------------------
		// Helper: turn a 64‑bit NTP timestamp (seconds + fraction) into a chrono duration
		// ---------------------------------------------------------------------------
		auto ntp_to_duration = [](u32 sec_net, u32 frac_net) -> std::chrono::duration<double>
		{
			u32    sec  = ntohl(sec_net);
			u32    frac = ntohl(frac_net);
			double d    = static_cast<double>(sec) + static_cast<double>(frac) / TWO_POW_32;
			return std::chrono::duration<double>(d);
		};

		auto ntp_to_dur = [](u32 nsec, u32 nfrac) -> std::chrono::system_clock::time_point
		{
			constexpr i64 NTP_TO_UNIX = 2'208'988'800LL; // seconds between 1900‑01‑01 and 1970‑01‑01
			u32           sec         = ntohl(nsec);
			u32           frac        = ntohl(nfrac);

			f64 secs = static_cast<f64>(sec) + static_cast<f64>(frac) / 4294967296.0; // 2^32


			// Shift to Unix epoch and cast to system_clock duration
			auto dur = std::chrono::duration<double>(secs - NTP_TO_UNIX);
			return std::chrono::system_clock::time_point{std::chrono::duration_cast<std::chrono::system_clock::duration>(dur)};
		};

		// ----------------------- Extract timestamps ----------------------------
		// Offsets inside the packet (all network‑byte‑order):
		//   Originate Timestamp  (bytes 24‑31) – time client sent request (t1)
		//   Receive   Timestamp  (bytes 32‑39) – time server received request (t2)
		//   Transmit  Timestamp  (bytes 40‑47) – time server sent reply   (t3)

		u32 orig_sec  = *reinterpret_cast<u32*>(packet.data() + 24);
		u32 orig_frac = *reinterpret_cast<u32*>(packet.data() + 28);
		u32 recv_sec  = *reinterpret_cast<u32*>(packet.data() + 32);
		u32 recv_frac = *reinterpret_cast<u32*>(packet.data() + 36);
		u32 xmit_sec  = *reinterpret_cast<u32*>(packet.data() + 40);
		u32 xmit_frac = *reinterpret_cast<u32*>(packet.data() + 44);

		// Convert server timestamps to chrono::duration<double> (seconds)

		using namespace std::chrono;
		duration<double> t2 = ntp_to_duration(recv_sec, recv_frac); // server receive
		duration<double> t3 = ntp_to_duration(xmit_sec, xmit_frac); // server transmit

		// Convert client steady_clock timestamps to double seconds
		duration<double> d_t1 = t1.time_since_epoch();
		duration<double> d_t4 = t4.time_since_epoch();

		// ---- Compute delay & offset ---------------------------------------
		f64 delay  = (d_t4 - d_t1).count() - (t3 - t2).count();
		f64 offset = ((t2 - d_t1).count() + (t3 - d_t4).count()) / 2.0;


		// ----------------------- Convert server transmit to Unix time ----------
		// NTP epoch (1900‑01‑01) → Unix epoch (1970‑01‑01)
		u64 unix_seconds = static_cast<u64>(ntohl(xmit_sec)) - NTP_DELTA;
		f64 fraction_sec = static_cast<f64>(ntohl(xmit_frac)) / TWO_POW_32;


		auto ntp_time = std::chrono::system_clock::time_point{
		  std::chrono::seconds(unix_seconds)
		  //
		  + duration_cast<milliseconds>(duration<f64>(fraction_sec))
		  //
		};


		//// If you need a time_point anchored to the system clock:
		std::chrono::system_clock::time_point tp = ntp_time;

		auto format_utc = [](std::chrono::system_clock::time_point tp) -> std::string
		{
			std::time_t tt = std::chrono::system_clock::to_time_t(tp);
			std::tm     utc_tm{};
			gmtime_s(&utc_tm, &tt);
			std::ostringstream oss;
			oss << std::put_time(&utc_tm, "%Y-%m-%d %H:%M:%S UTC");
			return oss.str();
		};


		// ----------------------- Output ----------------------------------------
		dbg::println("Server: {}", hostname);
		dbg::println("Current UTC time (server transmit): {}", format_utc(ntp_time));
		dbg::println("Integer seconds: {}, Fractional part: {}", unix_seconds, fraction_sec);
		dbg::println("Round-trip statistics:\n");
		dbg::println("Client send (t1)   : {}s (steady_clock))", d_t1.count());
		dbg::println("Server receive (t2): {}s (NTP)", t2.count());
		dbg::println("Server transmit(t3): {}s (NTP)", t3.count());
		dbg::println("Client recv (t4)   : {}s (steady_clock)\n", d_t4.count());
		dbg::println("Delay     = {}s", delay);
		dbg::println("Offset    = {}s", offset);
		dbg::println("Offset -  = {}s", offset - NTP_DELTA);

		dbg::println("Integer seconds: {}, Fractional part: {}", unix_seconds, fraction_sec);


		// Round to the nearest minute – most zones are minute‑aligned
		int  total_min = static_cast<int>(std::round(offset / 60.0));
		int  hh        = total_min / 60;
		int  mm        = std::abs(total_min % 60);
		char sign      = (hh < 0 || (hh == 0 && offset < 0)) ? '-' : '+';
		dbg::println("UTC offset: {}{:02}:{:02}", sign, std::abs(hh), mm);

		// 1. Estimate the true UTC moment of t₁ using the offset we just computed:
		// std::chrono::system_clock::time_point approx_t1_utc = std::chrono::system_clock::now() - std::chrono::duration<double>{offset};

		// 2. Derive t₄_utc by adding the measured steady‑clock interval:
		// auto                                  client_interval = d_t4 - d_t1; // steady_clock duration
		//		std::chrono::system_clock::time_point approx_t4_utc   = approx_t1_utc + client_interval;
		// dbg::println("Approx t1: {}", approx_t1_utc);
		// dbg::println("Approx t4: {}", approx_t4_utc);

		auto zone_for_offset = [](seconds offset, system_clock::time_point tp = system_clock::now()) -> std::string
		{
			for (const auto& z : get_tzdb().zones)
			{
				// `z.get_info(tp)` gives the rule that applies at `tp`
				auto info = z.get_info(tp);
				if (info.offset == offset)
				{
					return std::string(z.name()); // e.g. "Europe/Berlin"
				}
			}
			return {};                            // no match found
		};

		dbg::println("Zone: {}", zone_for_offset(duration_cast<seconds>((duration<f64>(offset)))));

		std::string tz_name = "Europe/Helsinki";

		// Offset -  = 1757562486.6314878s
		//             1758139549

		// + milliseconds { fraction_sec }};


		zoned_time zt(tz_name, tp);
		dbg::println("In zone: {:%F %T %Z}", zt);

		// 1758137824
		// Integer seconds : 1758137820, Fractional part : 0.8626013642642647

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
