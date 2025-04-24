#include <windows.h>
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
	// dbg::println("key: {:#x} - {:#x}, {} - {}", key, scancode, action == Action::Up ? "UP" : "DOWN", mods);

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

		auto shift = current[Key::Shift];

		int k = 0;
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

	if (up and (key == Key::F11 or key == Key::F))
	{
		app.set(Attribute::togglefullscreen);
	}

	if (key == Key::Numpad1 and up)
	{
		app.set(Attribute::gameticks, 60u);
	}
	if (key == Key::Numpad2 and up)
	{
		app.set(Attribute::gameticks, 30u);
	}

	if (key == Key::Numpad3 and up)
	{
		app.set(Attribute::gameticks, 1u);
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

i32 deckard_main(utf8::view commandline)
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

	auto is_valid_ipv6 = [](const std::string_view input) -> bool
	{
		bool ret = false;

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

		auto g = groups + (has_double_colon ? zeros : 0);
		return 8 == (groups + (has_double_colon ? zeros : 0));
	};


	auto ipv6_from_string = [&](const std::string_view input) -> std::array<u8, 16>
	{
		constexpr u8 MAX_IPV6_ADDRESS_STR_LEN = 39;

		bool k = is_valid_ipv6(input);

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
				ret[pos + 0] = accumulator >> 8;
				ret[pos + 1] = accumulator;
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

		ret[pos + 0] = accumulator >> 8;
		ret[pos + 1] = accumulator;
		return ret;
	};

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

	double d = 1.4e3; // 1.4 * (10^3 aka 1000)
					  // 0x4.1p6, 4 * 2^6, 64, 256
	double d1 = 0x1.01p1;

	std::string_view x("0x1.4p3");
	f64              result{};
	auto             fcres = std::from_chars(x.data(), x.data() + x.size(), result, std::chars_format::hex);


	// std::string ipv6("2001:db8::1:0:0:1");
	std::string ipv6("::ffff:7f00:1");

	std::string nipv6 = ipv6.substr(2, 24);
	nipv6.reserve(64);
	nipv6.shrink_to_fit();

	for (int i = 0; i < nipv6.size(); i++)
	{
		dbg::println("{:#x}", nipv6[i]);
	}

	auto ds = ipv6.size();
	ipv6.append(ipv6);
	ipv6.append(ipv6);
	ipv6.append(ipv6);
	ipv6.append(ipv6);
	ipv6.append(ipv6);
	ipv6.append(ipv6);
	ds = ipv6.size();


	int jqq = 0;

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

	int j1 = 0;


	std::vector<u8> kos = make_vector<u8>(0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07);
	std::span<u8>   vkos{kos};

	dbg::println("{}", vkos.size());
	dbg::println("{}", vkos);


	kos.clear();

	dbg::println("{}", vkos.size());
	dbg::println("{}", vkos);


	{
		// ####

		std::string      a("--number 10 -n 'hello world'"); // --name 'hello'
		std::string_view sv(a);


		if (sv.starts_with("--"))
			sv.substr(2);
		else if (sv.starts_with("-"))
			sv.substr(1);


		// ####
	}


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


	deckard::db::db db("database.db");
	auto            dkod = db.exec("PRAGMA he");

	db.prepare("CREATE TABLE IF NOT EXISTS blobs(id INTEGER PRIMARY KEY, log_id INTERGER, text TEXT, data BLOB);").commit();
	db.prepare("CREATE TABLE IF NOT EXISTS fs(path TEXT NOT NULL, size INTEGER NOT NULL, hash TEXT NOT NULL, data BLOB NOT NULL)").commit();


	db.prepare("SELECT id,data FROM blobs WHERE id = 1").commit();
	auto aid   = db.at("id");
	auto adata = db.at<std::vector<u8>>("data");


	if (aid)
		dbg::println("id: {}", *aid);

	if (adata)
		dbg::println("data: {}", *adata);


	db.prepare("SELECT COUNT(*) AS count FROM fs;").commit();


	ScopeTimer<std::milli> t("transaction");
	db.begin_transaction();
	if (auto count = db.at("count"); count)
	{
		t.start();

		dbg::println("delete all data from fs");
		db.prepare("DELETE FROM fs WHERE size > 0").commit();
		db.end_transaction();

		t.now("deletes");
		t.reset();
	}


	constexpr u32 bulk_insert_count = 10000;

	db.prepare("SELECT COUNT(*) AS count FROM fs;").commit();

	db.begin_transaction();

	t.start();
	if (auto count = db.at("count"); count)
	{
		db.prepare("INSERT INTO fs (path, size, hash, data) VALUES (?1, ?2, ?3, ?4)");

		for (int k : upto(bulk_insert_count))
		{
			db.bind(std::format("data/level{:03}/sprite_{:4}_{:02d}.qoi", random::randu32(0, 999), random::alpha(4), k),
					random::randu32(1, 16 * 2048),
					"ABCD",
					"DATA");
			db.commit();
		}
	}
	t.now("bulk insert");
	db.end_transaction();
	t.now("commit");
	t.stop();

	db.close();


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

	app01.set_title(std::format("{}", sizeof(vulkanapp)));

	app01.set_keyboard_callback(keyboard_callback);
	app01.set_fixed_update_callback(fixed_update);
	app01.set_update_callback(update);
	app01.set_render_callback(render);


	return app01.run();
}
