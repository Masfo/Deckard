export module deckard.helpers;

import std;
import deckard.types;
import deckard.assert;
import deckard.debug;

export namespace deckard
{
	// upto 0..n-1, P3060
	inline constexpr auto upto = []<std::integral I>(I n) { return std::views::iota(I{}, n); };

	// loop (n, n+1, n+..)
	inline constexpr auto loop = []<std::integral I>(I start = 0) { return std::views::iota(start); };

	// repeat
	template<size_t Count>
	struct repeat_t final
	{
		template<std::invocable Func>
		constexpr void operator=(Func &&f) const noexcept
		{
			size_t count = Count;
			while (count--)
				f();
		}
	};

	template<size_t Count>
	inline constexpr repeat_t<Count> repeat;

	// to hex
	inline void to_hex(const std::span<u8> &input, std::span<u8> &output, char delimiter = ' ')
	{
		//
		constexpr char dict[32 + 1]{"0123456789abcdef0123456789ABCDEF"};

		// 61 61 61 61
		assert::if_true(output.size() >= (input.size() * 2 + input.size() - 1), "Not enough space in output buffer");
	}

	export template<typename T, typename U>
	T load_bigendian(U const bytes)
	{
		T ret{};
		std::memcpy(&ret, bytes, sizeof(T));
		return std::byteswap(ret);
	}

	//
	constexpr bool is_bit_set(u64 value, u32 bitindex) noexcept { return ((value >> bitindex) & 1) ? true : false; }

	auto clock_now() noexcept { return std::chrono::high_resolution_clock::now(); }

	// ScopeTimer
	class ScopeTimer
	{
	public:
		explicit ScopeTimer(std::string_view scopename)
		{
			name = scopename;
			start();
		}

		~ScopeTimer() { stop(); }

		void start() noexcept { start_time = clock_now(); }

		void stop() noexcept
		{
			std::chrono::duration<double, std::milli> duration(clock_now() - start_time);
			dbg::println("{} took {}", name, duration);
		}

	private:
		std::string                           name;
		std::chrono::steady_clock::time_point start_time{};
	};

} // namespace deckard
