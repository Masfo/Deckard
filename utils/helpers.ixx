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
		{
			std::memcpy(&ret, from.data(), sizeof(To));
		}
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

	// hash_combine

	template<typename T>
	T xorshift(const T& n, int i)
	{
		return n ^ (n >> i);
	}

	u32 distribute(const u32& n)
	{
		u32 p = 0x5555'5555ul;
		u32 c = 3'423'571'495ul;
		return c * xorshift(p * xorshift(n, 16), 16);
	}

	u64 distribute(const u64& n)
	{
		u64 p = 0x5555'5555'5555'5555ull;
		u64 c = 17'316'035'218'449'499'591ull;
		return c * xorshift(p * xorshift(n, 32), 32);
	}

	template<typename T, typename... Rest>
	inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
	{
		seed = std::rotl(seed, std::numeric_limits<size_t>::digits / 3) ^ distribute(std::hash<T>{}(v));
		(hash_combine(seed, rest), ...);
	}

	template<typename... Types>
	std::size_t hash_values(const Types&... args)
	{
		std::size_t seed = 0;
		hash_combine(seed, args...);
		return seed;
	}

	// to_hex

	struct HexOption
	{
		bool        byteswap{std::endian::native == std::endian::little};
		std::string delimiter{", "};
		bool        lowercase{false};
		bool        show_hex{true};
	};

	template<typename T = u8>
	u32 to_hex(const std::span<T> input, std::span<u8> output, const HexOption& options = {}) noexcept
	{
		const u32 delimiter_len = input.size() == 1 ? 0 : as<u32>(options.delimiter.size());
		const u32 hex_len       = options.show_hex ? 2 : 0;

		const u32 stride = (sizeof(T) * 2) + delimiter_len + hex_len;
		const u32 maxlen = (as<u32>(input.size()) * stride) - delimiter_len;

		if (input.empty() or output.size() < maxlen or output.empty())
			return maxlen;

		constexpr static std::array<u8, 32> HEX_LUT{
		  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
		const i32 lowercase_offset = options.lowercase ? 16 : 0;

		const bool show_hex = options.show_hex;
		u32        len      = 0;

		for (const auto [i, word] : std::views::enumerate(input))
		{
			const T input_word = options.byteswap ? std::byteswap(word) : word;

			u32 offset = 0;
			if (show_hex)
			{
				output[i * stride + offset] = '0';
				offset += 1;

				output[i * stride + offset] = 'x';
				offset += 1;
				len += 2;
			}

			for (u8 byte = 0; byte < sizeof(T); byte++)
			{
				const u8 shift              = byte * 8;
				const T  mask               = sizeof(T) == 1 ? 0xFF : as<T>(0xFF) << shift;
				const T  masked_byte        = (input_word & mask) >> shift;
				output[i * stride + offset] = HEX_LUT[(as<u8>(masked_byte) >> 4) + lowercase_offset];
				offset += 1;
				output[i * stride + offset] = HEX_LUT[(as<u8>(masked_byte) & 0xF) + lowercase_offset];
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

		(void)to_hex<T>(input, {reinterpret_cast<u8*>(ret.data()), ret.size()}, options);

		return ret;
	}

	std::string to_hex_string(const std::string_view input, const HexOption& options = {}) noexcept
	{
		//
		return to_hex_string(std::span{input.data(), input.size()}, options);
	}

} // namespace deckard
