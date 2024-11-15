#include <windows.h>
#include <Commctrl.h>
#include <intrin.h>
#include <random> // workaround for module std

import deckard;
using namespace deckard;
using namespace deckard::app;
using namespace deckard::random;
import std;
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

namespace fs = std::filesystem;

class commandliner
{
private:
	std::string_view commandline;

public:
	commandliner(std::string_view i)
		: commandline(i)
	{
	}

	void process()
	{
		//

		int x = 0;
	}
};

template<size_t SSO_SIZE = 32>
class alignas(8) u8string_sso
{
	static constexpr u32 MAX_CAPACITY = (1 < 31) - 1;

	union CapacitySizeUnion
	{
		u64 value{0};

		struct
		{
			u32 size : 32;
			u32 capacity : 31;
			u32 sso : 1; // tag for heap allocation
		} bits;
	};


private:
	CapacitySizeUnion m_cap_size;

	union
	{
		u8  stackbuf[SSO_SIZE - sizeof(CapacitySizeUnion)]{0};
		u8* ptr;
	};

	void set_sso(bool sso) { m_cap_size.bits.sso = true; }

	bool is_sso() const { return static_cast<bool>(m_cap_size.bits.sso); }

	void set_capacity(u32 newcap)
	{
		assert::check(newcap <= MAX_CAPACITY, "String has maximum capacity of 2^31 bits");
		m_cap_size.bits.capacity = std::min(newcap, MAX_CAPACITY);
	}

	void set_size(u32 newsize) { m_cap_size.bits.size = newsize; }

public:
	u32 capacity() const { return static_cast<u32>(m_cap_size.bits.capacity); }

	u32 size() const { return static_cast<u32>(m_cap_size.bits.size); }
};

static_assert(sizeof(u8string_sso<>) == 32, "u8string_sso should be 32-bytes");

union Data
{
	struct alignas(8) NonSSO
	{
		u8* ptr;
		u32 size;
		u32 capacity;
	} non_sso;

	struct alignas(8) SSO
	{
		u8 string[16 + sizeof(NonSSO) / sizeof(u8) - 1];
		u8 size; // turns to null byte when 0
	} sso;
};

static_assert(sizeof(Data) == 32);

struct codepoint_result
{
	u32      bytes_read{0};
	char32_t codepoint{0xFFFD};
};

// return: number of bytes read
codepoint_result decode_codepoint(const std::span<u8> buffer, u32 index)
{
	u32      bytes_read{0};
	char32_t codepoint{0xFFFD};

	dbg::println("{}", sizeof(codepoint));


	return {bytes_read, codepoint};
}

template<typename type = u8, size_t SSO_CAPACITY = 32, typename Allocator = std::allocator<type>>
union basic_smallbuffer
{
	using pointer   = type*;
	using size_type = u32;

	struct SSO
	{
		type str[SSO_CAPACITY - 1]{0}; // data() bytes
		type len{sizeof(str)};         // (1, 31) when Small
	} sso{};

	struct NonSSO
	{
		size_type size;
		size_type capacity;

		union
		{
			pointer ptr;

			struct
			{
				type padding[sizeof(SSO) - sizeof(pointer) - 1]; // never accessed via this reference
				type len;                                        // 0x00 when Big
			} ptrbytes;
		} buffer;
	} nonsso;

	bool is_sso() const { return sso.len > 0; }

	void set_sso_len(type newlen) { sso.len = std::min<type>(newlen, sizeof(sso.str)); }

	pointer data() { return is_sso() ? sso.str : nonsso.buffer.ptr; }

	size_type size() const { return is_sso() ? sso.len : nonsso.size; }

	size_type capacity() const { return is_sso() ? sizeof(sso.str) : nonsso.capacity; }

	std::string_view data() const
	{
		if (is_sso())
			return {sso.str, sizeof(sso.str) - sso.str};

		todo();
		return {};
	}

	void set(std::string_view input) { }
};

