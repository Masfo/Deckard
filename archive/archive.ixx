module;
#include <sqlite3.h>

export module deckard.archive;

import std;
import deckard.db;

namespace fs = std::filesystem;

export namespace deckard::archive
{
	// TODO: update from filesystem, check whats changed, update only changed files
	// TODO: Compress files before
	// TODO: path, size, compressed size, meta?
	//
	// TODO: db->load_folder("level01"); -> gives vector of handles?
	// TODO: db->load_file("level01/mainscript.txt");

	// insert/update file
	// compress (if needed, heuristics, if compresses less-than X% store raw)

	// Schema:
	//		* meta
	//		* path
	//		* size (compressed if blob size is less)
	//		* sha256
	//		* blob

	// SELECT * FROM fs WHERE path LIKE 'data/level01/%' AND size <= 1024
	// SELECT * FROM fs WHERE path GLOB 'data/level01/*'

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

	/* ************************************
	*
	  // open sqlite3 database connection
	sqlite3* db;
	sqlite3_open("path/to/database.db", &db);

	// insert blob
	{
		sqlite3_stmt* stmtInsert = nullptr;
		sqlite3_prepare_v2(db, "INSERT INTO table_name (vector_blob) VALUES (?)", -1, &stmtInsert, nullptr);

		std::vector<float> blobData(128); // your data
		sqlite3_bind_blob(stmtInsertFace, 1, blobData.data(), static_cast<int>(blobData.size() * sizeof(float)), SQLITE_STATIC);

		if (sqlite3_step(stmtInsert) == SQLITE_DONE)
			std::cout << "Insert successful" << std::endl;
		else
			std::cout << "Insert failed" << std::endl;

		sqlite3_finalize(stmtInsert);
	}

	// retrieve blob
	{
		sqlite3_stmt* stmtRetrieve = nullptr;
		sqlite3_prepare_v2(db, "SELECT vector_blob FROM table_name WHERE id = ?", -1, &stmtRetrieve, nullptr);

		int id = 1; // your id
		sqlite3_bind_int(stmtRetrieve, 1, id);


		// sqlite3_blob_read, N+offset


		std::vector<float> blobData;
		if (sqlite3_step(stmtRetrieve) == SQLITE_ROW)
		{
			// retrieve blob data
			const float* pdata = reinterpret_cast<const float*>(sqlite3_column_blob(stmtRetrieve, 0));
			// query blob data size
			blobData.resize(sqlite3_column_bytes(stmtRetrieve, 0) / static_cast<int>(sizeof(float)));
			// copy to data vector
			std::copy(pdata, pdata + static_cast<int>(blobData.size()), blobData.data());
		}

		sqlite3_finalize(stmtRetrieve);
	}

	sqlite3_close(db);



	// inout

	 auto close_db = [](sqlite3* db) { sqlite3_close(db); };

	{
		// open an in-memory database, and manage its lifetime with std::unique_ptr
		std::unique_ptr<sqlite3, decltype(close_db)> up;
		sqlite3_open(":memory:", std::out_ptr(up));

		sqlite3* db = up.get();
		// do something with db ...
	}

	 SELECT * FROM Students WHERE Name GLOB 'A*e';
									   LIKE 'A_e';
	GLOB: case-sensitive
	LIKE:
	GLOB: * any
		  ? one

	LIKE: % any
		  _ one

	*/


#if 0
		class file
		{
		public:
			file() = default;

			file(fs::path file) { open(file); }

			~file() { close(); }

			bool open(fs::path dbfile)
			{
				int rc = sqlite3_open(system::from_wide(dbfile.wstring()).c_str(), &m_db);
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

			bool close()
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

			bool is_open() const { return m_db != nullptr; }

			template<typename... Args>
			bool exec(std::string_view fmt, Args&&... args) const
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
			static int log_callback(void*, int column_count, char** data, char** columns)
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
#endif

} // namespace deckard::archive
