#include <windows.h>
#include <Commctrl.h>
#include <random> // workaround for module std
import deckard;
using namespace deckard;
using namespace deckard::app;
using namespace deckard::random;
import std;

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

} // namespace nt

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

int deckard_main()
{

	array2d<u8> grid(512, 512);
	grid.fill(255);

	grid.dump<char>();


	grid.set(1, 1, 0);
	grid.set(1, 2, 0);

	grid.set(2, 1, 128);

	grid.line(0, 0, 128, 128, 0);
	grid.line(129, 129, 129, 511, 64);

	grid.circle(380, 380, 64, 128);

	grid.raster_circle(100, 400, 64, 128);

	grid.basic_bezier(32, 32, 35, 380, 400, 400, 64);

	char c6 = grid.at(1, 6);

	for (int i : upto(128))
	{
		grid.line(0, 0, rnd(0, 128), rnd(128, 256), rnd(64, 128));
		grid.line(256, 256, rnd(128, 64 + 256 + 64), rnd(64, 64 + 256 + 64), rnd(0, 164));

		grid.circle(rnd(0, 511), rnd(0, 511), rnd(10, 64), rnd(16, 128));
	}

#if 0
	for (int i : upto(grid.width()))
	{


		if (random::randbool())
			grid.reverse_col(i);
	}
	for (int i : upto(grid.height()))
	{

		if (random::randbool())
			grid.reverse_row(i);
	}
#endif


	// grid.transpose();
	// grid.reverse_col(1);

	grid.export_ppm("out.pgm");

	auto hashes = grid.find_all('#');


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


	int kx = 0;

	auto insi = read_file("input.ini");

	std::string        line;
	std::istringstream iss(insi);
	while (std::getline(iss, line, '\n'))
	{
		dbg::println("line: {}", line);
	}


	std::string cmdparse("-v -o\"file.txt\" -d 1024");

	nt::commandliner cmd(cmdparse);
	cmd.process();


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
