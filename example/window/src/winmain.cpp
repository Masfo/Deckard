#include <windows.h>
#include <Commctrl.h>
#include <intrin.h>
#include <random> // workaround for module std

import std;
import deckard;
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

void test_cb01() { dbg::println("cb test 01"); }

void test_cb02() { dbg::println("cb test 02"); }

std::string convert_str(const auto arg)
{
	if constexpr (std::is_same_v<bool, std::remove_cv_t<decltype(arg)>>)
		return "bool";


	if constexpr (std::is_integral_v<std::remove_cv_t<decltype(arg)>>)
		return "int";

	if constexpr (std::is_floating_point_v<std::remove_cv_t<decltype(arg)>>)
		return "float";


	if constexpr (std::is_convertible_v<std::remove_cv_t<decltype(arg)>, std::string_view>)
		return "string";

	// if constexpr (std::is_pointer_v<std::remove_cv_t<decltype(arg)>>)
	//	return "pointer";

	dbg::panic("unhandled type");
}

template<typename... Args>
auto varsum(Args... args)
{
	(dbg::println("{} - {}", args, convert_str(args)), ...);

	return std::make_tuple(args...); // Performs a binary right fold with addition
}

union float4_2
{

	__m128 reg{0.0f, 0.0f, 0.0f, 0.0f};

	f32 x;
	f32 y;
	f32 z;
	f32 w;

	float4_2() = default;

	float4_2(f32 v1, f32 v2, f32 v3, f32 v4)
	{
		x = v1;
		y = v2;
		z = v3;
		w = v4;
	}
};

auto operator+(const float4_2& lhs, const float4_2& rhs)
{
	float4_2 result{};

	result.reg = _mm_add_ps(lhs.reg, rhs.reg);

	return result;
}

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

template<typename T>
int sgn(T val)
{
	return (T(0) < val) - (val < T(0));
}

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

