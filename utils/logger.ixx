export module deckard.logger;

import std;
import deckard.types;
import deckard.as;
import deckard.debug;
import deckard.file;
import deckard.helpers;
namespace fs = std::filesystem;

namespace deckard
{

	constexpr u64 BUFFER_LEN = 2048;

	class logger final
	{
	private:
		std::vector<u8>  buffer;
		std::atomic<u64> index;
		std::mutex       flush_mutex;
		std::mutex       push_mutex;

		std::atomic<u64> total_buffer_size;

		fs::path logfile;

		void clean_older_logfiles(u32 count = 4)
		{
			fs::path                                             currentpath = fs::current_path();
			std::vector<std::pair<fs::path, fs::file_time_type>> logfiles;
			logfiles.reserve(count * 2);

			for (const auto& file : fs::directory_iterator(currentpath))
			{
				if (not file.is_regular_file() and file.path().extension() != "txt")
					continue;

				std::string filename = file.path().stem().string();

				if (filename.starts_with("log."))
				{
					logfiles.emplace_back(file, fs::last_write_time(file.path()));
				}
			}

			if (logfiles.size() <= count)
				return;

			std::ranges::sort(logfiles, [](const auto& a, const auto& b) { return a.second > b.second; });

			for (const auto& [path, time] : logfiles | std::views::drop(count))
				fs::remove(path);
		}

		void save()
		{
			push("\n\nDeckard Log closing at {} {}", day_month_year(), hour_minute_second());

			flush();
		}

		void push(std::string_view str)
		{

#ifdef _DEBUG
			dbg::println("{}", str);
#endif
			std::scoped_lock l(push_mutex);

			u64 needed    = str.size() + 1; // message + newline

			u64 old_index = index.fetch_add(needed, std::memory_order_relaxed);
			u64 new_index = old_index + needed;

			if (new_index > BUFFER_LEN)
			{
				index.fetch_sub(needed, std::memory_order_relaxed);

				flush_internal();

				old_index = index.fetch_add(needed, std::memory_order_relaxed);
				new_index = old_index + needed;

				if (new_index > BUFFER_LEN)
				{
					index.fetch_sub(needed, std::memory_order_relaxed);

					// Message too large - truncate to fit buffer
					u64 available = BUFFER_LEN - 1; // Reserve 1 byte for newline
					std::memcpy(&buffer[0], str.data(), available);
					buffer[available] = '\n';
					index.store(available + 1, std::memory_order_release);
					total_buffer_size += available + 1;

					flush_internal();
					return;
				}
			}

			// Write message + newline to buffer
			std::memcpy(&buffer[old_index], str.data(), str.size());
			old_index += str.size();
			buffer[old_index] = '\n';

			total_buffer_size += needed;
		}

		template<typename... Args>
		void push(std::string_view fmt, Args&&... args)
		{
			push(std::vformat(fmt, std::make_format_args(args...)));
		}

		void initialize() { push("Deckard Log initialized at {} {}\n\n", day_month_year(), hour_minute_second()); }

		auto gen_log_filename() const
		{
			return fs::current_path() / std::format("log.{}.{}.txt", day_month_year(), hour_minute_second("."));
		}

	public:
		logger()
			: logger(BUFFER_LEN)
		{
		}

		logger(u64 len)
		{
			index = 0;
			buffer.resize(len);

			logfile = gen_log_filename();

			clean_older_logfiles();
			initialize();
		}

		~logger() { save(); }

		void flush()
		{
			std::scoped_lock l(flush_mutex);
			flush_internal();
		}

		void flush_internal()
		{
			u64 current_size = index.exchange(0, std::memory_order_acquire);

		if (current_size == 0)
			return;

		std::span<u8> log_span(as<u8*>(buffer.data()), current_size);
		(void)file::append({.file = logfile, .buffer = log_span});
	}

		template<typename... Args>
		void operator()(std::string_view fmt, Args&&... args)
		{
			push(fmt, args...);
		}

		u64 size() const { return total_buffer_size.load(); }


		// TODO
		// std::generator<std::string_view> getline()
	};

	export logger logger;

	export template<typename... Args>
	void log(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(args) > 0)
			logger("{}", std::vformat(fmt, std::make_format_args(args...)));
		else
			logger("{}", fmt);
	}

	export template<typename... Args>
	void info(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(args) > 0)
			logger("INFO: {}", std::vformat(fmt, std::make_format_args(args...)));
		else
			logger("INFO: {}", fmt);
	}

	export template<typename... Args>
	void debug(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(args) > 0)
			logger("DEBUG: {}", std::vformat(fmt, std::make_format_args(args...)));
		else
			logger("DEBUG: {}", fmt);
	}

	export template<typename... Args>
	void warn(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(args) > 0)
			logger("WARN: {}", std::vformat(fmt, std::make_format_args(args...)));
		else
			logger("WARN: {}", fmt);
	}

	export template<typename... Args>
	void warning(std::string_view fmt, Args&&... args)
	{
		warn(fmt, std::forward<Args>(args)...);
	}

	export template<typename... Args>
	void error(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(args) > 0)
			logger("ERROR: {}", std::vformat(fmt, std::make_format_args(args...)));
		else
			logger("ERROR: {}", fmt);
	}

	export template<typename... Args>
	void fatal(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(args) > 0)
			logger("FATAL: {}", std::vformat(fmt, std::make_format_args(args...)));
		else
			logger("FATAL: {}", fmt);
	}

	export template<typename... Args>
	void trace(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(args) > 0)
			logger("TRACE: {}", std::vformat(fmt, std::make_format_args(args...)));
		else
			logger("TRACE: {}", fmt);
	}

	export void flush() { logger.flush(); }


} // namespace deckard

