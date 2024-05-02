module;
#include <sqlite3.h>

export module deckard.db;


import std;
import deckard.types;
import deckard.debug;
namespace fs = std::filesystem;

namespace deckard::sqlite
{
	export struct sqlite3_stmt;

	export class db final
	{
	public:
		~db() { close(); }

		bool open(fs::path dbfile) noexcept
		{
			int rc = sqlite3_open(dbfile.string().c_str(), &m_db);
			if (rc)
			{
				dbg::println("SQLite3 open error: {}", sqlite3_errmsg(m_db));
				return false;
			}

			_ = exec("PRAGMA synchronous = NORMAL;");
			_ = exec("PRAGMA journal_mode = WAL;");
			_ = exec("PRAGMA temp_store = MEMORY;");

			application_id(0);
			application_id(0);


			return true;
		}

		void application_id(int id) const noexcept
		{
			auto str = std::format("PRAGMA application_id = {}", id);
			_        = exec(str.c_str());
		}

		void user_version(int ver) const noexcept
		{
			auto str = std::format("PRAGMA user_version = {}", ver);
			_        = exec(str.c_str());
		}

		// TODO: variadic
		bool exec(std::string_view statement) const noexcept
		{
			if (!is_open())
				return false;

			int rc = sqlite3_exec(m_db, statement.data(), &log_callback, 0, nullptr);

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

		bool begin_transaction() const noexcept
		{
			int rc = exec("BEGIN TRANSACTION");

			return rc == SQLITE_OK;
		}

		bool end_transaction() const noexcept
		{
			int rc = exec("END TRANSACTION");

			return rc == SQLITE_OK;
		}

		bool commit() const noexcept
		{
			int rc = exec("COMMIT;");

			return rc == SQLITE_OK;
		}

		void close() const noexcept { sqlite3_close(m_db); }

		bool is_open() const noexcept { return m_db != nullptr; }

	private:
		sqlite3 *m_db{nullptr};
	};
} // namespace deckard::sqlite
