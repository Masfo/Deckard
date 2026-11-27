module;
#include <Windows.h>

export module deckard.log;
import deckard.types;
import std;

namespace deckard::log
{
	template<typename T>
	concept StringViewSource = requires(const T& text)
	{
		{ text.as_string_view() } -> std::same_as<std::string_view>;
	};

/*
*	█    █   ██████   █        █████    ██████   █████     ████
*	█    █   █        █        █    █   █        █    █   █
*	██████   ████     █        █████    ████     █████     ████
*	█    █   █        █        █        █        █   █         █
*	█    █   ██████   ██████   █        ██████   █    █    ████
*/

	std::string date_string()
	{
		const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
		return std::format("{:%d.%m.%Y}", std::chrono::zoned_time{std::chrono::current_zone(), now});
	}

	std::string time_string(std::string_view separator = ":")
	{
		const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
		return std::format("{0:%H}{1}{0:%M}{1}{0:%S}", std::chrono::zoned_time{std::chrono::current_zone(), now}, separator);
	}

	// keep the newest `count` files named log.*.txt in the current directory
	void clean_older_logfiles(std::size_t count = 4)
	{
		namespace fs = std::filesystem;

		std::error_code                                      ec;
		std::vector<std::pair<fs::path, fs::file_time_type>> logfiles;

		for (const auto& entry : fs::directory_iterator(fs::current_path(ec), ec))
		{
			if (not entry.is_regular_file(ec) or entry.path().extension() != ".txt")
				continue;

			if (entry.path().stem().string().starts_with("log."))
				logfiles.emplace_back(entry.path(), entry.last_write_time(ec));
		}

		if (logfiles.size() <= count)
			return;

		std::ranges::sort(logfiles, [](const auto& a, const auto& b) { return a.second > b.second; });

		for (const auto& [path, time] : logfiles | std::views::drop(count))
			fs::remove(path, ec);
	}
/*
*     ████     ████    █    █    ████     ████    █        ██████    ████    ██████   █    █   █   ██   
*    █        █    █   ██   █   █        █    █   █        █        █          ██     ██   █   █  █     
*    █        █    █   █ █  █    ████    █    █   █        ████      ████      ██     █ █  █   ███      
*    █        █    █   █  █ █        █   █    █   █        █             █     ██     █  █ █   █  █     
*     ████     ████    █   ██    ████     ████    ██████   ██████    ████    ██████   █   ██   █   ██   
*/
	// Sinks implement write, ewrite, and flush.

	// stdout/stderr + OutputDebugString
	export struct ConsoleSink
	{
		void write(std::string_view text)
		{
			WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text.data(), static_cast<DWORD>(text.size()), nullptr, nullptr);
			OutputDebugStringA(std::string(text).c_str());
		}

		void ewrite(std::string_view text)
		{
			WriteFile(GetStdHandle(STD_ERROR_HANDLE), text.data(), static_cast<DWORD>(text.size()), nullptr, nullptr);
			OutputDebugStringA(std::string(text).c_str());
		}

		void flush() {}
	};

/*
*    ██████   ██████   █        ██████    ████    ██████   █    █   █   ██   
*    █          ██     █        █        █          ██     ██   █   █  █     
*    ████       ██     █        ████      ████      ██     █ █  █   ███      
*    █          ██     █        █             █     ██     █  █ █   █  █     
*    █        ██████   ██████   ██████    ████    ██████   █   ██   █   ██   
*/

	// Buffers text; the file is created on the first flush or oversized write.
	export struct FileSink
	{
		static constexpr std::size_t BUFFER_SIZE = 2_KiB;

		std::filesystem::path    path; // empty: log.<date>.<time>.txt in the current directory
		std::vector<std::byte>   buffer;
		std::size_t              offset{0};
		std::mutex               mutex;

		FileSink()
			: buffer(BUFFER_SIZE)
		{
		}

		explicit FileSink(std::filesystem::path p)
			: path(std::move(p))
			, buffer(BUFFER_SIZE)
		{
		}

		~FileSink() { flush(); }

		FileSink(const FileSink&)            = delete;
		FileSink& operator=(const FileSink&) = delete;

		void write(std::string_view text); // log-impl.cppm

		void ewrite(std::string_view text) { write(text); }


		void flush()
		{
			std::scoped_lock lock(mutex);
			flush_locked();
		}

	private:
		bool append_locked(std::span<const u8> bytes); // log-impl.cppm

		bool flush_locked()
		{
			if (offset == 0)
				return true;

			if (not append_locked({reinterpret_cast<const u8*>(buffer.data()), offset}))
				return false;

			offset = 0;
			return true;
		}
	};
