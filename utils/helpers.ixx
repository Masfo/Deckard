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
	inline constexpr auto upto      = []<std::integral I>(I n) { return std::views::iota(I{}, n); };
	inline constexpr auto upto_from = []<std::integral I>(I s, I n) { return std::views::iota(s, n); };

	//  for(const auto &i: range(0,100,5))
	inline constexpr auto range = []<std::integral I, std::integral U>(I begin, U end, U stepsize = 1)
	{
		const auto boundary = [end](U i) { return i < end; };
		return std::ranges::views::iota(begin) | std::ranges::views::stride(stepsize) | std::ranges::views::take_while(boundary);
	};

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
		if constexpr (requires { from.data(); })
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

	template<typename T, typename... Args>
	std::array<T, sizeof...(Args)> make_array(Args... args)
	{
		return {static_cast<T>(args)...};
	}

	// make_vector
	template<class... Ts>
	constexpr std::vector<typename std::decay<typename std::common_type<Ts...>::type>::type> make_vector(Ts&&... ts)
	{
		return std::vector<typename std::decay<typename std::common_type<Ts...>::type>::type>{std::forward<Ts>(ts)...};
	}

	template<typename T, typename... Args>
	std::vector<T> make_vector(Args... args)
	{
		return {static_cast<T>(args)...};
	}

	// sum
	template<class A, class... Args>
	constexpr A sum(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return a + b;
		else
			return sum(sum(a, b), args...);
	}

	
	// subtract
	template<class A, class... Args>
	constexpr A subtract(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return a - b;
		else
			return subtract(subtract(a, b), args...);
	}

	// product
	template<class A, class... Args>
	constexpr A product(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return a * b;
		else
			return product(product(a, b), args...);
	}



	// min
	template<std::integral A, std::integral... Args>
	constexpr A vmin(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return std::min(a, b);
		else
			return vmin(vmin(a, b), args...);
	}

	// max
	template<std::integral A, std::integral... Args>
	constexpr A vmax(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return std::max(a, b);
		else
			return vmax(vmax(a, b), args...);
	}

	// lcm
	template<std::integral A, std::integral... Args>
	constexpr A vlcm(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return std::lcm(a, b);
		else
			return vlcm(vlcm(a, b), args...);
	}

	// gcd
	template<std::integral A, std::integral... Args>
	constexpr A vgcd(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return std::gcd(a, b);
		else
			return vgcd(vgcd(a, b), args...);
	}

	// push
	auto vpush = [](auto& vec, auto&&... items) { (vec.push_back(std::forward<decltype(items)>(items)), ...); };

	// vsort
	template<sortable... Ts>
	void vsort(Ts&... vs)
	{
		(std::ranges::sort(vs), ...);
	}

	auto try_to_string(const auto input, i32 base = 10) -> std::optional<std::string>
	{
		char buffer[32]{0};


		auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), input, base);
		if (ec != std::errc())
			return {};

		return buffer;
	}

	// concat
	template<std::unsigned_integral T = u64>
	T concat(T x, T y)
	{
		size_t pow{10};
		while (y >= pow)
			pow *= 10;
		return x * pow + y;
	}

	template<std::integral T>
	T concat(T x, T y)
	{
		return as<T>(concat(as<u64>(x), as<u64>(y)));
	}

	template<arithmetic A, arithmetic... Args>
	constexpr A concat(A a, A b, Args... args)
	{
		if constexpr (sizeof...(args) == 0)
			return concat(a, b);
		else
			return concat(concat(a, b), args...);
	}

	// concat/vector
	template<std::ranges::input_range... Rs>
	auto concat(const Rs... rs)
	{

		using TYPE = std::common_type_t<std::decay_t<Rs>...>;
		TYPE ret{};
		ret.reserve((rs.size() + ...));

		(std::ranges::copy(rs.begin(), rs.end(), std::back_inserter(ret)), ...);
		return ret;
	}

	template<typename Type, std::size_t... sizes>
	auto concat(const std::array<Type, sizes>&... arrays)
	{
		std::array<Type, (sizes + ...)> result;
		std::size_t                     index{};

		((std::copy_n(arrays.begin(), sizes, result.begin() + index), index += sizes), ...);

		return result;
	}

	// kcombo
	template<std::ranges::input_range O, std::ranges::input_range T, std::ranges::input_range R>
	void kcombo_util(const O& r, i32 start, i32 count, i32 subindex, T& current, R& ret)
	{
		if (count == 0)
		{
			ret.emplace_back(current);
			return;
		}

		for (auto i = start; i < r.size(); ++i)
		{
			current[subindex] = r[i];
			kcombo_util(r, i + 1, count - 1, subindex + 1, current, ret);
		}
	}

	template<std::ranges::input_range R>
	auto kcombo(const R& r, i32 count = 2)
	{
		using range_type = std::ranges::range_value_t<R>;
		using sub_range  = std::vector<range_type>;

		std::vector<sub_range> ret;
		sub_range              current(count, {});

		kcombo_util(r, 0, count, 0, current, ret);

		return ret;
	}

	template<size_t COUNT, std::ranges::input_range R>
	auto kcombo(const R& r)
	{
		using range_type = std::ranges::range_value_t<R>;
		using sub_range  = std::array<range_type, COUNT>;

		static_assert(COUNT > 1, "kcombo is for two or more items");

		std::vector<sub_range> ret;
		sub_range              current{};

		kcombo_util(r, 0, COUNT, 0, current, ret);

		return ret;
	}

	// isrange
	template<typename T>
	bool isrange(T c, T a, T b)
	{
		return (c >= a) && (c <= b);
	}

	// isdigit
	bool isdigit(char c) { return isrange(c, '0', '9'); }

	// isascii
	bool isascii(char c) { return isrange(c, 'a', 'z') or isrange(c, 'A', 'Z'); }

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

	template<arithmetic T = i32>
	auto to_number(std::string_view input, int [[maybe_unused]] base = 10) -> T
	{
		if (input.empty())
			return T{0};

		if (auto result = try_to_number<T>(input, base); result)
			return *result;


		dbg::println("to_number(\"{}\"): failed to convert to number", input);
		return T{0};
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

	// head, from index
	template<ContainerResize T>
	auto head(const T& container, size_t count)
	{
		if (count >= container.size())
			return container;

		T result{};
		result.resize(count);
		std::ranges::copy_n(container.begin(), count, result.begin());
		return result;
	}

	template<ContainerResize T>
	auto head(const T& container)
	{
		assert::check(container.size() > 0, "Container must have one or more elements");

		return container[0];
	}

	template<size_t COUNT, typename T, size_t S>
	auto head(const std::array<T, S>& container)
	{
		static_assert(COUNT <= S, "Can't head longer than container");


		std::array<T, COUNT> result{};
		std::ranges::copy_n(container.begin(), COUNT, result.begin());
		return result;
	}

	template<size_t COUNT, ContainerResize T>
	auto head(const T& container)
	{
		assert::check(container.size() > 0, "Container must have 1 or more elements");
		return container[0];
	}

	// tail, from index
	template<ContainerResize T>
	auto tail(const T& container, size_t count = 1)
	{
		if (count > container.size() or count - container.size() == 0)
		{
			return T{};
		}

		if (count == 0)
			return container;

		T result{};
		result.resize(container.size() - count);
		std::ranges::copy_n(container.begin() + count, result.size(), result.begin());
		return result;
	}

	// tail-array
	template<size_t I, typename T, size_t S>
	auto tail(const std::array<T, S>& container)
	{
		static_assert(I <= S, "Can't tail longer than container");


		std::array<T, S - I> result{};
		std::ranges::copy_n(container.begin() + I, S - I, result.begin());
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

	template<arithmetic T>
	constexpr T pow10(T n)
	{
		T result = 1;
		for (size_t i = 1; i <= n; ++i)
			result *= 10;
		return result;
	}

	// odd
	struct even_fn
	{
		template<arithmetic T>
		auto operator()(T&& v) const -> bool
		{
			return (v & 1) == 0;
		}

		template<arithmetic T>
		friend auto operator|(T&& v, even_fn fun)
		{
			return fun(std::forward<T>(v));
		}
	};

	constexpr even_fn is_even;

	struct odd_fn
	{
		template<arithmetic T>
		auto operator()(T&& v) const -> bool
		{
			return (v & 1) != 0;
		}

		template<arithmetic T>
		friend auto operator|(T&& v, odd_fn fun)
		{
			return fun(std::forward<T>(v));
		}
	};

	constexpr odd_fn is_odd;

	// count_digits, only positives, takes abs
	template<std::unsigned_integral T>
	constexpr size_t count_digits(T v)
	{
		if (v == 0)
			return 1;
		return static_cast<size_t>(std::log10(v) + 1);
	}

	template<std::signed_integral T>
	constexpr size_t count_digits(T v)
	{
		return count_digits(as<u64>(v < 0 ? -v : v));
	}

	template<arithmetic T>
	constexpr auto split_digit(T v) -> std::pair<T, T>
	{
		const u32 digit_count = count_digits(v);
		assert::check(digit_count > 1, "Cannot split single digit");

		auto divisor = as<T>(pow10(digit_count / 2));

		T r1 = static_cast<T>(v / divisor);
		T r2 = static_cast<T>(v - r1 * divisor);
		return std::make_pair(r1, r2 < 0 ? -r2 : r2);
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

		add_unit(1024 * 1_TiB, "PiB");
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
