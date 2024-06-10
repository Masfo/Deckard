export module deckard.helpers;

import std;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.debug;

using namespace std::string_view_literals;

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
		constexpr void operator=(Func&& f) const noexcept
		{
			size_t count = Count;
			while (count--)
				f();
		}
	};

	template<size_t Count>
	inline constexpr repeat_t<Count> repeat;


	template<typename T>
	concept HasDataMember = requires(T obj) { obj.data(); };

	export template<typename To, typename From>
	To load_as(const From from) noexcept
	{
		// TODO: Bitcast
		To ret{};
		if constexpr (HasDataMember<From>)
			std::memcpy(&ret, from.data(), sizeof(To));
		else
			std::memcpy(&ret, from, sizeof(To));

		return ret;
	}

	export template<typename To, typename From>
	To load_as_be(const From from) noexcept
	{
		return std::byteswap(load_as<To>(from));
	}

	//

	template<typename T = u8>
	constexpr T bitmask(u8 size, u8 offset = 0)
	{
		return ((1 << size) - 1) << offset;
	}

	constexpr bool test_bit(u64 value, u32 bitindex) noexcept { return ((value >> bitindex) & 1) ? true : false; }

	auto clock_now() noexcept { return std::chrono::high_resolution_clock::now(); }

	void clock_stop(std::string_view id, std::chrono::steady_clock::time_point start)
	{
		std::chrono::duration<double, std::milli> took(clock_now() - start);
		std::println("{} took {}", id, took);
	}

	// ScopeTimer
	template<typename R = std::milli>
	class ScopeTimer
	{
	public:
		explicit ScopeTimer(std::string_view scopename)
		{
			name = scopename;
			start();
		}

		~ScopeTimer()
		{
			if (not stopped)
				now();
		}

		void start() noexcept { start_time = clock_now(); }

		void stop() noexcept
		{
			now();
			stopped = true;
		}

		void now() noexcept { dbg::println("{} took {}", name, duration()); }

		auto duration() noexcept
		{
			std::chrono::duration<float, R> dur(clock_now() - start_time);
			return dur;
		}

	private:
		std::string                           name;
		std::chrono::steady_clock::time_point start_time{};
		bool                                  stopped{false};
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

	// hash_combine


	// to_hex

	struct HexOption
	{
		std::string delimiter{", "};
		bool        endian_swap{std::endian::native == std::endian::little};
		bool        lowercase{false};
		bool        show_hex{true};
	};

	template<typename T = u8>
	u64 to_hex(const std::span<T> input, std::span<u8> output, const HexOption& options = {}) noexcept
	{
		const u64 delimiter_len = input.size() == 1 ? 0 : as<u64>(options.delimiter.size());
		const u64 hex_len       = options.show_hex ? 2 : 0;

		const u64 stride = (sizeof(T) * 2) + delimiter_len + hex_len;
		const u64 maxlen = (as<u64>(input.size()) * stride) - delimiter_len;

		if (input.empty() or output.size() < maxlen or output.empty())
			return maxlen;

		constexpr static std::array<u8, 32> HEX_LUT{
		  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
		const i32 lowercase_offset = options.lowercase ? 16 : 0;

		const bool show_hex = options.show_hex;
		u64        len      = 0;

		for (const auto [i, word] : std::views::enumerate(input))
		{
			const T input_word = options.endian_swap ? std::byteswap(word) : word;

			u32 offset = 0;
			if (show_hex)
			{
				output[i * stride + offset] = '0';
				offset += 1;

				output[i * stride + offset] = 'x';
				offset += 1;
				len += 2;
			}

			for (u8 byteindex = 0; byteindex < sizeof(T); byteindex++)
			{
				const T  shift       = byteindex * 8;
				const T  mask        = sizeof(T) == 1 ? 0xFF : as<T>(0xFF_u8) << shift;
				const u8 masked_byte = as<u8>((input_word & mask) >> shift);


				output[i * stride + offset] = HEX_LUT[((masked_byte) >> 4) + lowercase_offset];
				offset += 1;

				output[i * stride + offset] = HEX_LUT[((masked_byte) & 0xF) + lowercase_offset];
				offset += 1;
			}

			len += sizeof(T);

			for (size_t j = 0; (as<u64>(i) < input.size() - 1) and (j < options.delimiter.size()); j++)
			{
				output[i * stride + offset] = options.delimiter[j];
				offset += 1;
				len += 1;
			}
		}
		return len;
	}

	template<typename T>
	std::string to_hex_string(const std::span<T> input, const HexOption& options = {}) noexcept
	{
		std::string ret;

		ret.resize(to_hex(input, {}, options));

		(void)to_hex<T>(input, {as<u8*>(ret.data()), ret.size()}, options);

		return ret;
	}

	std::string to_hex_string(const std::string_view input, const HexOption& options = {}) noexcept
	{
		return to_hex_string(std::span{as<u8*>(input.data()), input.size()}, options);
	}

	// epoch
	template<typename T = std::chrono::seconds>
	auto epoch() noexcept
	{
		auto now = std::chrono::system_clock::now();

		if constexpr (std::is_same_v<T, std::chrono::seconds>)
			return std::chrono::duration_cast<T>(now.time_since_epoch()).count() & 0xFFFF'FFFF_u32;
		if constexpr (std::is_same_v<T, std::chrono::milliseconds>)
			return std::chrono::duration_cast<T>(now.time_since_epoch()).count() & 0xFFFF'FFFF'FFFF'FFFF_u64;
	}

	std::string timezone() noexcept
	{
		const auto zone     = std::chrono::current_zone()->get_info(std::chrono::system_clock::now());
		const auto zonename = std::chrono::current_zone()->name();
		return std::format("{} {}", zone.abbrev, zonename);
	}

	std::string time_as_string() noexcept
	{
		auto time = std::chrono::system_clock::now();
		return std::format("{:%T}", std::chrono::current_zone()->to_local(time));
	}

	std::string date_as_string() noexcept
	{
		auto epoch = std::chrono::system_clock::now();
		return std::format("{:%F}", std::chrono::current_zone()->to_local(epoch));
	}

} // namespace deckard
