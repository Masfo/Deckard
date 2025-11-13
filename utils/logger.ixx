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

	constexpr u64 BUFFER_LEN = 2_MiB;

	class logger final
	{
	private:
		std::vector<u8>  buffer;
		std::atomic<u64> index;

		std::atomic<u64> linecount;

		void clean_older_logfiles(u32 count = 4)
		{
			fs::path                                             currentpath = fs::current_path();
			std::vector<std::pair<fs::path, fs::file_time_type>> logfiles;
			logfiles.reserve(count*2);

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
			push("\n\n");
			push("Deckard Log closing at {} {}", day_month_year(), hour_minute_second());

			std::string new_logfile = std::format("log.{}.{}.txt", day_month_year(), hour_minute_second("."));

			(void)file::write(new_logfile, view());
		}

		void push(std::string_view str)
		{
			u64 needed = str.size() + 1;
			u64 idx    = index.fetch_add(needed, std::memory_order_acq_rel);

			if (index + needed > buffer.size())
				return;

			std::memcpy(buffer.data() + idx, str.data(), str.size());
			buffer[idx + str.size()] = '\n';
			linecount.fetch_add(1);
		}

		template<typename... Args>
		void push(std::string_view fmt, Args&&... args)
		{
			push(std::vformat(fmt, std::make_format_args(args...)));
		}

		void initialize()
		{
			push("Deckard Log initialized at {} {}", day_month_year(), hour_minute_second());
			push("\n\n");
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

			clean_older_logfiles();
			initialize();
		}

		~logger()
		{
			//
			save();
		}

		u64 remaining() const
		{
			u64 total = buffer.size();
			u64 used  = index.load(std::memory_order_acquire);
			return used >= total ? 0 : total - used;
		}

		u64 size() const { return buffer.size(); }

		template<typename... Args>
		void operator()(std::string_view fmt, Args&&... args)
		{
			push(fmt, args...);
		}

		u64 line_count() const { return linecount.load(std::memory_order_acquire); }

		std::string_view view() const
		{
			size_t n = index.load(std::memory_order_acquire);
			if (n > buffer.size())
				n = buffer.size();
			return std::string_view(as<const char*>(buffer.data()), n);
		}

		// TODO
		// std::generator<std::string_view> getline()
	};

	export logger logger;

	export template<typename... Args>
	void info(std::string_view fmt, Args&&... args)
	{
		if constexpr (sizeof...(args) > 0)
			logger("INFO: {}", std::vformat(fmt, std::make_format_args(args...)));
		else
			logger("INFO: {}", fmt);
	}


} // namespace deckard
