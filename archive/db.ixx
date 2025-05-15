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
	#if 0
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
			i32           rc = 0;


			rc = sqlite3_prepare_v2(m_db, sql.data(), -1, &statement, nullptr);
			if (rc != SQLITE_OK)
			{
				dbg::println("sqlite3::prepare: {}", sqlite3_errmsg(m_db));
				return std::unexpected(sqlite3_errmsg(m_db));
			}


			rc = sqlite3_step(statement);
			switch (rc)
			{
				case SQLITE_ROW: [[fallthrough]];
				case SQLITE_DONE: break;

				default:
				{
					dbg::println("sqlite3::exec: step error: {}", sqlite3_errmsg(m_db));
					rc = sqlite3_finalize(statement);
					if (rc != SQLITE_OK)
						return std::unexpected(sqlite3_errmsg(m_db));
				}
			}
			rc = sqlite3_finalize(statement);
			if (rc != SQLITE_OK)
				return std::unexpected(sqlite3_errmsg(m_db));

			return true;
		}

		db& begin_transaction()
		{
			exec("BEGIN TRANSACTION;");
			return *this;
		}

		db& end_transaction()
		{
			exec("COMMIT TRANSACTION;");
			return *this;
		}

		db& prepare(std::string_view input)
		{
			clear();

			// TODO: statement to own struct/class, caching it
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
				return *this;
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

		template<typename T = i64>
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

					if constexpr (std::is_same_v<T, std::vector<u8>>)
					{
						if (std::holds_alternative<T>(rm))
							return std::get<T>(rm);
						return {};
					}

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
							case SQLITE_NULL:
							{
								row[name] = {};
								break;
							}
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

			// defaults
			exec("PRAGMA synchronous = NORMAL; PRAGMA journal_mode=WAL; PRAGMA temp_store = MEMORY;");
			return true;
		}

		void set_ids(u32 application_id, u32 user_id)
		{
			exec(std::format("PRAGMA application_id = {}; PRAGMA user_version = {};", application_id, user_id));
		}

		void close()
		{
			clear();

			exec("PRAGMA OPTIMIZE; VACUUM;");

			i32 rc = sqlite3_close_v2(m_db);
			if (rc != SQLITE_OK)
				dbg::println("sqlite3_close_v2: error: {}", sqlite3_errmsg(m_db));

			m_db = nullptr;
		}
	};

	/*
		auto bfs = db.bind_function(
	  "my_add",
	  2,
	  [](sqlite3_context* ctx, int argc, sqlite3_value** argv)
	  {
		  auto x = sqlite3_value_double(argv[0]);
		  auto y = sqlite3_value_double(argv[1]);
		  sqlite3_result_double(ctx, x + y);
	  });
	*/
	#endif
} // namespace deckard::db
