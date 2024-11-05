module;
#include <sqlite3.h>

export module deckard.db;


import std;
import deckard.types;
import deckard.as;

import deckard.assert;
import deckard.debug;
import deckard.win32;
import deckard.random;
import deckard.helpers;


namespace fs = std::filesystem;

namespace deckard::db
{
	struct option
	{
		bool autoreset{true};
	};

	using sqlite_type = std::variant<std::monostate, u64, i64, f64, std::string, std::vector<u8>>;
	using sqlite_row  = std::unordered_map<std::string, sqlite_type>;
	using sqlite_rows = std::vector<sqlite_row>;

	export class db
	{
	private:
		using sqlite_function = void(sqlite3_context*, i32, sqlite3_value**);

		sqlite3*      m_db{nullptr};
		sqlite3_stmt* m_statement{nullptr};
		option        m_option{};

		sqlite_rows m_rows;

		template<typename T>
		T get_column_value(i32 index)
		{
			auto bytes = sqlite3_column_bytes(m_statement, index);
			auto type  = sqlite3_column_type(m_statement, index);
			switch (type)
			{
				case SQLITE_INTEGER:
					if constexpr (std::is_integral_v<T>)
						return sqlite3_column_int64(m_statement, index);
					return {};

				case SQLITE_FLOAT:
					if constexpr (std::is_floating_point_v<T>)
						return sqlite3_column_double(m_statement, index);
					return {};
				case SQLITE_TEXT:
				{
					if constexpr (std::is_same_v<T, std::string>)
					{
						std::string ret;
						const u8*   pdata = sqlite3_column_text(m_statement, index);
						ret.resize(bytes);
						std::copy(pdata, pdata + as<i32>(ret.size()), ret.data());
						return ret;
					}
					return {};
				}
				case SQLITE_BLOB:
				{
					if constexpr (non_string_container<T>)
					{
						std::vector<u8> blob;
						blob.resize(bytes);

						const u8* pdata = reinterpret_cast<const u8*>(sqlite3_column_blob(m_statement, index));
						std::copy(pdata, pdata + as<i32>(blob.size()), blob.data());

						return blob;
					}
					return {};
				}

				case SQLITE_NULL: return {};

				default: std::unreachable();
			};
		}

		template<typename T>
		void bind_sql(i32 index, const T& value)
		{


			i32 rc = SQLITE_ERROR;

			if constexpr (std::is_integral_v<T>)
			{
				if (std::in_range<i32>(value))
					rc = sqlite3_bind_int(m_statement, index, as<i32>(value));
				else if (std::in_range<i64>(value))
					rc = sqlite3_bind_int64(m_statement, index, value);
				else
				{
					dbg::println("TODO: big unsigned integers");
					return;
				}
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				rc = sqlite3_bind_double(m_statement, index, value);
			}
			else if constexpr (std::is_convertible_v<T, std::string_view>)
			{
				if constexpr (std::is_same_v<T, std::string> or std::is_same_v<T, std::string_view>)
					rc = sqlite3_bind_text(m_statement, index, value.data(), as<i32>(value.size()), SQLITE_TRANSIENT);
				else
					rc = sqlite3_bind_text(m_statement, index, value, as<i32>(std::strlen(value)), SQLITE_TRANSIENT);
			}
			else if constexpr (non_string_container<T>)
			{
				rc = sqlite3_bind_blob(m_statement, index, value.data(), as<i32>(value.size()), SQLITE_TRANSIENT);
			}
			else
			{
				//
				static_assert(false, "type not bindable");
			}

			if (rc != SQLITE_OK)
				dbg::println("{}", sqlite3_errmsg(m_db));
		}

		template<typename T, typename... Args>
		void bind_helper(i32 index, const T& value, Args... args)
		{
			bind_sql(index, value);

			if constexpr (sizeof...(args) > 0)
			{
				bind_helper(++index, args...);
			}
		}


