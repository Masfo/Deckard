export module deckard.helpers;

import std;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.debug;

using namespace std::string_view_literals;

export namespace deckard
{
	constexpr std::string_view whitespace_string{" \t\f\n\r\v"};


	// upto 0..n-1, P3060
	inline constexpr auto upto = []<std::integral I>(I n) { return std::views::iota(I{}, n); };


	// loop (n, n+1, n+..)
	inline constexpr auto loop = []<std::integral I>(I start = 0) { return std::views::iota(start); };

	// repeat
	template<size_t Count>
	struct repeat_t final
	{
		template<std::invocable Func>
		constexpr void operator=(Func&& f) const
		{
			size_t count = Count;
			while (count--)
				f();
		}
	};

	template<size_t Count>
	inline constexpr repeat_t<Count> repeat;



	export template<typename To, typename From>
	To load_as(const From from)
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
	To load_as_be(const From from)
	{
		return std::byteswap(load_as<To>(from));
	}

	export template<typename To, typename From>
	To load_as_le(const From from)
	{
		return load_as<To>(from);
	}



	template<typename T = u8>
	constexpr T bitmask(u8 size, u8 offset = 0)
	{
		return ((1 << size) - 1) << offset;
	}

	constexpr bool test_bit(u64 value, u32 bitindex) { return ((value >> bitindex) & 1) ? true : false; }

	auto clock_now() { return std::chrono::steady_clock::now(); }

	void clock_stop(std::string_view id, std::chrono::steady_clock::time_point start)
	{
		std::chrono::duration<double, std::milli> took(clock_now() - start);
		std::println("{} took {}", id, took);
	}

	// to_hex
	struct HexOption
	{
		std::string delimiter{", "};
		bool        endian_swap{std::endian::native == std::endian::little};
		bool        lowercase{false};
		bool        show_hex{true};
	};

