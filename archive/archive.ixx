module;
#include <sqlite3.h>

export module deckard.archive;

import std;
import deckard.debug;

namespace fs = std::filesystem;

export namespace deckard::archive
{
	// TODO: Pivot to just holding binary data as data pak,
	// TODO: update from filesystem, check whats changed, update only changed files
	// TODO: Compress files before
	// TODO: path, size, compressed size, meta?
	//
	// TODO: db->load_folder("level01"); -> gives vector of handles?
	// TODO: db->load_file("level01/mainscript.txt");

	class file
	{
	public:
		file() = default;

		file(fs::path file) noexcept { open(file); }

		~file() { close(); }

		bool open(fs::path dbfile) noexcept
		{
			int rc = sqlite3_open(dbfile.string().c_str(), &m_db);
			if (rc)
			{
				dbg::println("SQLite3 open error: {}", sqlite3_errmsg(m_db));
				return false;
			}

			exec("PRAGMA synchronous = NORMAL;");
			exec("PRAGMA journal_mode = WAL;");
			exec("PRAGMA temp_store = MEMORY;");

			return true;
		}

		bool close() noexcept
		{
			int rc = sqlite3_close(m_db);
			if (rc != SQLITE_OK)
			{
				dbg::println("SQLite3 closing error: {}", sqlite3_errmsg(m_db));
				return false;
			}
			m_db = nullptr;
			return true;
		}

		bool is_open() const noexcept { return m_db != nullptr; }

		template<typename... Args>
		bool exec(std::string_view fmt, Args&&... args) const noexcept
		{
			if (!is_open())
				return false;


			const auto statement = std::vformat(fmt, std::make_format_args(args...));
			int        rc        = sqlite3_exec(m_db, statement.data(), &log_callback, 0, nullptr);

			if (rc != SQLITE_OK)
			{
				dbg::println("SQLite3 error: {}", sqlite3_errmsg(m_db));
				return false;
			}

			return true;
		}

	private:
		static int log_callback(void*, int column_count, char** data, char** columns) noexcept
		{

			dbg::println("Column count: {}", column_count);
			for (int i = 0; i < column_count; i++)
			{
				std::string d("data");
				if (d.contains(columns[i]))
					dbg::println("\tData in column: '{}' = '{}'", columns[i], data[i]);
				else
					dbg::println("\tData in column: '{}' = '{}'", columns[i], data[i]);
			}

			return 0;
		}

		sqlite3* m_db{nullptr};
	};
} // namespace deckard::archive
