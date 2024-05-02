module;
#include <sqlite3.h>

export module deckard.db;


import std;
import deckard.types;
import deckard.debug;
import deckard.enums;

namespace fs = std::filesystem;

export namespace deckard::sqlite
{


	class db final
	{
	public:
		db() = default;

		db(fs::path file) noexcept { open(file); }

		~db() { close(); }

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

			application_id(0);
			user_version(0);


			return true;
		}

		void application_id(int id) const noexcept { exec("PRAGMA application_id = {};", id); }

		void user_version(int ver) const noexcept { exec("PRAGMA user_version = {};", ver); }

		// TODO: variadic
		template<typename... Args>
		bool exec(std::string_view fmt, Args &&...args) const noexcept
		{
			if (!is_open())
				return false;

			auto Callback = &log_callback;

			auto statement = std::vformat(fmt, std::make_format_args(args...));
			int  rc        = sqlite3_exec(m_db, statement.data(), Callback, 0, nullptr);

			if (rc != SQLITE_OK)
			{
				dbg::println("SQLite3 error: {}", sqlite3_errmsg(m_db));
				return false;
			}

			return true;
		}

		static int log_callback(void *, int count, char **data, char **columns) noexcept
		{
			dbg::println("Column count: {}", count);
			for (int i = 0; i < count; i++)
			{
				dbg::println("\tData in column: '{}' = '{}'", columns[i], data[i]);
			}

			return 0;
		}

		bool begin_transaction() const noexcept { return exec("BEGIN TRANSACTION"); }

		bool commit() const noexcept { return exec("COMMIT;"); }

		void optimize() const noexcept
		{
			_ = exec("PRAGMA OPTIMIZE");
			_ = exec("VACUUM");
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

	private:
		sqlite3 *m_db{nullptr};
	};
} // namespace deckard::sqlite