	template<typename T>
	u64 to_hex(const std::span<T> input, std::span<u8> output, const HexOption& options = {})
	{
		u64 len{0};


		const u64 delimiter_len = input.size() == 1 ? 0 : as<u64>(options.delimiter.size());
		const u64 hex_len       = options.show_hex ? 2 : 0;

		const u64 stride = (sizeof(T) * 2) + delimiter_len + hex_len;
		const u64 maxlen = (as<u64>(input.size()) * stride) - delimiter_len;

		if (output.empty())
			return maxlen;

		if (input.empty())
			return 0;

		constexpr static std::array<u8, 32> HEX_LUT{
		  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
		const i32 lowercase_offset = options.lowercase ? 16 : 0;

		const bool show_hex = options.show_hex;

		auto output_byte = [&](size_t i, u32 offset, char b) -> bool
		{
			u32 index = as<u32>(i * stride + offset);
			if (index >= output.size())
				return false;

			output[index] = b;
			len += 1;
			return true;
		};

		// TODO: output width( ie, how often newlines)

		for (const auto [i, word] : std::views::enumerate(input))
		{

			const T input_word = options.endian_swap ? std::byteswap(word) : word;

			u32 offset = 0;

			if (show_hex)
			{
				if (not output_byte(i, offset++, '0'))
					break;
				if (not output_byte(i, offset++, 'x'))
					break;
			}

			for (u8 byteindex = 0; byteindex < sizeof(T); byteindex++)
			{
				const T  shift       = byteindex * 8;
				const T  mask        = sizeof(T) == 1 ? 0xFF : as<T>(0xFF_u8) << shift;
				const u8 masked_byte = as<u8>((input_word & mask) >> shift);

				if (not output_byte(i, offset++, HEX_LUT[((masked_byte) >> 4) + lowercase_offset]))
					break;
				if (not output_byte(i, offset++, HEX_LUT[((masked_byte) & 0xF) + lowercase_offset]))
					break;
			}


			for (size_t j = 0; (as<u64>(i) < input.size() - 1) and (j < options.delimiter.size()); j++)
			{
				if (not output_byte(i, offset++, options.delimiter[j]))
					break;
			}
		}


		return len;
	}

	// TODO: limit output size
	template<typename T>
	std::string to_hex_string(const std::span<T> input, const HexOption& options = {})
	{
		std::string ret;

		auto len = to_hex<T>(input, {}, options);
		ret.resize(len);

		(void)to_hex<T>(input, {as<u8*>(ret.data()), ret.size()}, options);

		return ret;
	}

	std::string to_hex_string(const std::string_view input, const HexOption& options = {})
	{
		return to_hex_string(std::span{as<u8*>(input.data()), input.size()}, options);
	}

	std::string to_hex_string(const std::string_view input, size_t len, const HexOption& options = {})
	{
		return to_hex_string(std::span{as<u8*>(input.data()), len}, options);
	}

	// epoch
	template<typename T = std::chrono::seconds>
	auto epoch()
	{
		auto now = std::chrono::system_clock::now();

		if constexpr (std::is_same_v<T, std::chrono::seconds>)
			return std::chrono::duration_cast<T>(now.time_since_epoch()).count() & 0xFFFF'FFFF_u32;
		if constexpr (std::is_same_v<T, std::chrono::milliseconds>)
			return std::chrono::duration_cast<T>(now.time_since_epoch()).count() & 0xFFFF'FFFF'FFFF'FFFF_u64;
	}

	std::string timezone_as_string()
	{
		const auto zone     = std::chrono::current_zone()->get_info(std::chrono::system_clock::now());
		const auto zonename = std::chrono::current_zone()->name();
		return std::format("{} {}", zone.abbrev, zonename);
	}

	std::string current_time_as_string()
	{
		auto time = std::chrono::system_clock::now();
		return std::format("{:%T}", std::chrono::current_zone()->to_local(time));
	}

	std::string current_date_as_string()
	{
		auto epoch = std::chrono::system_clock::now();
		return std::format("{:%F}", std::chrono::current_zone()->to_local(epoch));
	}

	template<typename T, typename U>
	bool contains(const T& container, const U& element)
	{
		return std::ranges::contains(container, element);
	}

	// make_array
	template<class... Ts>
	constexpr std::array<typename std::decay<typename std::common_type<Ts...>::type>::type, sizeof...(Ts)> make_array(Ts&&... ts)
	{
		return std::array<typename std::decay<typename std::common_type<Ts...>::type>::type, sizeof...(Ts)>{std::forward<Ts>(ts)...};
	}

	// make_vector
	template<class... Ts>
	constexpr std::vector<typename std::decay<typename std::common_type<Ts...>::type>::type> make_vector(Ts&&... ts)
	{
		return std::vector<typename std::decay<typename std::common_type<Ts...>::type>::type>{std::forward<Ts>(ts)...};
	}

	// vmin
	template<class A, class... Args>
	constexpr A vmin(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return std::min(a, b);
		else
			return vmin(vmin(a, b), args...);
	}

	// vmax
	template<class A, class... Args>
	constexpr A vmax(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return std::max(a, b);
		else
			return vmax(vmax(a, b), args...);
	}

	// vlcm
	template<class A, class... Args>
	constexpr A vlcm(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return std::lcm(a, b);
		else
			return vlcm(vlcm(a, b), args...);
	}

	// vpush
	auto vpush = [](auto& vec, auto&&... items) { (vec.push_back(std::forward<decltype(items)>(items)), ...); };

	auto try_to_string(const auto input, i32 base = 10) -> std::optional<std::string>
	{
		char buffer[32]{0};


		auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), input, base);
		if (ec != std::errc())
			return {};

