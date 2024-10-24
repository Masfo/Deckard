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


	template<typename T>
	concept HasDataMember = requires(T obj) { obj.data(); };

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

	//


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

	// strip - range
	std::string strip(std::string_view str, char a, char z)
	{
		std::string ret;
		ret.reserve(str.size());
		for (const char& c : str)
			if (c < a || c > z)
				ret += c;

		return ret;
	}

	// strip -
	std::string strip(std::string_view str, std::string_view strip_chars)
	{
		std::string ret;
		ret.reserve(str.size());

		for (auto& c : str)
			if (!strip_chars.contains(c))
				ret += c;

		return ret;
	}

	// replace
	std::string replace(std::string_view subject, const std::string_view search, std::string_view replace)
	{
		std::string output(subject);
		size_t      pos = 0;
		while ((pos = output.find(search, pos)) != std::string::npos)
		{
			output.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return output;
	}

	// split
	std::vector<std::string_view> split(std::string_view strv, std::string_view delims = " ")
	{
		std::vector<std::string_view> output;
		size_t                        first = 0;

		while (first < strv.size())
		{
			const auto second = strv.find_first_of(delims, first);

			if (first != second)
				output.emplace_back(strv.substr(first, second - first));

			if (second == std::string_view::npos)
				break;

			first = second + 1;
		}

		return output;
	}

	// split_once
	auto split_once(std::string_view str, std::string_view delims = " ")
	{
		using namespace std::string_literals;
		size_t pos = str.find(delims);
		if (pos != std::string::npos)
		{
			return make_array(str.substr(0, pos), str.substr(pos + 1));
		}
		else
		{
			return make_array(str, ""s);
		}
	}

	// split_exact
	std::vector<std::string_view> split_exact(std::string_view str, std::string_view delims, bool include_empty = false)
	{
		std::vector<std::string_view> output;
		u64                           first = 0;
		while (first < str.size())
		{
			const auto second = str.find(delims, first);

			if (include_empty || first != second) // empty line
				output.emplace_back(str.substr(first, second - first));

			if (second == std::string_view::npos)
				break;
			first = second + delims.size();
		}
		return output;
	}

	std::vector<std::string_view> split_exact(std::string_view str, u64 length)
	{
		std::vector<std::string_view> v;
		v.reserve(2);
		if (length <= str.length())
		{
			v.emplace_back(str.substr(0, length));
			v.emplace_back(str.substr(length));
		}
		return v;
	}

	std::vector<std::string_view> split_stride(std::string_view str, u64 stride)
	{
		std::vector<std::string_view> v;

		for (u64 i = 0; i <= str.length(); i += stride)
		{
			if (i < str.length())
				v.emplace_back(str.substr(i, stride));
		}
		return v;
	}

	// trim
	std::string_view trim_front(std::string_view s)
	{
		if (s.empty())
			return s;

		s.remove_prefix(s.find_first_not_of(whitespace_string));
		return s;
	}

	std::string_view trim_back(std::string_view s)
	{
		if (s.empty())
			return s;

		s.remove_suffix(s.size() - s.find_last_not_of(whitespace_string) - 1);
		return s;
	}

	std::string_view trim(std::string_view s)
	{
		s = trim_front(s);
		return trim_back(s);
	};

	template<typename T = i32>
	auto try_to_number(std::string_view input, int [[maybe_unused]] base = 10) -> std::optional<T>
	{
		if (input.empty())
			return {};

		if (input.starts_with('+'))
			input.remove_prefix(1);


		T val{};

		if constexpr (std::is_floating_point_v<T>)
		{
			auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), val);
			if (ptr == input.data())
			{
				dbg::println("try_to_number<float> failed: '{}'", input);
				return {};
			}

			return val;
		}

		if constexpr (std::is_integral_v<T>)
		{
			if (input.starts_with('#'))
			{
				input.remove_prefix(1);
				auto [ptr, ec]{std::from_chars(input.data(), input.data() + input.size(), val, 16)};
				if (ptr == input.data())
				{
					dbg::trace("try_to_number(\"{}\", base({})). Is not a hex number", input, 16);
					return {};
				}

				return val;
			}

			auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), val, base);
			if (ptr == input.data())
			{
				#ifdef _DEBUG
				dbg::trace("try_to_number failed: '{}'", input);
				dbg::stacktrace();
				#endif

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

		void start() { start_time = clock_now(); }

		void stop()
		{
			now();
			stopped = true;
		}

		void now() { dbg::println("{} took {}", name, duration()); }

		auto duration()
		{
			std::chrono::duration<float, R> dur(clock_now() - start_time);
			return dur;
		}

	private:
		std::string                           name;
		std::chrono::steady_clock::time_point start_time{};
		bool                                  stopped{false};
	};

	//

	class AverageTimer
	{
	private:
		using Type = i64;
		using R    = std::nano;
		u64                                   m_iterations{0};
		std::chrono::duration<Type, R>        m_total_dur{0};
		std::chrono::steady_clock::time_point start_time{};
		bool                                  has_dumped{false};

	public:
		AverageTimer() = default;

		// Copy
		AverageTimer(AverageTimer const&)            = delete;
		AverageTimer& operator=(AverageTimer const&) = delete;
		// Move
		AverageTimer(AverageTimer&&)            = delete;
		AverageTimer& operator=(AverageTimer&&) = delete;

		~AverageTimer()
		{
			if (has_dumped == false)
				dbg::println("{}", dump());
		}

		void begin() { start_time = clock_now(); }

		void end()
		{
			m_total_dur += clock_now() - start_time;
			m_iterations += 1;
		}

		auto total() const { return m_total_dur; }

		auto average() const { return std::chrono::duration<Type, R>(m_total_dur / m_iterations); }

		u64 iterations() const { return m_iterations; }

		void clear()
		{
			m_iterations = 0;
			has_dumped   = false;
		}

		std::string dump()
		{
			has_dumped = true;
			return std::format(
			  "Total time: {}, Iterations: {}, Average: {}", pretty_time(m_total_dur), m_iterations, pretty_time(average()));
		}
	};
} // namespace deckard