using SmallStringBuffer = basic_smallbuffer<u8, 32>;

static_assert(sizeof(SmallStringBuffer) == 32, "SmallStringBuffer should be 32-bytes");

class u8str
{
private:
	SmallStringBuffer sbo{};

public:
};

#define CTX_PI 3.141592653589793f

constexpr f32 ctx_sinf(f32 x) noexcept
{
	/* source : http://mooooo.ooo/chebyshev-sine-approximation/ */
	while (x < -CTX_PI)
		x += CTX_PI * 2;
	while (x > CTX_PI)
		x -= CTX_PI * 2;

	f32 coeffs[] = {
	  -0.10132118f,        // x
	  0.0066208798f,       // x^3
	  -0.00017350505f,     // x^5
	  0.0000025222919f,    // x^7
	  -0.000000023317787f, // x^9
	  0.00000000013291342f // x^11
	};

	f32 x2  = x * x;
	f32 p11 = coeffs[5];
	f32 p9  = p11 * x2 + coeffs[4];
	f32 p7  = p9 * x2 + coeffs[3];
	f32 p5  = p7 * x2 + coeffs[2];
	f32 p3  = p5 * x2 + coeffs[1];
	f32 p1  = p3 * x2 + coeffs[0];
	return (x - CTX_PI + 0.00000008742278f) * (x + CTX_PI - 0.00000008742278f) * p1 * x;
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

// static_assert(k_md5[0] == 0xd76a'a478);
// static_assert(k_md5[63] == 0xeb86'd391);
uint64_t multiply_64bit_by_8bit_chunks(uint64_t a, uint8_t b)
{
	uint64_t result = 0;
	uint8_t  carry  = 0;

	for (int i = 0; i < 8; ++i)
	{
		uint8_t  a_byte  = (a >> (i * 8)) & 0xFF;
		uint16_t product = a_byte * b + carry;
		result |= (uint64_t)product << (i * 8);
		carry = product >> 8;
	}

	return result;
}

std::vector<int> multiply_large_numbers(const std::vector<int>& num1, const std::vector<int>& num2)
{
	int              n1 = as<int>(num1.size()), n2 = as<int>(num2.size());
	std::vector<int> result(n1 + n2, 0);

	// Multiply each digit of num2 with each digit of num1
	for (int i = n2 - 1; i >= 0; i--)
	{
		int carry = 0;
		for (int j = n1 - 1; j >= 0; j--)
		{
			int product       = num1[j] * num2[i] + carry + result[i + j + 1];
			carry             = product / 10;
			result[i + j + 1] = product % 10;
		}
		result[i] = carry;
	}

	// Remove leading zeros
	int i = 0;
	while (i < result.size() - 1 && result[i] == 0)
	{
		i++;
	}

	result.erase(result.begin(), result.begin() + i);
	return result;
}

extern "C" struct sqlite3_value;
extern "C" struct sqlite3_context;
extern "C" f64  sqlite3_value_double(sqlite3_value*);
extern "C" void sqlite3_result_double(sqlite3_context*, f64);

template<typename T = u8, typename Allocator = std::allocator<T>>
class TestAllocator
{
private:
	T*  ptr{};
	u32 size{0};

public:
	TestAllocator() = default;

	~TestAllocator() { destroy(); }

	template<typename Alloc = Allocator>
	void allocate(int s, Alloc&& alloc = Allocator{})
	{
		size = s;
		dbg::println("{} allocate", size);

		ptr = alloc.allocate(size);
		std::uninitialized_fill(ptr, ptr + size, (u8)'Q');

		dbg::println("{}", ptr[0]);
	}

	template<typename Alloc = Allocator>
	void destroy(Alloc&& alloc = Allocator{})
	{
		dbg::println("destroy {}", size);
		std::destroy_n(ptr, size);
		alloc.deallocate(ptr, size);

		size = 0;
		ptr  = nullptr;
	}

	T* data() const { return ptr; }
};

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

void test_cb01() { dbg::println("cb test 01"); }

void test_cb02() { dbg::println("cb test 02"); }

int deckard_main()
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", window::build::version_string, window::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif

	Tree<char> tree[]{{'D', tree + 1, tree + 2}, {'B', tree + 3, tree + 4}, {'F', tree + 5, tree + 6}, {'A'}, {'C'}, {'E'}, {'G'}};

	for (char x : tree->traverse_inorder())
		dbg::print("{} ", x);
	dbg::println();


	// ###################

	auto* cb_ptr = &test_cb01;
	char  buffer[sizeof(cb_ptr)]{};

	std::memcpy(buffer, &cb_ptr, sizeof(buffer));

	cb_ptr();

	cb_ptr = &test_cb02;
	std::memcpy(buffer, &cb_ptr, sizeof(buffer));

	cb_ptr();

	auto* ptr_from_buffer = std::bit_cast<void (*)()>(buffer);
	ptr_from_buffer();


	// ###########

	for (const auto& coord : Iter2D(4, 5))
		dbg::print("({}, {}) ", coord.x, coord.y);

	dbg::println();


	// ########################################################################

	auto rle = compress_rle<std::string>("WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW");

	for (const auto& i : rle)
	{
		dbg::println("{} - {}", i.first, i.second);
	}


	// ########################################################################
	u8* ptr{};
	{
		TestAllocator ta;

		ta.allocate(10);
		ptr = ta.data();
	}

	ptr[0] = 'A';

	std::string sss("\x41\xC3\x84\xE2\x86\xA5\xF0\x9F\x8C\x8D"sv);
	auto        i = utf8::length(sss);


	int j = 0;


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


	constexpr u32 bulk_insert_count = 10'000;

	db.prepare("SELECT COUNT(*) AS count FROM fs;").commit();

	db.begin_transaction();

	t.start();
	if (auto count = db.at("count"); count)
	{
		db.prepare("INSERT INTO fs (path, size, hash, data) VALUES (?1, ?2, ?3, ?4)");

		for (int i : upto(bulk_insert_count))
		{
			db.bind(std::format("data/level{:03}/sprite_{:4}_{:02d}.qoi", random::randu32(0, 999), random::alpha(4), i),
					random::randu32(1, 16 * 2'048),
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


	SmallStringBuffer sbo;
	sbo.set_sso_len(12);


	dbg::println("SmallBuffer {}", sizeof(u8str));
	std::string input = "2 + 3 * (4 - 5)";
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

	// 1 byte: A   0x41			0x41
	// 2 byte: Ä   0xC4			0xC3 0x84
	// 3 byte: ↥   0x21A8		0xE2 0x86 0xA8
	// 4 byte: 🌍  0x1F30D		0xF0 0x9F 0x8C 0x8D
	std::string u8str_d("\x41\xC3\x84\xE2\x86\xA8\xF0\x9F\x8C\x8D");

	u32 index = 0;

	auto [bytes, codepoint] = decode_codepoint({(u8*)u8str_d.data(), u8str_d.size()}, index);


	u8string_sso sso;
	dbg::println("sso1: {}", sizeof(sso));

	Data dd;
	dbg::println("sso2: {}", sizeof(dd));


	file f("input.ini");

	utf8::string u8str("\x41\xC3\x84\xE2\x86\xA5\xF0\x9F\x8C\x8D");

	dbg::println("{} in bytes {}", u8str.size(), u8str.size_in_bytes());


	for (const auto& cp : u8str)
	{
		dbg::println("{:X}", (int)cp);
	}


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
	   .width  = 1'280,
	   .height = 720,
	   .flags  = Attribute::vsync | Attribute::resizable});

	app01.set_title(std::format("{}", sizeof(vulkanapp)));

	app01.set_keyboard_callback(keyboard_callback);
	app01.set_fixed_update_callback(fixed_update);
	app01.set_update_callback(update);
	app01.set_render_callback(render);


	return app01.run();
}