/*
*    ████     ████    █    █    ████     ████    █        ██████   ██████   ██████   █        ██████    ████    ██████   █    █   █   ██   
*   █        █    █   ██   █   █        █    █   █        █        █          ██     █        █        █          ██     ██   █   █  █     
*   █        █    █   █ █  █    ████    █    █   █        ████     ████       ██     █        ████      ████      ██     █ █  █   ███      
*   █        █    █   █  █ █        █   █    █   █        █        █          ██     █        █             █     ██     █  █ █   █  █     
*    ████     ████    █   ██    ████     ████    ██████   ██████   █        ██████   ██████   ██████    ████    ██████   █   ██   █   ██   
*/
	export struct ConsoleFileSink
	{
		ConsoleSink console;
		FileSink    file;

		ConsoleFileSink() = default;

		explicit ConsoleFileSink(std::filesystem::path path)
			: file(std::move(path))
		{
		}

		void write(std::string_view text)
		{
			console.write(text);
			file.write(text);
		}

		void ewrite(std::string_view text)
		{
			console.ewrite(text);
			file.ewrite(text);
		}

		void flush()
		{
			file.flush();
		}
	};

/*
*   █         ████     ████     ████    ██████   █████    
*   █        █    █   █        █        █        █    █   
*   █        █    █   █  ███   █  ███   ████     █████    
*   █        █    █   █    █   █    █   █        █   █    
*   ██████    ████     ████     ████    ██████   █    █   
*/


	export template<typename Sink>
	struct Logger : private Sink
	{
		using Sink::flush;

		template<typename... Args>
		explicit Logger(Args&&... args)
			: Sink(std::forward<Args>(args)...)
		{
		}

		// raw text, no prefix, no newline
		template<typename... Args>
		void print(std::string_view fmt, Args&&... args)
		{
			if constexpr (sizeof...(Args) > 0)
				Sink::write(std::vformat(fmt, std::make_format_args(args...)));
			else
				Sink::write(fmt);
		}

		template<StringViewSource T, typename... Args>
		void print(const T& fmt, Args&&... args)
		{
			print(fmt.as_string_view(), std::forward<Args>(args)...);
		}

		template<typename... Args>
		void println(std::string_view fmt, Args&&... args)
		{
			if constexpr (sizeof...(Args) > 0)
				Sink::write(std::format("{}\n", std::vformat(fmt, std::make_format_args(args...))));
			else
				Sink::write(std::format("{}\n", fmt));
		}

		template<StringViewSource T, typename... Args>
		void println(const T& fmt, Args&&... args)
		{
			println(fmt.as_string_view(), std::forward<Args>(args)...);
		}

		void println() { Sink::write("\n"); }

		template<typename... Args>
		void eprint(std::string_view fmt, Args&&... args)
		{
			if constexpr (sizeof...(Args) > 0)
				Sink::ewrite(std::vformat(fmt, std::make_format_args(args...)));
			else
				Sink::ewrite(fmt);
		}

		template<StringViewSource T, typename... Args>
		void eprint(const T& fmt, Args&&... args)
		{
			eprint(fmt.as_string_view(), std::forward<Args>(args)...);
		}

		template<typename... Args>
		void eprintln(std::string_view fmt, Args&&... args)
		{
			if constexpr (sizeof...(Args) > 0)
				Sink::ewrite(std::format("{}\n", std::vformat(fmt, std::make_format_args(args...))));
			else
				Sink::ewrite(std::format("{}\n", fmt));
		}

		template<StringViewSource T, typename... Args>
		void eprintln(const T& fmt, Args&&... args)
		{
			eprintln(fmt.as_string_view(), std::forward<Args>(args)...);
		}

		void eprintln() { Sink::ewrite("\n"); }

	};

