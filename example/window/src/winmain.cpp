﻿#include <windows.h>
#include <Commctrl.h>


import std;
import deckard;
import deckard.types;
import deckard.timers;
using namespace deckard;
using namespace deckard::app;
using namespace deckard::random;
using namespace std::string_view_literals;


#ifndef _DEBUG
import window;
#endif

std::array<unsigned char, 256> previous{0};
std::array<unsigned char, 256> current{0};

void keyboard_callback(vulkanapp& app, i32 key, i32 scancode, Action action, i32 mods)
{
	dbg::println("key: {:#x} - {:#x}, {} - {}", key, scancode, action == Action::Up ? "UP" : "DOWN", mods);

	bool up = action == Action::Up;

	// num 6 -> 0x66, 0x4d
	// f     -> 0x46, 0x21

	if (up and key == Key::B)
	{
		current.swap(previous);
		{
			ScopeTimer<std::micro> _("state");
			GetKeyboardState(&current[0]);
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
		app.set(Attribute::vsync);
	}

	if (key == Key::F2 and up)
	{
		app.resize(1280, 720);
	}

	if (up and (key == Key::F11 or key == Key::F))
	{
		app.set(Attribute::togglefullscreen);
	}

	if (key == Key::Numpad1 and up)
	{
		app.set(Attribute::gameticks, 60u);
		dbg::println("ticks 60");
	}
	if (key == Key::Numpad2 and up)
	{
		app.set(Attribute::gameticks, 30u);
		dbg::println("ticks 30");
	}

	if (key == Key::Numpad3 and up)
	{
		app.set(Attribute::gameticks, 5u);
		dbg::println("ticks 5");
	}

	if (key == Key::Add and up)
	{
		u32 newticks = app.get(Attribute::gameticks) * 2;
		app.set(Attribute::gameticks, newticks);
		dbg::println("ticks {}", newticks);
	}

	if (key == Key::Subtract and up)
	{
		u32 newticks = app.get(Attribute::gameticks) / 2;
		app.set(Attribute::gameticks, newticks == 0 ? 1 : newticks);
		dbg::println("ticks {}", newticks);
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
#error("use std::sin")
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

i32 deckard_main(utf8::view /*commandline*/)
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", window::build::version_string, window::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif


#if 0
Tree<char> tree[]{{'D', tree + 1, tree + 2}, {'B', tree + 3, tree + 4}, {'F', tree + 5, tree + 6}, {'A'}, {'C'}, {'E'}, {'G'}};

for (char x : tree->traverse_inorder())
dbg::print("{} ", x);
dbg::println();
#endif
	// ###############################################



	// ###############################################


	auto is_valid_ipv6 = [](const std::string_view input) -> bool
	{
		if (input.empty() or input.size() > 39)
			return false;


		u8   groups           = 0;
		u8   zeros            = 0;
		bool has_double_colon = false;

		auto validate_group = [](std::string_view group) -> bool
		{
			if (group.empty() or group.size() > 4)
				return false;

			return std::ranges::all_of(group, utf8::is_ascii_hex_digit);
		};

		std::string_view remaining = input;
		while (not remaining.empty())
		{
			auto colon = remaining.find(':');
			if (colon == 0)
			{
				if (has_double_colon)
					return false;
				if (remaining.size() < 2 or remaining[1] != ':')
					return false;
				has_double_colon = true;
				zeros            = 8 - groups;
				remaining.remove_prefix(2);
				continue;
			}

			auto group = remaining.substr(0, colon);
			if (not validate_group(group))
				return false;

			groups++;
			if (groups > 8)
				return false;

			if (colon == std::string_view::npos)
				break;

			remaining.remove_prefix(colon + 1);
		}

		// auto g = groups + (has_double_colon ? zeros : 0);
		return 8 == (groups + (has_double_colon ? zeros : 0));
	};


	auto ipv6_from_string = [&](const std::string_view input) -> std::array<u8, 16>
	{
		static constexpr u8 MAX_IPV6_ADDRESS_STR_LEN = 39;

		// https://www.rfc-editor.org/rfc/rfc4291
		//
		// TODO: check for .(dot) from end
		//
		//  0:0:0:0:0:0:13.1.68.3
		//  0:0:0:0:0:FFFF:129.144.52.38
		//
		//	or in compressed form:
		//
		//  ::13.1.68.3
		//  ::FFFF:129.144.52.38

		std::array<u8, 16> ret{};

		u16 accumulator = 0;
		u8  colon_count = 0;
		u8  pos         = 0;

		for (u8 i = 1; i < input.size(); i++)
		{
			if (input[i] == ':')
			{
				if (input[i - 1] == ':')
					colon_count = 14;
				else if (colon_count)
					colon_count -= 2;
			}
		}

		for (u8 i = 0; i < input.size() && pos < 16; i++)
		{

			if (input[i] == ':')
			{
				ret[pos + 0] = as<u8>(accumulator >> 8);
				ret[pos + 1] = as<u8>(accumulator);
				accumulator  = 0;

				if (colon_count && i && input[i - 1] == ':')
					pos = colon_count;
				else
					pos += 2;
			}
			else
			{
				accumulator <<= 4;
				accumulator |= utf8::ascii_hex_to_int(input[i]);
			}
		}

		ret[pos + 0] = as<u8>(accumulator >> 8);
		ret[pos + 1] = as<u8>(accumulator);
		return ret;
	};
	/*
	The conversion algorithm is as follows:

	Examine the first 12 bytes of the 16-byte representation
		If they match the IPv4‑mapped prefix (10 bytes of 0x00 followed by 0xFF, 0xFF``):
		Interpret the last 4 bytes as an IPv4 address in dotted‑decimal notation
	Otherwise:
		Interpret the 16 bytes as an IPv6 address in colon‑hexadecimal notation
	*/

	auto ipv6_to_string = [](const std::array<u8, 16>& buffer) -> std::string
	{
		std::array<u16, 8> groups;
		for (int i = 0; i < 8; ++i)
			groups[i] = as<u16>((buffer[2 * i] << 8) | buffer[2 * i + 1]);


		int best_start = -1, best_len = 0;
		for (int i = 0; i < groups.size();)
		{
			if (groups[i] == 0)
			{
				int j = i;
				while (j < groups.size() && groups[j] == 0)
					++j;
				int len = j - i;
				if (len > best_len)
				{
					best_start = i;
					best_len   = len;
				}
				i = j;
			}
			else
			{
				++i;
			}
		}

		if (best_len < 2)
			best_start = -1;

		std::string ret;
		for (int i = 0; i < 8;)
		{
			if (i == best_start)
			{
				ret += (i == 0) ? ":" : ":";
				i += best_len;

				if (i >= 8)
					break;
			}
			else
			{
				if (i > 0)
					ret += ":";
				ret += std::format("{:x}", groups[i]);

				++i;
			}
		}
		return ret;
	};
	// IPv6 Address:    2001:0db8:85a3:0000:0000:8a2e:0370:7334
	//	16 - Byte Representation: [20 01 0d b8 85 a3 00 00 00 00 8a 2e 03 70 73 34]

	auto ipvs6 = ipv6_from_string("2001:0db8:85a3::8a2e:0370:7334");

	// IPv4 Address : 192.0.2.1 16 - Byte Representation
	//	: [00 00 00 00 00 00 00 00 00 00 FF FF C0 00 02 01]
	auto ipvs4 = ipv6_from_string("::ffff:192.0.2.1");

	auto r = R"(
	 
	)"sv;


	std::string abc("abcdefg");

	auto it_abc = abc.begin() + 0;
	dbg::println("a? auto: {}", *it_abc);

	it_abc += 1;

	dbg::println("b? auto: {}", *it_abc);


	it_abc += 4;

	dbg::println("f? auto: {}", *it_abc);


	filemap f("input.bin", filemap::access::readwrite);


	auto slice = f[0, 256];

	slice[0] = 'D';
	slice[1] = 'E';
	slice[2] = 'A';
	slice[3] = 'D';

	f.close();


	// std::string ipv6("2001:db8::1:0:0:1");
	std::string ipv6("::ffff:7f00:1");

	std::string nipv6 = ipv6.substr(2, 24);
	nipv6.reserve(64);
	nipv6.shrink_to_fit();

	for (int i = 0; i < nipv6.size(); i++)
	{
		dbg::println("{:#x}", nipv6[i]);
	}


	//
	// std::string ipv6("2001:0db8:85a3::8a2e:0370:7334");

	//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	// 20 01 0d b8 00 00 00 00 00 01 00 00 00 00 00 01
	// 192.168.1.1 - c0.ab.01.01
	//  ::ffff:c0ab:0101
	//
	//  127.0.0.1 - ::ffff:7f00:1


	dbg::println("ipv6: {}", ipv6);

	auto ipv_addr = ipv6_from_string(ipv6);
	dbg::println("bytes: {}", ipv_addr);

	auto ipv_recon = ipv6_to_string(ipv_addr);
	dbg::println("recon: {}", ipv_recon);


	dbg::println("log2(32) = {}", std::log2(32));
	dbg::println("log2(64) = {}", std::log2(64));
	dbg::println("log2(85) = {}", std::log2(85));
	dbg::println("log2(92) = {}", std::log2(92));


	std::vector<u8> kos = make_vector<u8>(0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07);
	std::span<u8>   vkos{kos};

	dbg::println("{}", vkos.size());
	dbg::println("{}", vkos);


	kos.clear();

	dbg::println("{}", vkos.size());
	dbg::println("{}", vkos);


	// ###################

	dbg::println("{}", string::match("*X*", "qH1") ? "match!" : "not found");

	// ###################

	for (const auto& coord : Iter2D(4, 5))
		dbg::print("({}, {}) ", coord.x, coord.y);

	dbg::println();

	{
		constexpr std::string_view unicode[]{"▀▄─", "▄▀─", "▀─▄", "▄─▀"};

		for (int y{}, p{}; y != 6; ++y, p = ((p + 1) % 4))
		{
			for (int x{}; x != 16; ++x)
				dbg::print("{}", unicode[p]);
			dbg::println();
		}
	}


	// ########################################################################
	// ✅ ❌

	auto rle = compress_rle<std::string>("WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW");

	for (const auto& i : rle)
	{
		dbg::println("{} - {}", i.first, i.second);
	}


	// Bailey–Borwein–Plouffe
	f64 pi{0};
	for (auto k : range(0, 15))
	{
		f64 term =
		  (1.0 / std::pow(16, k)) *  //
		  ((4.0 / (8.0 * k + 1.0)) - //
		   (2.0 / (8.0 * k + 4.0)) - //
		   (1.0 / (8.0 * k + 5.0)) - //
		   (1.0 / (8.0 * k + 6.0)));

		pi += term;
	}


	// ###########################################################################

	// Distribution for [0, 1) for radius scaling
	std::uniform_real_distribution<f32> uniform_zero_to_one(0.0f, 1.0f);
	// Distribution for [0, 2*PI) for angle
	std::uniform_real_distribution<f32> uniform_angle(0.0f, 2.0f * as<f32>(std::numbers::pi));

	auto getRandomPointInCircle = [&](f32 radius, f32 centerX = 0.0f, f32 centerY = 0.0f) -> std::pair<f32, f32>
	{
		std::random_device rd;
		// Generate random angle theta E [0, 2*PI)
		f32 theta = uniform_angle(rd);

		// Generate random u E [0, 1) for radius scaling
		// The radius r is proportional to sqrt(u) to ensure uniform distribution.
		f32 u     = uniform_zero_to_one(rd);
		f32 r_val = radius * std::sqrt(u);

		// Convert polar to Cartesian coordinates
		return {centerX + r_val * std::cos(theta), centerY + r_val * std::sin(theta)};
	};

	// for(int i=0; i < 8000; ++i)
	//{
	//	auto [x, y] = getRandomPointInCircle(5.0f);
	//	dbg::println("{:.3f}, {:.3f}", x, y);
	// }


	i32 ab1 = -125;

	dbg::println("{}", std::in_range<u16>(ab1));
	dbg::println("{}", std::in_range<i16>(ab1));

	// ########################################################################

	// ########################################################################

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


	app01.set_keyboard_callback(keyboard_callback);
	app01.set_fixed_update_callback(fixed_update);
	app01.set_update_callback(update);
	app01.set_render_callback(render);


	return app01.run();
}