std::pair<std::array<u8, 4>, u32> static_encode_codepoint(char32 cp)
{
	std::array<u8, 4> bytes{0};
	u32               count = 0;
	if (cp <= 0x7F)
	{
		bytes[0] = (static_cast<u8>(cp));
		count    = 1;
	}
	else if (cp <= 0x7FF)
	{
		bytes[0] = (static_cast<u8>((cp >> 6) | 0xC0));
		bytes[1] = (static_cast<u8>((cp & 0x3F) | 0x80));
		count    = 2;
	}
	else if (cp <= 0xFFFF)
	{
		bytes[0] = (static_cast<u8>((cp >> 12) | 0xE0));
		bytes[1] = (static_cast<u8>(((cp >> 6) & 0x3F) | 0x80));
		bytes[2] = (static_cast<u8>((cp & 0x3F) | 0x80));
		count    = 3;
	}
	else if (cp <= 0x10'FFFF)
	{
		bytes[0] = (static_cast<u8>((cp >> 18) | 0xF0));
		bytes[1] = (static_cast<u8>(((cp >> 12) & 0x3F) | 0x80));
		bytes[2] = (static_cast<u8>(((cp >> 6) & 0x3F) | 0x80));
		bytes[3] = (static_cast<u8>((cp & 0x3F) | 0x80));
		count    = 4;
	}
	else
	{
		bytes[0] = (0xEF);
		bytes[1] = (0xBF);
		bytes[2] = (0xBD); // U+FFFD replacement character
		count    = 3;
	}
	return {bytes, count};
}

std::vector<u8> encode_codepoint(char32 codepoint)
{
	std::vector<u8> bytes;
	if (codepoint <= 0x7F)
	{
		bytes.push_back(static_cast<u8>(codepoint));
	}
	else if (codepoint <= 0x7FF)
	{
		bytes.push_back(static_cast<u8>((codepoint >> 6) | 0xC0));
		bytes.push_back(static_cast<u8>((codepoint & 0x3F) | 0x80));
	}
	else if (codepoint <= 0xFFFF)
	{
		bytes.push_back(static_cast<u8>((codepoint >> 12) | 0xE0));
		bytes.push_back(static_cast<u8>(((codepoint >> 6) & 0x3F) | 0x80));
		bytes.push_back(static_cast<u8>((codepoint & 0x3F) | 0x80));
	}
	else if (codepoint <= 0x10'FFFF)
	{
		bytes.push_back(static_cast<u8>((codepoint >> 18) | 0xF0));
		bytes.push_back(static_cast<u8>(((codepoint >> 12) & 0x3F) | 0x80));
		bytes.push_back(static_cast<u8>(((codepoint >> 6) & 0x3F) | 0x80));
		bytes.push_back(static_cast<u8>((codepoint & 0x3F) | 0x80));
	}
	else
	{
		bytes.push_back(0xEF);
		bytes.push_back(0xBF);
		bytes.push_back(0xBD); // U+FFFD replacement character
	}
	return bytes;
}

constexpr std::array<u8, 364> utf8_table{
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  10, 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,
  3,  3,  11, 6,  6,  6,  5,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  0,  12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12, 12, 0,  12, 12, 12, 12, 12, 0,  12, 0,  12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12,
  12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
};
constexpr u32 UTF8_ACCEPT{0};
constexpr u32 UTF8_REJECT{12};

i32 deckard_main(std::string_view commandline)
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", window::build::version_string, window::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif

	{
		i64 index = 0;

		// buffer                 = {0xE7, 0x95, 0x8C};

		auto last_codepoint = [](const std::span<u8> buffer, i64 current_index)
		{
			if (current_index >= 0)
			{
				current_index -= 1;

				while (current_index > 0 and utf8::is_continuation_byte(buffer[current_index]))
					current_index -= 1;

				if (current_index < 0)
					current_index = 0;
			}
			return current_index;
		};

		auto next_codepoint = [](const std::span<u8> buffer, i64 current_index) -> i64
		{
			if (current_index >= buffer.size())
				return buffer.size();

			i64 next = current_index;
			next++;

			while (next < buffer.size() and utf8::is_continuation_byte(buffer[next]))
				next++;

			while (next < buffer.size())
			{
				u8 byte = buffer[next];
				if (not utf8::is_single_byte(byte))
					break;

				if (not utf8::is_start_of_codepoint(byte))
					break;

				next++;
			}

			return next;
		};

		auto decode = [](const std::span<u8> buffer, i64 index) -> char32
		{
			constexpr char32 REPLACEMENT_CHARACTER{0xFFFD}; // U+FFFD 0xEF 0xBF 0xBD(239, 191, 189) REPLACEMENT CHARACTER

			if (index >= buffer.size())
				return REPLACEMENT_CHARACTER;

			u32    state     = 0;
			char32 codepoint = 0;
			for (; index < buffer.size(); index++)
			{
				u8 byte = buffer[index];

				const u32 type = utf8_table[byte];
				codepoint      = state ? (byte & 0x3fu) | (codepoint << 6) : (0xffu >> type) & byte;
				state          = utf8_table[256 + state + type];
				if (state == 0)
					return codepoint;
				else if (state == UTF8_REJECT)
					return REPLACEMENT_CHARACTER;
			}

			return REPLACEMENT_CHARACTER;
		};

		auto replace_codepoint = [](std::vector<u8>& buffer, i64 pos, char32 new_codepoint) -> bool
		{
			if (pos >= buffer.size())
				return false;

			i64 next = pos;
			next++;

			while (next < buffer.size() and utf8::is_continuation_byte(buffer[next]))
				next++;

			size_t bytes_to_remove  = next - pos;
			auto [new_bytes, count] = static_encode_codepoint(new_codepoint);
			buffer.erase(buffer.begin() + pos, buffer.begin() + next);
			buffer.insert(buffer.begin() + pos, new_bytes.begin(), new_bytes.begin() + count);
			return true;
		};


		//                         0     1           3                 6                        10
		std::vector<u8> buffer = {0x41, 0xC3, 0x84, 0xE2, 0x86, 0xA5, 0xF0, 0x9F, 0x8C, 0x8D, 0x42};

		// 1.


		i64 current_codepoint = 0;
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = next_codepoint(buffer, 0);                 // 1
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = next_codepoint(buffer, current_codepoint); // 3
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = next_codepoint(buffer, current_codepoint); // 6
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = next_codepoint(buffer, current_codepoint); // 10
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		goto skip;
		current_codepoint = last_codepoint(buffer, current_codepoint); // 6
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = last_codepoint(buffer, current_codepoint); // 3
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = last_codepoint(buffer, current_codepoint); // 1
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = last_codepoint(buffer, current_codepoint); // 0
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));
		dbg::println();
	skip:

		dbg::println("replaced: {}", replace_codepoint(buffer, 1, 0x41));
		current_codepoint = 0;
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = next_codepoint(buffer, 0);                 // 1
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = next_codepoint(buffer, current_codepoint); // 3
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = next_codepoint(buffer, current_codepoint); // 6
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));

		current_codepoint = next_codepoint(buffer, current_codepoint); // 10
		dbg::println("{} - {:X}", current_codepoint, (u32)decode(buffer, current_codepoint));


		dbg::println("----");

		auto [bstr, count_bstr] = static_encode_codepoint(0x0001'F30D);
		char cstr_ref[5]{};
		std::memcpy(cstr_ref, bstr.data(), count_bstr);

		dbg::println("i: {}", cstr_ref);

		auto smallstr = utf8::string2("AÄ↥🌍B");
		auto longstr  = utf8::string2("extra super hyper mega long buffer AÄ↥🌍B>");

		//
		//		dbg::println("small: {}", smallstr);
		//		dbg::println("long : {}", longstr);
		//
		//		for (auto it = smallstr.begin(); it != smallstr.end(); ++it)
		//			dbg::println("1) [{:X}] {}", *it, it.width());
		//
		//		for (auto it = smallstr.rbegin(); it != smallstr.rend(); ++it)
		//			dbg::println("2) [{:X}]", *it);
		//
		constexpr f32 FEPS = 0.001f;

		f32 x = 3.14f;

		auto float_equal = [](f32 left, f32 right) -> bool
		{
			f32 diff = left - right;
			return diff < FEPS and diff > -FEPS;
		};

		dbg::println("{}", float_equal(3.14f, 3.14f));
		dbg::println("{}", float_equal(3.14f, 3.16f));
		dbg::println("{}", float_equal(3.14f, 4.10f));





        std::string abc("abc");

        auto abc_it = abc.begin() + 1;
        *abc_it = 'x'; // Replace character at position 1 with 'x'

		smallstr = utf8::string2("AÄ↥🌍B");
		dbg::println("{} codepoints:", smallstr);
		for (auto it : smallstr)
			dbg::print("{:X} ", it);
		dbg::println();

		auto it = smallstr.begin() + 3;
		
		//it.set_codepoint(0x44);

		dbg::println("after modify: {} codepoints:", smallstr);
		for (auto it : smallstr)
			dbg::print("{:X} ", it);

		// 🔴🟡🟢🔵🟣🟠🟤⚫⚪🟦🟧🟨🟩🟪🟫
		// ⚠️ 👀

		utf8::string2 abc1("AÄ↥🌍B");
		utf8::string2 abc2("AÄ↥🌍B");
		utf8::string2 abc3("ABCDE");

		char32 abcd1 = abc1[0]; // A
		char32 abcd2 = abc1[1]; // Ä
		char32 abcd3 = abc1[3]; // 🌍
		char32 abcd4 = abc1[4]; // B

		//abc1[3] = 0x44;         


		dbg::println("true  = {}", abc1 == abc2);
		dbg::println("false = {}", abc1 == abc3);


		// std::ranges::reverse_view rv{smallstr};
		//	for (auto it : rv)
		//		dbg::println("4) [{:X}]", it);

		int k = 0;
	}


	graph::graph<i32> g(5);

	g.add(0, 1, 4);
	g.add(0, 2, 8);
	g.add(1, 2, 2);
	g.add(1, 3, 6);
	g.add(2, 3, 3);
	g.add(3, 4, 5);
	g.add(4, 0, 7);

	print(g);


	for (auto x : gen())
		dbg::print("{}.", x);

	dbg::println();


	Noisy<int> n1{456};

	Noisy<int> n2 = std::move(n1);


	int k = 0;