/*
*    ████    █         ████    █████     ████    █         ████    
*   █        █        █    █   █    █   █    █   █        █        
*   █  ███   █        █    █   █████    ██████   █         ████    
*   █    █   █        █    █   █    █   █    █   █             █   
*    ████    ██████    ████    █████    █    █   ██████    ████    
*/


	export using ConsoleLogger = Logger<ConsoleSink>;
	export using FileLogger    = Logger<FileSink>;

	// debug: console + file, release: file only
#ifdef NDEBUG
	export using DefaultLogger = Logger<FileSink>;
#else
	export using DefaultLogger = Logger<ConsoleFileSink>;
#endif

	DefaultLogger global_logger;

	std::atomic_bool suppress_init{false};
	std::atomic_bool initialized{false};
	std::once_flag   init_flag;

	// declared after global_logger so it is destroyed first
	struct ClosingGuard
	{
		~ClosingGuard()
		{
			if (initialized.load())
				global_logger.println("\n\nDeckard Log closing at {} {}", date_string(), time_string());

			global_logger.flush();
		}
	} closing_guard;

	export void initialize()
	{
		std::call_once(init_flag,
					   []
					   {
						   if (suppress_init.load())
							   return;

						   global_logger.println("Deckard Log initialized at {} {}\n", date_string(), time_string());
						   initialized = true;
					   });
	}

	export void set_suppress_init(bool suppress) { suppress_init = suppress; }

/*
*   █████    █    █   █████    █        ██████    ████    
*   █    █   █    █   █    █   █          ██     █        
*   █████    █    █   █████    █          ██     █        
*   █        █    █   █    █   █          ██     █        
*   █         ████    █████    ██████   ██████    ████    
*/


	export void flush() { global_logger.flush(); }

	export template<typename... Args>
	void print(std::string_view fmt, Args&&... args)
	{
		global_logger.print(fmt, args...);
	}

	export template<StringViewSource T, typename... Args>
	void print(const T& fmt, Args&&... args)
	{
		global_logger.print(fmt, std::forward<Args>(args)...);
	}

	export template<typename... Args>
	void println(std::string_view fmt, Args&&... args)
	{
		global_logger.println(fmt, args...);
	}

	export template<StringViewSource T, typename... Args>
	void println(const T& fmt, Args&&... args)
	{
		global_logger.println(fmt, std::forward<Args>(args)...);
	}

	export void println()
	{
		global_logger.println();
	}

	export template<typename... Args>
	void eprint(std::string_view fmt, Args&&... args)
	{
		global_logger.eprint(fmt, std::forward<Args>(args)...);
	}

	export template<StringViewSource T, typename... Args>
	void eprint(const T& fmt, Args&&... args)
	{
		global_logger.eprint(fmt, std::forward<Args>(args)...);
	}

	export template<typename... Args>
	void eprintln(std::string_view fmt, Args&&... args)
	{
		global_logger.eprintln(fmt, std::forward<Args>(args)...);
	}

	export template<StringViewSource T, typename... Args>
	void eprintln(const T& fmt, Args&&... args)
	{
		global_logger.eprintln(fmt, std::forward<Args>(args)...);
	}

	export void eprintln()
	{
		global_logger.eprintln();
	}

} // namespace deckard::log

export namespace deckard
{
	void flush() { log::flush(); }
} // namespace deckard