	public:
		db(std::filesystem::path path)
			: db(path, {})
		{
		}

		db(std::filesystem::path path, option opt)
		{
			m_option = opt;
			open(path);
		}

		bool is_open() const { return m_db != nullptr; }

		std::string err() const { return sqlite3_errmsg(m_db); }

		std::expected<bool, std::string> exec(std::string_view sql)
		{
			sqlite3_stmt* statement{nullptr};
			i32           rc = sqlite3_prepare_v2(m_db, sql.data(), -1, &statement, nullptr);
			if (rc != SQLITE_OK)
				return std::unexpected(sqlite3_errmsg(m_db));

			rc = sqlite3_step(statement);

			rc = sqlite3_finalize(statement);
			if (rc != SQLITE_OK)
				return std::unexpected(sqlite3_errmsg(m_db));

			return true;
		}

		db& prepare(std::string_view input)
		{
			clear();

			i32 rc = sqlite3_prepare_v2(m_db, input.data(), -1, &m_statement, nullptr);

			if (rc != SQLITE_OK)
			{
				m_statement = nullptr;
				dbg::println("sqlite3::prepare: {}", sqlite3_errmsg(m_db));
			}

			return *this;
		}

		template<typename... Args>
		db& bind(Args... args)
		{
			if (m_statement == nullptr)
			{
				dbg::println("sqlite3::bind: statement is invalid");
				return *this;
			}

			bind_helper(1, args...);

			return *this;
		}

		// reset state and bindings
		db& reset()
		{
			i32 rc = sqlite3_reset(m_statement);
			if (rc != SQLITE_OK)
			{
				dbg::println("sqlite3::reset_bindings: {}", sqlite3_errmsg(m_db));
				return *this;
			}

			rc = sqlite3_clear_bindings(m_statement);
			if (rc != SQLITE_OK)
			{
				dbg::println("sqlite3::reset_bindings: {}", sqlite3_errmsg(m_db));
				return *this;
			}

			return *this;
		}

		// clear statement
		db& clear()
		{
			i32 rc = sqlite3_finalize(m_statement);
			if (rc != SQLITE_OK)
			{
				dbg::println("SQLite3 error: {}", sqlite3_errmsg(m_db));
			}

			m_statement = nullptr;
			m_rows.clear();

			return *this;
		}

		// just simple execute of SQL, no results
		db& exec()
		{
			if (m_statement == nullptr)
			{
				dbg::println("sqlite3::commit: statement is invalid");
				return *this;
			}

			i32 rc = sqlite3_step(m_statement);
			if (rc != SQLITE_DONE and rc != SQLITE_ROW)
			{
				dbg::println("sqlite3::exec: {}", sqlite3_errmsg(m_db));
			}

			if (m_option.autoreset == true)
				reset();

			return *this;
		}

		i32 rows() const { return as<i32>(m_rows.size()); }

		template<typename T=i64>
		std::optional<T> at(const std::string& col, i32 row = 0)
		{
			//
			if (m_rows.size() >= 0 and row < m_rows.size())
			{
				const auto rowmap = m_rows[row];
				if (rowmap.contains(col))
				{
					auto rm = rowmap.at(col);

					// TODO: variant -> T
					// if string -> T, try_to_number
					// // T -> string, ??
					// if integer->integer -> simple as cast

					if constexpr (std::is_signed_v<T>)
					{
						if (std::holds_alternative<T>(rm))
							return std::get<T>(rm);
						return {};
					}
					else
					{
						if constexpr (std::is_unsigned_v<T>)
						{
							if (not std::holds_alternative<i64>(rm))
							{
								if (std::holds_alternative<std::string>(rm))
									return try_to_number<T>(std::get<std::string>(rm));

								return {};
							}
						}
						return {};
					}
				}

				return {};
			}

			return {};
		}

		// execute and cache results
		db& commit()

