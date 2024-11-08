#include <windows.h>
#include <Commctrl.h>
#include <intrin.h>
#include <random> // workaround for module std

import deckard;
using namespace deckard;
using namespace deckard::app;
using namespace deckard::random;
import std;

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

// Token types
enum TokenType2 : u8
{
	IDENTIFIER,
	NUMBER,
	PLUS,
	MINUS,
	TIMES,
	DIVIDE,
	LEFT_PAREN,
	RIGHT_PAREN,
	EOF_TOKEN
};

// Token structure
struct Token
{
	TokenType2  type;
	std::string value;
};

// Lexer class
class Lexer
{
public:
	Lexer(const std::string& input)
		: input(input)
		, current_pos(0)
	{
	}

	Token next_token()
	{
		while (isspace(input[current_pos]))
		{
			++current_pos;
		}

		if (current_pos == input.length())
		{
			return Token{EOF_TOKEN, ""};
		}

		char current_char = input[current_pos];

		if (isalpha(current_char))
		{
			std::string identifier;
			while (isalnum(current_char))
			{

				identifier += current_char;
				++current_pos;
				current_char = input[current_pos];
			}
			return Token{IDENTIFIER, identifier};
		}

		if (isdigit(current_char))
		{
			std::string number;
			while (isdigit(current_char))
			{

				number += current_char;
				++current_pos;
				current_char = input[current_pos];
			}
			return Token{NUMBER, number};
		}

		switch (current_char)
		{
			case '+': return Token{PLUS, "+"};
			case '-': return Token{MINUS, "-"};
			case '*': return Token{TIMES, "*"};
			case '/': return Token{DIVIDE, "/"};
			case '(': return Token{LEFT_PAREN, "("};
			case ')': return Token{RIGHT_PAREN, ")"};
			default: dbg::eprintln("Error: Unexpected character: {}", current_char); exit(1);
		}
	}

private:
	const std::string input;
	size_t            current_pos;
};

// Parser class
class Parser
{
public:
	Parser(Lexer& lexer)
		: lexer(lexer)
		, current_token(lexer.next_token())
	{
	}

	int expression()
	{
		int left = term();
		while (current_token.type == PLUS || current_token.type == MINUS)
		{
			if (current_token.type == PLUS)
			{
				eat(PLUS);
				left += term();
			}
			else
			{
				eat(MINUS);
				left -= term();
			}
		}
		return left;
	}

	int term()
	{
		int left = factor();
		while (current_token.type == TIMES || current_token.type == DIVIDE)
		{
			if (current_token.type == TIMES)
			{
				eat(TIMES);
				left *= factor();
			}
			else
			{
				eat(DIVIDE);
				left /= factor();
			}
		}
		return left;
	}

	int factor()
	{
		int value;
		if (current_token.type == NUMBER)
		{
			value = std::stoi(current_token.value);
			eat(NUMBER);
		}
		else if (current_token.type == LEFT_PAREN)
		{
			eat(LEFT_PAREN);
			value = expression();
			eat(RIGHT_PAREN);
		}
		else
		{
			dbg::eprintln("Error: Expected NUMBER or LEFT_PAREN, {}", current_token.value);
			return 0;
		}
		return value;
	}

	int eat(TokenType2 expected_type)
	{
		if (current_token.type == expected_type)
		{
			current_token = lexer.next_token();
		}
		else
		{
			dbg::eprintln("Error: Expected {} but found {}, {}", (int)expected_type, (int)current_token.type, current_token.value);
			return 0;
		}
	}

private:
	Lexer& lexer;
	Token  current_token;
};

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

	void set(std::string_view input)
	{
		//
	}
};

using SmallStringBuffer = basic_smallbuffer<u8, 32>;

static_assert(sizeof(SmallStringBuffer) == 32, "SmallStringBuffer should be 32-bytes");

class u8str
{
private:
	SmallStringBuffer sbo{};

public:
};

template<typename T>
class adjacencylist
{
private:
	std::vector<std::vector<T>> list;

public:
};

union float4
{
public:
	__m128 v;

	struct _c
	{
		f32 x, y, z, w;
	} c;

public:
	float4()
		: float4(0.0f)
	{
	}

	float4(f32 scalar) { v = _mm_set_ps1(scalar); }

	float4(f32 x, f32 y)
		: float4(x, y, 0.0f, 0.0f)
	{
	}

