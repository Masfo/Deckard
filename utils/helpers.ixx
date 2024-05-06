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

	export template<typename T, typename U>
	T load_bigendian(U const bytes)
	{
		T ret{};
		std::memcpy(&ret, bytes, sizeof(T));
		return std::byteswap(ret);
	}

	template<typename T>
	auto as_bytes(T data)
	{
		return std::as_bytes(std::span{data});
	}

	template<typename T>
	auto as_writable_bytes(T data)
	{
		return std::as_writable_bytes(std::span{data});
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

	// Prettys
	std::string PrettyBytes(u64 bytes) noexcept
	{

		constexpr std::array<char[6], 9> unit{{"bytes", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"}};

		auto count = static_cast<f64>(bytes);
		u64  suffix{0};

		while (count >= static_cast<f64>(1_KiB) && suffix <= unit.size())
		{
			suffix++;
			count /= static_cast<f64>(1_KiB);
		}

		if (std::fabs(count - std::floor(count)) == 0.0)
			return std::format("{} {}", static_cast<u64>(count), unit[suffix]);
		return std::format("{:.2f} {}", count, unit[suffix]);
	}

} // namespace deckard
