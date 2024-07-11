module;
#include <sqlite3.h>

export module deckard.archive;

import std;
import deckard.debug;

namespace fs = std::filesystem;

export namespace deckard::archive
{
	// TODO: update from filesystem, check whats changed, update only changed files
	// TODO: Compress files before
	// TODO: path, size, compressed size, meta?
	//
	// TODO: db->load_folder("level01"); -> gives vector of handles?
	// TODO: db->load_file("level01/mainscript.txt");
	/*

	CREATE TABLE IF NOT EXISTS files (
		id integer PRIMARY KEY,
		file_name text NOT NULL,
		file_blob text NOT NULL
	);

	fsID integer primary key autoincrement
				- an auto increment ID to uniquely identify a directory or a file
				  (no two directories or files will have the same ID) (ID number '1'
				   is reserved for root directory)
	fsType		- '0' indicates a directory while '1' is a file
	fsFileSize	- for a directory, it is '0'; for a file, it is the number of
				  bytes stored in the 'DataBlock' table
	fsName		- name of file or directory (not full path)
	fsParent	- fsID of the parent directory
	fsChild		- for a directory, it is an array of fsIDs of its children (each ID is 4 bytes
				  in size but can be changed to 8 bytes); for a file, it is a single 'dID',
				  which is the primary key in the 'DataBlock' table
	*/

	class file
	{
	public:
		file() = default;

		file(fs::path file) noexcept { open(file); }

		~file() { close(); }

		bool open(fs::path dbfile) noexcept
		{
			int rc = sqlite3_open(dbfile.string().c_str(), &m_db);
			if (rc != SQLITE_OK)
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