		{
			if (m_statement == nullptr)
			{
				dbg::println("sqlite3::commit: statement is invalid");
				return *this;
			}

			i32 data_count = sqlite3_data_count(m_statement);
			i32 col_count  = sqlite3_column_count(m_statement);

			while (sqlite3_step(m_statement) == SQLITE_ROW)
			{
				sqlite_row row;
				for (i32 i : upto(col_count))
				{
					auto type  = sqlite3_column_type(m_statement, i);
					auto bytes = sqlite3_column_bytes(m_statement, i);
					auto res   = sqlite3_column_double(m_statement, i);

					auto name = sqlite3_column_name(m_statement, i);
					if (name != nullptr)
					{
						switch (type)
						{
							case SQLITE_INTEGER:
							{
								row[name] = get_column_value<i64>(i);
								break;
							}

							case SQLITE_FLOAT:
							{
								row[name] = get_column_value<f64>(i);
								break;
							}
							case SQLITE_TEXT:
							{
								row[name] = get_column_value<std::string>(i);
								break;
							}
							case SQLITE_BLOB:
							{
								row[name] = get_column_value<std::vector<u8>>(i);
								break;
							}
							case SQLITE_NULL: break;
						}
					}
				}
				m_rows.push_back(row);
			}


			if (m_option.autoreset == true)
				reset();

			return *this;
		}

		std::expected<bool, std::string> bind_function(std::string_view signature, i32 param_count, sqlite_function func)
		{
			//
			i32 rc = sqlite3_create_function(m_db, signature.data(), param_count, SQLITE_UTF8, NULL, func, NULL, NULL);
			if (rc != SQLITE_OK)
			{
				return std::unexpected(sqlite3_errmsg(m_db));
			}
			return true;
		}

		std::expected<bool, std::string> open(std::filesystem::path path)
		{
			if (path.empty())
				return std::unexpected("database name empty");

			i32 rc = sqlite3_open_v2(path.generic_string().c_str(), &m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
			if (rc != SQLITE_OK)
				return std::unexpected(sqlite3_errmsg(m_db));

			return true;
		}

		void close()
		{
			clear();

			i32 rc = sqlite3_close_v2(m_db);
			if (rc != SQLITE_OK)
				dbg::println("sqlite3_close_v2: error: {}", sqlite3_errmsg(m_db));

			m_db = nullptr;
		}
	};

	void my_add(sqlite3_context* ctx, int argc, sqlite3_value** argv)
	{
		auto x = sqlite3_value_double(argv[0]);
		auto y = sqlite3_value_double(argv[1]);
		sqlite3_result_double(ctx, x + y);
		// return 0;
	}

	export void test()
	{
		// open sqlite3 database connection
		sqlite3*      db{};
		sqlite3_stmt* stmtInsert{};


		std::array<u8, 512> blob{};
		random::random_bytes(blob);


		int rc = sqlite3_open_v2("database.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}


		rc = sqlite3_create_function(db, "my_add", 2, SQLITE_UTF8, NULL, my_add, NULL, NULL);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}

		// std::vector<u8>     blob; // your data
		// blob.resize(128);

		// random::random_bytes({blob.data(), blob.size()});

		blob[0] = 'M';
		blob[1] = 'A';
		blob[2] = 'S';
		blob[3] = 'F';
		blob[4] = 'O';


		//  insert blob

		rc = sqlite3_prepare_v2(db, "INSERT INTO blobs (data) VALUES (?1)", -1, &stmtInsert, nullptr);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}


