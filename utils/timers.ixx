export module deckard.timers;

import std;
import deckard.types;
import deckard.helpers;
import deckard.debug;

namespace deckard
{
	using namespace std::chrono_literals;

	constexpr f32 MAX_DELTA_TIME = std::chrono::duration<f32>(100ms).count();
	// fixed timer

	template<typename T>
	concept enum_with_count = std::is_scoped_enum_v<T> and requires {
		{ std::to_underlying(T::count) } -> std::convertible_to<size_t>;
	};

	template<enum_with_count T>
	constexpr size_t enum_count_v = static_cast<size_t>(std::to_underlying(T::count));

	export template<enum_with_count T, std::array<f32, enum_count_v<T>> Intervals>
	class fixed_timers final
	{
		static_assert(std::is_scoped_enum_v<T>, "T must be a scoped enum");
		static_assert(std::ranges::all_of(Intervals, [](f32 v) { return v > 0.0f; }), "All intervals must be > 0");

	private:
		std::array<f32, enum_count_v<T>> timers{};
		std::array<u32, enum_count_v<T>> iterations{};
		u32                              max_iterations;

	public:
		explicit constexpr fixed_timers(u32 max_iterations = 5) noexcept
			: max_iterations{max_iterations}
		{
		}

		[[nodiscard]] constexpr f32& operator[](T index) noexcept { return timers[std::to_underlying(index)]; }

		[[nodiscard]] constexpr f32 operator[](T index) const noexcept { return timers[std::to_underlying(index)]; }

		constexpr void update(T index, f32 dt) noexcept
		{
			timers[std::to_underlying(index)] += dt;
			iterations[std::to_underlying(index)] = 0;
		}

		constexpr void update(f32 dt) noexcept
		{
			for (auto& t : timers)
				t += dt;
			iterations.fill(0);
		}

		[[nodiscard]] constexpr bool tick(T index) noexcept
		{
			const auto i        = std::to_underlying(index);
			auto&      t        = timers[i];
			auto&      count    = iterations[i];
			const f32  interval = Intervals[i];

			if (t >= interval and count < max_iterations)
			{
				t -= interval;
				++count;
				return true;
			}
			return false;
		}

		constexpr void reset(T index) noexcept
		{
			timers[std::to_underlying(index)]     = 0.0f;
			iterations[std::to_underlying(index)] = 0;
		}
	};

	export class fixed_step
	{
	private:
		f32 interval{0.0f};
		f32 accumulator{0.0f};
		u64 ticks{0};

	public:
		explicit fixed_step(f32 hz)
			: interval(1.0f / hz)
		{
		}

		void advance(f32 dt) noexcept
		{
			if (dt > 0.0f)
				accumulator += dt;
		}

		bool tick() noexcept
		{

			if (accumulator >= interval)
			{
				accumulator -= interval;
				ticks++;
				return true;
			}
			return false;
		}

		// interpolation for rendering, range 0.0 - 1.0
		[[nodiscard]] f32 alpha() const noexcept { return accumulator / interval; }

		[[nodiscard]] f32 delta() const noexcept { return interval; }

		u64 ticks_count() const noexcept { return ticks; }
	};

	// Frame timers
	using namespace std::chrono_literals;
	const f32 MAX_DELTA_TIME = std::chrono::duration<f32>(100ms).count();

	export class frame_timer
	{
	private:

		using clock = std::chrono::steady_clock;
		clock::time_point last_time{clock::now()};

	public:
		frame_timer() = default;

		void reset() { last_time = clock::now(); }


		f32 tick() 
		{
			const auto now = clock::now();
			const f32  dt  = std::chrono::duration<f32>(now - last_time).count();
			last_time      = now;
			return std::min(dt, MAX_DELTA_TIME);
		}

		f32 fps() { return 1.0f / tick(); }

	};


	// ScopeTimer
	export template<typename R = std::milli>
	class ScopeTimer
	{
	public:
		ScopeTimer()
			: name("Unknown timer")
		{
		}

		ScopeTimer(std::string_view scopename)
		{
			name = scopename;
			start();
		}

		~ScopeTimer()
		{
			if (not stopped)
				now();
		}

		void reset() { start(); };

		void start() { start_time = clock_now(); }

		void stop(std::string_view input="")
		{
			now(input);
			stopped = true;
		}

		void now(std::string_view input = "")
		{
			if (input.empty())
				dbg::println("{} took {}", name, duration());
			else
				dbg::println("{}/{} took {}", name, input, duration());
		}

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

	export class AverageTimer
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
}