	float4(f32 x, f32 y, f32 z)
		: float4(x, y, z, 0.0f)
	{
	}

	float4(f32 x, f32 y, f32 z, f32 w) { v = _mm_setr_ps(x, y, z, w); }
};

union float2
{
	__m128 v{0.0f, 0.0f, 0.0f, 0.0f};

	struct _c
	{
		float x, y, pad1, pad2;
	} c;

public:
	float2()
		: float2(0.0f)
	{
	}

	float2(f32 s) { v = _mm_setr_ps(s, s, 1.0f, 1.0f); }

	float2(f32 x, f32 y) { v = _mm_setr_ps(x, y, 1.0f, 1.0f); }
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
	int              n1 = num1.size(), n2 = num2.size();
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

int deckard_main()
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", window::build::version_string, window::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif


	// ########################################################################


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

	db.prepare("SELECT log_id AS result FROM blobs WHERE id = 35;").commit();
	auto v64 = db.at<u64>("log_id", 0);
	dbg::println("v: {}", v64 ? *v64 : 0);

	auto bfs = db.bind_function(
	  "my_add",
	  2,
	  [](sqlite3_context* ctx, int argc, sqlite3_value** argv)
	  {
		  auto x = sqlite3_value_double(argv[0]);
		  auto y = sqlite3_value_double(argv[1]);
		  sqlite3_result_double(ctx, x + y);
	  });

	db.prepare("SELECT 3.14+0xFFFFFFFFFFFF as result").commit();

	auto vresult2 = db.at<f64>("result");
	if (vresult2)
	{
		dbg::println("values {}", *vresult2);
	}

	auto bind_test = []
	{
		std::vector<u8> v;
		v.push_back('H');
		v.push_back('I');
		v.push_back('!');
		return v;
	};


	db.prepare("INSERT INTO blobs (log_id, text) VALUES (?1,?2)").bind(1, "testing").commit();
	db.bind(epoch(), random::alpha(16)).commit();

	db.prepare("INSERT INTO blobs (data) VALUES (?1)").bind(bind_test()).commit();


	db.prepare("SELECT * FROM blobs WHERE id=23;").commit();


	db.close();


	auto k = math::index_from_3d(1, 1, 1, 3, 3);

	i32 ab1 = -125;

	dbg::println("{}", std::in_range<u16>(ab1));
	dbg::println("{}", std::in_range<i16>(ab1));


	// 0x07e6c88e44dd2
	// 0xC379AAA42D208
	std::vector<int> num1 = {3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1, 3};
	std::vector<int> num2 = {7, 8, 9, 8, 5, 4, 6, 3, 2, 1, 9, 8, 6, 2, 1, 6, 8, 4, 6, 2, 1, 3, 5, 1, 9, 8, 4, 3, 2, 1, 2, 6, 5, 4};

	auto res = multiply_large_numbers(num1, num2);
	// 3438837141525000
	for (int digit : res)
		dbg::print("{}", digit);
	dbg::println();
	dbg::println("25379713406982738815160559254284513486903839651407928201597302");


	float4 vq;

	vq.c.x = 2.0f;
	vq.c.y = 2.0f;


	grid<u8> g;
	g.resize(1'024, 1'024);

	i32       iX = 0, iY = 0;
	const i32 iXmax = g.width();
	const i32 iYmax = g.height();

	f64       Cx, Cy;
	const f64 CxMin = -1.5;
	const f64 CxMax = 0.5;
	const f64 CyMin = -1.0;
	const f64 CyMax = 1.0;
	/* */
	f64 PixelWidth  = (CxMax - CxMin) / iXmax;
	f64 PixelHeight = (CyMax - CyMin) / iYmax;

	i32          Iteration    = 0;
	const int    IterationMax = 400;
	const double EscapeRadius = 2;
	double       ER2          = EscapeRadius * EscapeRadius;

	unsigned char color[3]{};