		rc = sqlite3_bind_blob(stmtInsert, 1, &blob[0], as<i32>(blob.size()), SQLITE_STATIC);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}
		rc = sqlite3_step(stmtInsert);
		if (rc != SQLITE_DONE)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}

		rc = sqlite3_finalize(stmtInsert);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}

	ending:
		rc = sqlite3_close_v2(db);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
		}

		int x = 0;
	}

	export void test_read_blob()
	{
		sqlite3*      db{};
		sqlite3_stmt* stmtRetrieve{};
		int           id = 1; // your id
		int           data_count{};
		int           col_count{};

		std::array<u8, 256> blob{};
		// std::vector<u8> blob;

		int rc = sqlite3_open_v2("database.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}

		rc = sqlite3_create_function_v2(db, "my_add", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, my_add, NULL, NULL, NULL);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}

		rc = sqlite3_create_function_v2(
		  db,
		  "my_add2",
		  2,
		  SQLITE_UTF8 | SQLITE_DETERMINISTIC,
		  NULL,
		  [](sqlite3_context* ctx, int argc, sqlite3_value** argv)
		  {
			  auto x = sqlite3_value_double(argv[0]);
			  auto y = sqlite3_value_double(argv[1]);
			  sqlite3_result_double(ctx, x + y);
		  },
		  NULL,
		  NULL,
		  NULL);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}

		{
			// retrieve blob
			rc = sqlite3_prepare_v2(db, "SELECT my_add2(123.2,123.4)", -1, &stmtRetrieve, nullptr);
			if (rc != SQLITE_OK)
			{
				dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
				goto ending;
			}

			while (sqlite3_step(stmtRetrieve) == SQLITE_ROW)
			{
				auto type  = sqlite3_column_type(stmtRetrieve, 0);
				auto bytes = sqlite3_column_bytes(stmtRetrieve, 0);
				auto res   = sqlite3_column_double(stmtRetrieve, 0);

				int j = 0;
			}
		}
		rc = sqlite3_finalize(stmtRetrieve);


		// retrieve blob
		rc = sqlite3_prepare_v2(db, "SELECT data FROM blobs where id = ?1", -1, &stmtRetrieve, nullptr);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}

		rc = sqlite3_bind_int(stmtRetrieve, 1, id);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}

		// rc = sqlite3_bind_blob(stmtRetrieve, 1, &blob[0], as<i32>(blob.size()), SQLITE_STATIC);
		// if (rc != SQLITE_OK)
		//{
		//	dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
		//	goto ending;
		// }

		// #define SQLITE_INTEGER  1
		// #define SQLITE_FLOAT    2
		// #define SQLITE_TEXT     3
		// #define SQLITE_BLOB     4
		// #define SQLITE_NULL     5
		//
		// sqlite3_blob_read, N+offset

		// sqlite3_column_bytes
		// sqlite3_column_type
		// sqlite3_column_count
		// sqlite3_data_count
		//
		// sqlite3_column_blob			BLOB result
		// sqlite3_column_double		REAL result
		// sqlite3_column_int			32-bit INTEGER result
		// sqlite3_column_int64			64-bit INTEGER result
		// sqlite3_column_text			UTF-8 TEXT result

		data_count = sqlite3_data_count(stmtRetrieve);
		col_count  = sqlite3_column_count(stmtRetrieve);

		while (sqlite3_step(stmtRetrieve) == SQLITE_ROW)
		{
			dbg::print("{}: ", sqlite3_column_name(stmtRetrieve, 0));
			// retrieve blob data
			const u8* pdata = reinterpret_cast<const u8*>(sqlite3_column_blob(stmtRetrieve, 0));
			// query blob data size
			// blob.resize(sqlite3_column_bytes(stmtRetrieve, 0) / static_cast<int>(sizeof(float)));
			// copy to data vector
			std::copy(pdata, pdata + static_cast<int>(blob.size()), blob.data());
			dbg::println("{}", to_hex_string<u8>(blob, {.delimiter = " ", .show_hex = false}));
		}


		rc = sqlite3_finalize(stmtRetrieve);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
			goto ending;
		}


	ending:
		rc = sqlite3_close_v2(db);
		if (rc != SQLITE_OK)
		{
			dbg::println("SQLite3 error: {}", sqlite3_errmsg(db));
		}
	}

} // namespace deckard::db