		return buffer;
	}

	// isrange
	template<typename T>
	bool isrange(T c, T a, T b) 
	{
		return (c >= a) && (c <= b);
	}

	// isdigit
	bool isdigit(char c) 
	{
		return isrange(c, '0', '9');
	}

	// isascii
	bool isascii(char c) 
	{
		return isrange(c, 'a', 'z') or isrange(c, 'A', 'Z');
	}

	template<arithmetic T = i32>
	auto try_to_number(std::string_view input, int [[maybe_unused]] base = 10) -> std::optional<T>
	{
		if (input.empty())
			return {};

		if (input.starts_with('+'))
			input.remove_prefix(1);

		if (input.starts_with("0x"))
		{
			input.remove_prefix(2);
			base = 16;
		}
		else if (input.starts_with("0b"))
		{
			input.remove_prefix(2);
			base = 2;
		}
		else if (input.starts_with('#'))
		{
			input.remove_prefix(1);
			base = 16;
		}


		T val{};

		if constexpr (std::is_floating_point_v<T>)
		{
			auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), val);
			if (ec != std::errc())
			{
				dbg::trace("try_to_number<float>(\"{}\"). Failed to convert", input, base);

				return {};
			}

			return val;
		}

		if constexpr (std::is_integral_v<T>)
		{
			auto [ptr, ec]{std::from_chars(input.data(), input.data() + input.size(), val, base)};
			if (ec == std::errc::result_out_of_range)
			{
				dbg::trace("try_to_number(\"{}\", base({})). Out of range", input, base);
				return {};
			}
			else if (ec != std::errc())
			{
				dbg::trace("try_to_number(\"{}\", base({})). Failed to convert", input, base);
				return {};
			}

			return val;
		}
	}

	template<typename T>
	concept ContainerResize = requires(T a) {
		a.size();
		a.resize(0);
		a.begin();
		a.rbegin();
	};

	// move index
	template<basic_container T>
	void move_index(T& v, size_t oldIndex, size_t newIndex)
	{
		if (oldIndex > v.size() || newIndex > v.size() || oldIndex == newIndex)
			return;

		if (oldIndex > newIndex)
			std::rotate(v.rend() - oldIndex - 1, v.rend() - oldIndex, v.rend() - newIndex);
		else
			std::rotate(v.begin() + oldIndex, v.begin() + oldIndex + 1, v.begin() + newIndex + 1);
	}

	// index_of
	template<non_string_container T, typename U>
	auto try_index_of(const T& v, U find) -> std::optional<u64>
	{
		auto result = std::ranges::find(v, find);
		if (result == v.end())
			return {};

		return std::ranges::distance(v.begin(), result);
	}

	auto try_index_of(std::string_view input, std::string_view substr) -> std::optional<u64>
	{
		auto found = input.find(substr);
		if (found == std::string_view::npos)
			return {};

		return found;
	}

	// take n elements
	template<ContainerResize T>
	auto take(const T& container, size_t count)
	{
		assert::check(count <= container.size(), "Count is larger than the container");

		if (count == container.size())
			return container;

		T result{};
		result.resize(count);
		std::ranges::copy_n(container.begin(), count, result.begin());
		return result;
	}

	template<size_t RSize, typename T, size_t S>
	auto take(const std::array<T, S>& container)
	{
		assert::check(RSize <= container.size(), "Count is larger than the container");


		std::array<T, RSize> result{};
		std::ranges::copy_n(container.begin(), RSize, result.begin());
		return result;
	}

	// last n elements
	template<ContainerResize T>
	auto last(const T& container, size_t count)
	{
		assert::check(count <= container.size(), "Count is larger than the container");

		if (count == container.size())
			return container;

		T result{};
		result.resize(count);
		std::ranges::copy_n(container.rbegin(), count, result.rbegin());
		return result;
	}

	// last array
	template<size_t RSize, typename T, size_t S>
	auto last(const std::array<T, S>& container)
	{
		assert::check(RSize <= container.size(), "Count is larger than the container");


		std::array<T, RSize> result{};
		std::ranges::copy_n(container.rbegin(), RSize, result.rbegin());
		return result;
	}

	// Prettys
	std::string human_readable_bytes(u64 bytes)
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

	std::string pretty_bytes(u64 bytes)
	{

		std::string ret;
		ret.reserve(64);

		auto add_unit = [&bytes, &ret](u64 unit, std::string_view unitstr)
		{
			if (unit == 0)
			{
				ret += std::format("{}{} byte{}", (ret.empty() ? "" : ", "), bytes, bytes > 1 ? "s" : "");
				return;
			}

			u64 count = 0;
			while (bytes >= unit)
			{
				bytes -= unit;
				count += 1;
			}

			if (count > 0)
				ret += std::format("{}{} {}", (ret.empty() ? "" : ", "), count, unitstr);
		};

		add_unit(1'024 * 1_TiB, "PiB");
		add_unit(1_TiB, "TiB");
		add_unit(1_GiB, "GiB");
		add_unit(1_MiB, "MiB");
		add_unit(1_KiB, "KiB");
		add_unit(0, "bytes");

		return ret;
	}

	template<typename T, typename R>
	std::string pretty_time(std::chrono::duration<T, R> duration)
	{

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
		duration -= milliseconds;

		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(milliseconds);
		milliseconds -= seconds;

		auto minutes = std::chrono::duration_cast<std::chrono::minutes>(seconds);
		seconds -= minutes;

		auto hours = std::chrono::duration_cast<std::chrono::hours>(minutes);
		minutes -= hours;

		auto days = std::chrono::duration_cast<std::chrono::days>(hours);
		hours -= days;

		auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
		duration -= microseconds;


		std::string result;
		result.reserve(64);


		if (days.count() > 0)
			result += std::format("{} ", days);
		if (hours.count() > 0)
			result += std::format("{} ", hours);
		if (minutes.count() > 0)
			result += std::format("{} ", minutes);
		if (seconds.count() > 0)
			result += std::format("{} ", seconds);
		if (milliseconds.count() > 0)
			result += std::format("{} ", milliseconds);
		if (microseconds.count() > 0)
			result += std::format("{} ", microseconds);
		if (duration.count() > 0)
			result += std::format("{}", duration);


		if (result.back() == ' ')
			result.resize(result.size() - 1);
		result.shrink_to_fit();

		return result;
	}


} // namespace deckard