	for (iY = 0; iY < iYmax; iY++)
	{
		Cy = CyMin + iY * PixelHeight;
		if (fabs(Cy) < PixelHeight / 2)
			Cy = 0.0; /* Main antenna */
		for (iX = 0; iX < iXmax; iX++)
		{
			Cx = CxMin + iX * PixelWidth;
			/* initial value of orbit = critical point Z= 0 */
			f64 Zx  = 0.0;
			f64 Zy  = 0.0;
			f64 Zx2 = Zx * Zx;
			f64 Zy2 = Zy * Zy;
			/* */
			for (Iteration = 0; Iteration < IterationMax && ((Zx2 + Zy2) < ER2); Iteration++)
			{
				Zy  = 2 * Zx * Zy + Cy;
				Zx  = Zx2 - Zy2 + Cx;
				Zx2 = Zx * Zx;
				Zy2 = Zy * Zy;
			};
			/* compute  pixel color (24 bit = 3 bytes) */
			if (Iteration == IterationMax)
			{ /*  interior of Mandelbrot set = black */
				color[0] = 0;
				color[1] = 0;
				color[2] = 0;
			}
			else
			{                         /* exterior of Mandelbrot set = white */
				color[0] = Iteration; /* Red*/
				color[1] = Iteration; /* Green */
				color[2] = Iteration; /* Blue */
			};
			g.set(iX, iY, color[0]);
		}
	}


	g.export_ppm("out.pgm");


	file f1("dir🌍\\input.bin");

	std::array<u8, 16> rdata{};

	random::random_bytes(rdata);

	u64 wyh = utils::rapidhash(rdata);


	dbg::println("wrote: {}", f1.seek_write(rdata, 16, 64));

	f1.close();

	auto kos = limits::min<bool>;

	quat q1(vec3(1.0f, 2.0f, 3.0f));


	using namespace std::string_literals;


	std::allocator<u8> alloc;
	u8*                ptr_u8 = alloc.allocate(16);


	std::uninitialized_fill_n(ptr_u8, 16, 'X');

	ptr_u8[0] = 100;

	alloc.deallocate(ptr_u8, 16);
	ptr_u8 = nullptr;


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


#if 0
	{
		int                     nButtonPressed = 0;
		TASKDIALOGCONFIG        config         = {0};
		const TASKDIALOG_BUTTON buttons[]      = {
          {100, L"Update"},
          {101, L"Play now"},
        };

		const TASKDIALOG_BUTTON radiobuttons[] = {
		  {100, L"Update 2 "},
		  {101, L"Play now 2"},
		};
		config.cbSize             = sizeof(config);
		config.pszWindowTitle     = L"Deckard";
		config.hInstance          = 0;
		config.dwFlags            = TDF_USE_COMMAND_LINKS | TDF_ENABLE_HYPERLINKS;
		config.dwCommonButtons    = TDCBF_CLOSE_BUTTON;
		config.pszMainIcon        = TD_INFORMATION_ICON;
		config.pszMainInstruction = L"Taboo Builder";
		config.pszContent =
		  L"New update available, v1.0.1234.DEADBEEF\n\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings Fixed server pings Fixed server pings Fixed server pings\n"
		  L"* Update function no longer hangs\n"
		  "\n\n<a href=\"https://www.taboobuilder.com/patchnotes/\">Read more patch notes</a>\n";
		config.pButtons = buttons;
		config.cButtons = ARRAYSIZE(buttons);


		config.pfCallback = [](HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData) -> HRESULT
		{
			switch (uNotification)
			{
				case TDN_HYPERLINK_CLICKED: dbg::println("url"); break;
				default: break;
			}
			return S_OK;
		};

		using TaskDialogIndirectFunc =
		  HRESULT(const TASKDIALOGCONFIG* pTaskConfig, int* pnButton, int* pnRadioButton, BOOL* pfVerificationFlagChecked);
		TaskDialogIndirectFunc* TaskDialogIndirect = nullptr;

		HMODULE mod = LoadLibraryA("Comctl32.dll");
		if (mod)
		{
			TaskDialogIndirect = reinterpret_cast<TaskDialogIndirectFunc*>(GetProcAddress(mod, "TaskDialogIndirect"));
		}

		if (TaskDialogIndirect)
		{
			auto err = TaskDialogIndirect(&config, &nButtonPressed, NULL, NULL);
			if (SUCCEEDED(err))
			{
				switch (nButtonPressed)
				{
					case 0: dbg::println("Dialog failed"); break;
					case IDCANCEL: dbg::println("Dialog cancel"); break;
					case IDNO: dbg::println("Dialog no"); break;
					case IDRETRY: dbg::println("Dialog retry"); break;
					case IDYES: dbg::println("Dialog yes"); break;
					case 100: dbg::println("Update"); break;
					case 101: dbg::println("Play"); break;


					default: break;
				}
			}
		}

		FreeLibrary(mod);
	}
#endif
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
