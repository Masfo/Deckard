export module deckard.timers;

import std;
import deckard.types;
import deckard.helpers;
import deckard.debug;

namespace deckard
{
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

		void stop()
		{
			now();
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
