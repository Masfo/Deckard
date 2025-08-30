#include <windows.h>
#include <Commctrl.h>
#include <intrin.h>


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
	// ########################################################################
	{
		constexpr size_t mof_size  = sizeof(std::move_only_function<void()>);
		constexpr size_t stdf_size = sizeof(std::function<void()>);
		constexpr size_t fr_size   = sizeof(function_ref<void()>);
		dbg::println("move_only_function size: {} / std::function size: {}, function_ref size: {}", mof_size, stdf_size, fr_size);

		using function_t = function_ref<void()>;

		std::mutex                       mtx;
		std::mutex                       vfr_mutex;
		std::deque<function_t> vfr;
		std::condition_variable          cv;
		std::condition_variable          cv1;

		std::atomic_int counter{0}, threader_tasks{0};

		std::atomic_bool stop{false};

		bool ready{false};
		for (int i = 0; i < 20; i++)
		{
			vfr.push_back([i,&counter] 
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

	for (int i = 0; i < 50; i++)
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

	_;
	// ########################################################################

	auto start = clock_now();


	std::this_thread::sleep_for(2541ms);

	clock_delta("sleep", start);

	_ = 0;

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

	auto argparser = [&](std::string_view cmdl) { };

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


	Secret a{42};
	Secret b{58};

	Secret c{a - b};


	_ = 0;

	// ###############################################


	using callable_ref = function_ref<int(int)>;

	std::vector<callable_ref> vints;


	vints.push_back([](int input) -> int { return input * 2; });
	vints.push_back(
	  [](int input) -> int
	  {
		  dbg::println("input*3: {}", input);
		  return input * 3;
	  });
	vints.push_back(function_call);

	dbg::println("size: {} / {}", sizeof(callable_ref), vints.size());

	auto copy_vints = vints;

	for (const auto& callable : copy_vints)
	{
		int result = callable(6);
		dbg::println("result: {}", result);
	}

	// ###############################################

	auto process_result = system::execute_process("alinesleep.exe", "1", 1s);


	_ = 0;

	// ###############################################

	auto execute_proc =
	  [](std::string_view      executable,
		 std::string_view      commandline,
		 std::filesystem::path working_directory = std::filesystem::current_path())
	{
		//
	};


	_ = 0;
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

	auto rle = compress_rle<std::string>("WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW");

	for (const auto& i : rle)
	{
		dbg::println("{} - {}", i.first, i.second);
	}
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