#if 0
	Tree<char> tree[]{{'D', tree + 1, tree + 2}, {'B', tree + 3, tree + 4}, {'F', tree + 5, tree + 6}, {'A'}, {'C'}, {'E'}, {'G'}};

	for (char x : tree->traverse_inorder())
		dbg::print("{} ", x);
	dbg::println();
#endif

	dbg::println("d {}", count_digits(-51));


	// ###################


	u32         q2{};
	i16         q22{};
	std::string xjk1("uie");
	int*        x{};
	// 12345678901234567890123456789012
	// 00111110110011001100110011001101
	dbg::println("{:032b}", std::bit_cast<u32>(0.4f));

	auto oke = varsum("🐱", "constchar*", xjk1, "hello"sv, 1, 3.4, 2.3f, q2, true, q22);

	// ###################


	auto fdsc = make_array<f32>(1, 2, 3, 4);


	using enum string::option;
	string::option op{};
	auto           i22 = strip("ako", op);

	auto io = include_only(string::replace("p=12,36 v=45,99", ",", " "), w | d);


	auto        vkox = varsum<i32, std::string>(1, "555");
	std::string stt("hello world");


	auto testext = read_text_file("test01.txt");


	// ###################

	dbg::println("{}", string::match("*X*", "qH1") ? "match!" : "not found");

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
	dbg::println("len: {}", utf8::length(sss).value_or(-1));


	dbg::println("  valid string    = {}", utf8::is_valid("\xf0\x90\x8c\xbc"));
	dbg::println("invalid 2nd octet = {}", utf8::is_valid("\xf0\x28\x8c\xbc"));
	dbg::println("invalid 3rd octet = {}", utf8::is_valid("\xf0\x90\x28\xbc"));
	dbg::println("invalid 4th octet = {}", utf8::is_valid("\xf0\x28\x8c\x28"));

	bool ivalid = utf8::is_valid(sss);

	dbg::println("len: {}", utf8::length("a√πa").value_or(-1));
	dbg::println("len: {}", utf8::length("\xf0\x28\x8c\xbc").value_or(-1));

	LPCWSTR     cmdstr  = L"a√aπa";
	const char* cmdstr2 = "B";
	cmdstr2             = "\xe2\x88\x9a";


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

	// 1 byte: A   0x41			0x41
	// 2 byte: Ä   0xC4			0xC3 0x84
	// 3 byte: ↥   0x21A8		0xE2 0x86 0xA8
	// 4 byte: 🌍  0x1F30D		0xF0 0x9F 0x8C 0x8D
	std::string u8str_d("\x41\xC3\x84\xE2\x86\xA8\xF0\x9F\x8C\x8D");

	u32 index = 0;


	for (auto i : range(10, 15))
	{
	}


	auto [bytes, codepoint] = decode_codepoint({(u8*)u8str_d.data(), u8str_d.size()}, index);


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
