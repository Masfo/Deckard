export module deckard.config;

import deckard.assert;
import deckard.utf8;
import deckard.types;
import deckard.file;
import deckard.debug;
import deckard.helpers;
import deckard.as;
import deckard.utils.hash;

namespace fs = std::filesystem;

namespace deckard
{
	export enum struct TokenType : u8 {
		NEWLINE_POSIX,   // 0x0A \n
		NEWLINE_WINDOWS, // 0x0D 0x0A \r\n
		SECTION,         // [section]
		COMMENT,         // # comment
		KEY,             // key =
		VALUE_STRING,    // "value" or bare identifier
		VALUE_INTEGER,   // 123
		VALUE_FLOAT,     // 3.14
		VALUE_BOOL,      // true / false
	};


#ifdef _DEBUG
	constexpr std::array<std::string_view, 9> TokenStrings{
	  "NEWLINE_POSIX",
	  "NEWLINE_WINDOWS",
	  "SECTION",
	  "COMMENT",
	  "KEY",
	  "STRING",
	  "INTEGER",
	  "FLOAT",
	  "BOOL",
	};

	std::string_view token_type_to_string(TokenType type) { return TokenStrings[as<u8>(type)]; }
#endif

	struct TokenValue
	{
		TokenType type{};
		u32       start{};
		u32       length{};
	};

	export struct parse_error
	{
		std::string message;
	};

	class config;

	export struct value_proxy
	{
		std::string_view key;
		const config&    cfg;

		template<typename T>
		T as() const;

		template<typename T>
		std::vector<T> as_all() const;

		operator bool() const;
		explicit operator std::string() const;
		explicit operator int() const;
		explicit operator float() const;
		explicit operator double() const;
	};

	export struct mutable_value_proxy
	{
		std::string_view key;
		config&          cfg;

		template<typename T>
		T as() const;

		template<typename T>
		std::vector<T> as_all() const;

		template<typename T>
		mutable_value_proxy& operator=(T value);

		operator bool() const;
		explicit operator std::string() const;
		explicit operator int() const;
		explicit operator float() const;
		explicit operator double() const;
	};

	export class config
	{
	private:
		utf8::string                 m_data;
		std::vector<TokenValue>      tokens;
		std::unordered_map<u64, std::vector<u64>> key_hash_to_token_index;
		std::vector<parse_error>     m_errors;
		fs::path                     filename;

		void skip_until_newline(utf8::view& view)
		{
			while (view and (not utf8::is_newline(*view)))
				view++;
		}

		void skip_whitespace(utf8::view& view)
		{
			using namespace utf8::basic_characters;

			while (view and (*view == SPACE or *view == CHARACTER_TABULATION))
				view++;
		}

		void trim_trailing_whitespace(utf8::view& end, u32& len)
		{
			using namespace utf8::basic_characters;

			while (len > 0)
			{
				auto tmp = end;
				--tmp;
				const char32 ch = *tmp;
				if (ch == SPACE or ch == CHARACTER_TABULATION)
				{
					--end;
					--len;
				}
				else
					break;
			}
		}

		void build_index()
		{
			std::string section;
			utf8::view  view(m_data);

			for (u64 i = 0; i < tokens.size(); ++i)
			{
				const auto& tok = tokens[i];

				if (tok.type == TokenType::SECTION)
				{
					auto sv = view.subview(tok.start, tok.length);
					section.assign(sv.c_str(), sv.size_in_bytes());
				}
				else if (tok.type == TokenType::KEY and i + 1 < tokens.size())
				{
					auto key_sv  = view.subview(tok.start, tok.length);
					auto key_str = std::string{key_sv.c_str(), key_sv.size_in_bytes()};

					auto full_key = section.empty() ? key_str : section + '.' + key_str;
					auto hash     = utils::stringhash(full_key);
					key_hash_to_token_index[hash].push_back(i + 1);
				}
			}
		}

		static TokenType classify_value(std::string_view sv)
		{
			if (sv.empty())
				return TokenType::VALUE_STRING;

			if (sv == "true" or sv == "false")
				return TokenType::VALUE_BOOL;

			const char* p   = sv.data();
			const char* end = p + sv.size();

			// Optional leading sign
			if (p != end and (*p == '+' or *p == '-'))
				++p;

			// Integer digits
			const char* int_start = p;
			while (p != end and *p >= '0' and *p <= '9')
				++p;

			if (p == int_start)
				return TokenType::VALUE_STRING; // no digits

			if (p == end)
				return TokenType::VALUE_INTEGER;

			// Decimal point separates integer from float
			if (*p != '.')
				return TokenType::VALUE_STRING;
			++p;

			// Fractional digits (required)
			const char* frac_start = p;
			while (p != end and *p >= '0' and *p <= '9')
				++p;

			if (p == frac_start)
				return TokenType::VALUE_STRING; // bare "3." is not a float

			// Optional exponent
			if (p != end and (*p == 'e' or *p == 'E'))
			{
				++p;
				if (p != end and (*p == '+' or *p == '-'))
					++p;
				const char* exp_start = p;
				while (p != end and *p >= '0' and *p <= '9')
					++p;
				if (p == exp_start)
					return TokenType::VALUE_STRING; // 'e' without digits
			}

			return (p == end) ? TokenType::VALUE_FLOAT : TokenType::VALUE_STRING;
		}

		void parse_key(utf8::view& view)
		{
			using namespace utf8::basic_characters;

			skip_whitespace(view);

			auto copyview = view;
			u32  start    = as<u32>(copyview.index());
			while (copyview and utf8::is_xid_continue(*copyview))
				copyview++;
			u32 len = as<u32>(copyview.index() - start);
			trim_trailing_whitespace(copyview, len);
			skip_whitespace(copyview);
			if (not copyview or *copyview != EQUALS_SIGN)
			{
				skip_until_newline(view);
				return;
			}

			view += len;
			skip_whitespace(view);
			tokens.push_back({TokenType::KEY, start, len});
		}

		void parse_value(utf8::view& view)
		{
			using namespace utf8::basic_characters;

			skip_whitespace(view);
			if (not view)
				return;

			// Quoted string — store content inside the quotes
			if (*view == QUOTATION_MARK)
			{
				view++; // skip opening "
				auto copyview = view;
				u32  start    = as<u32>(copyview.index());
				while (copyview and *copyview != QUOTATION_MARK and not utf8::is_newline(*copyview))
					copyview++;
				u32 len = as<u32>(copyview.index() - start);
				view += len;
				if (view and *view == QUOTATION_MARK)
					view++; // skip closing "
				tokens.push_back({TokenType::VALUE_STRING, start, len});
				return;
			}

			// Unquoted value (booleans, numbers, bare identifiers)
			auto copyview = view;
			u32  start    = as<u32>(copyview.index());
			while (copyview and not utf8::is_newline(*copyview))
			{
				if (*copyview == HASH)
					break;
				copyview++;
			}
			u32 len = as<u32>(copyview.index() - start);
			trim_trailing_whitespace(copyview, len);

			auto             val_sub = view.subview(start, len);
			std::string_view sv{val_sub.c_str(), val_sub.size_in_bytes()};
			view += len;
			tokens.push_back({classify_value(sv), start, len});
		}

		void parse_comment(utf8::view& view)
		{
			using namespace utf8::basic_characters;

			view++; // eat hash
			skip_whitespace(view);

			auto copyview = view;
			u32  start    = as<u32>(copyview.index());

			while (copyview and (not utf8::is_newline(*copyview)))
				copyview++;

			u32 len = as<u32>(copyview.index() - start);
			trim_trailing_whitespace(copyview, len);

			view += len;

			tokens.push_back({TokenType::COMMENT, start, len});
		}

		void parse_section(utf8::view& view)
		{
			using namespace utf8::basic_characters;
			view++; // eat left square bracket

			u32 start = as<u32>(view.index());

			while (view and *view != RIGHT_SQUARE_BRACKET and not utf8::is_newline(*view))
				view++;
			u32 len = as<u32>(view.index() - start);

			// Trim on a copy so view stays positioned at ']'
			auto copyview = view;
			trim_trailing_whitespace(copyview, len);

			const bool has_closing_bracket = view and *view == RIGHT_SQUARE_BRACKET;
			if (has_closing_bracket)
				view++; // eat right square bracket

			if (len == 0)
			{
				m_errors.push_back({"empty section name"});
				return;
			}

			if (not has_closing_bracket)
			{
				m_errors.push_back({"missing closing ']' for section"});
				return;
			}

			tokens.push_back({TokenType::SECTION, start, len});
		}

		void parse()
		{
			using namespace utf8::basic_characters;
			using namespace std::string_view_literals;


			if (m_data.empty())
				return;

			utf8::view view(m_data);


			while (view)
			{
				// New lines
				if (*view == LINE_FEED) // \n
				{
					u32 current_index = as<u32>(view.index());
					tokens.push_back({TokenType::NEWLINE_POSIX, current_index, 1});
					view++;
					continue;
				}

				if (*view == CARRIAGE_RETURN) // \r
				{
					u32 current_index = as<u32>(view.index());
					view++;

					if (view and *view == LINE_FEED) // \r\n
					{
						tokens.push_back({TokenType::NEWLINE_WINDOWS, current_index, 2});
						view++;
						continue;
					}

					tokens.push_back({TokenType::NEWLINE_POSIX, current_index, 1});
					continue;
				}

				// section
				if (*view == LEFT_SQUARE_BRACKET)
				{
					parse_section(view);
					continue;
				}

				// whitespace before key is ignored
				if (utf8::is_whitespace(*view))
				{
					view++;
					continue;
				}

				// Comment
				if (*view == HASH)
				{
					parse_comment(view);
					continue;
				}

				// key
				if (utf8::is_xid_start(*view))
				{
					parse_key(view);
					continue;
				}

				// equals
				if (*view == EQUALS_SIGN)
				{
					view++;
					parse_value(view);
					continue;
				}
				dbg::println("config: unrecognized token starting with '{}'", static_cast<char>(*view));
				view++;
			}

			build_index();
		}

	public:
		config() = default;

		explicit config(fs::path file)
		{
			filename = file;
			m_data.assign(file::read_text_file(file));
			parse();
		}

		config(std::string_view input)
			: m_data(input)
		{
			parse();
		}

		config(const utf8::string& input)
			: m_data(input)
		{
			parse();
		}

#ifdef __cpp_deleted_function
#error "Use this one"
		config(const char* input) = delete("lets avoid old char*");
#endif
		config(const char* input) = delete;

		~config() { (void)save(); }

#ifdef _DEBUG
		void dump() const
		{
			utf8::view view(m_data);

			for (const auto& token : tokens)
			{
				dbg::println(
				  "{}({}-{}): '{}'",
				  token_type_to_string(token.type),
				  token.start,
				  token.length,
				  view.subview(token.start, token.length).trim());
			}
		}

		void dump_index() const
		{
			std::string section;
			utf8::view  view(m_data);

			for (u64 i = 0; i < tokens.size(); ++i)
			{
				const auto& tok = tokens[i];

				if (tok.type == TokenType::SECTION)
				{
					auto sv = view.subview(tok.start, tok.length);
					section.assign(sv.c_str(), sv.size_in_bytes());
				}
				else if (tok.type == TokenType::KEY and i + 1 < tokens.size())
				{
					auto key_sv   = view.subview(tok.start, tok.length);
					auto key_str  = std::string{key_sv.c_str(), key_sv.size_in_bytes()};
					auto full_key = section.empty() ? key_str : section + '.' + key_str;

					auto hash = utils::stringhash(full_key);
					auto it   = key_hash_to_token_index.find(hash);
					if (it == key_hash_to_token_index.end())
						continue;

					for (u64 val_idx : it->second)
					{
						const auto& val_tok = tokens[val_idx];
						auto        val_sv  = view.subview(val_tok.start, val_tok.length);
						dbg::println("'{}' -> '{}'", full_key, val_sv);
					}
				}
			}
		}
#endif
		u32 size() const { return static_cast<u32>(tokens.size()); }

		bool has_errors() const { return not m_errors.empty(); }

		std::span<const parse_error> errors() const { return m_errors; }

		template<typename T>
		T get(std::string_view key) const
		{
			auto it = key_hash_to_token_index.find(utils::stringhash(key));
			if (it == key_hash_to_token_index.end() or it->second.empty())
				return T{};

			const auto&      tok = tokens[it->second.front()];
			utf8::view       view(m_data);
			auto             val_view = view.subview(tok.start, tok.length);
			std::string_view sv{val_view.c_str(), val_view.size_in_bytes()};

			if constexpr (std::is_same_v<T, bool>)
				return sv == "true";
			else if constexpr (std::is_same_v<T, std::string>)
				return std::string{sv};
			else if constexpr (std::is_arithmetic_v<T>)
			{
				T result{};
				std::from_chars(sv.data(), sv.data() + sv.size(), result);
				return result;
			}
			else
				return T{};
		}

		template<typename T>
		std::vector<T> get_all(std::string_view key) const
		{
			auto it = key_hash_to_token_index.find(utils::stringhash(key));
			if (it == key_hash_to_token_index.end())
				return {};

			std::vector<T> results;
			results.reserve(it->second.size());
			utf8::view view(m_data);

			for (u64 idx : it->second)
			{
				const auto&      tok      = tokens[idx];
				auto             val_view = view.subview(tok.start, tok.length);
				std::string_view sv{val_view.c_str(), val_view.size_in_bytes()};

				if constexpr (std::is_same_v<T, bool>)
					results.push_back(sv == "true");
				else if constexpr (std::is_same_v<T, std::string>)
					results.push_back(std::string{sv});
				else if constexpr (std::is_arithmetic_v<T>)
				{
					T result{};
					std::from_chars(sv.data(), sv.data() + sv.size(), result);
					results.push_back(result);
				}
			}
			return results;
		}

		bool has_multiple(std::string_view key) const
		{
			auto it = key_hash_to_token_index.find(utils::stringhash(key));
			return it != key_hash_to_token_index.end() and it->second.size() > 1;
		}

		template<typename T>
		void set(std::string_view key, T value)
		{
			// Plain content (no quotes) — used for in-place token replacement
			std::string content;
			if constexpr (std::is_same_v<T, bool>)
				content = value ? "true" : "false";
			else if constexpr (std::is_convertible_v<T, std::string_view>)
				content = std::string{std::string_view{value}};
			else
				content = std::format("{}", value);

			auto it = key_hash_to_token_index.find(utils::stringhash(key));
			if (it != key_hash_to_token_index.end() and not it->second.empty())
			{
				// Update: replace the first occurrence's content only
				auto& val_tok   = tokens[it->second.front()];
				u32   old_start = val_tok.start;
				u32   old_len   = val_tok.length;
				u32   new_len   = as<u32>(utf8::view{std::string_view{content}}.size());

				m_data.replace(old_start, old_len, content);
				val_tok.length = new_len;

				i64 delta = as<i64>(new_len) - as<i64>(old_len);
				if (delta != 0)
				{
					u64 val_idx = it->second.front();
					for (u64 i = val_idx + 1; i < tokens.size(); ++i)
					{
						if (tokens[i].start >= old_start + old_len)
							tokens[i].start = as<u32>(as<i64>(tokens[i].start) + delta);
					}
				}
				return;
			}

			// Add: string values are written with double quotes; others bare
			std::string written_value;
			if constexpr (std::is_convertible_v<T, std::string_view>)
				written_value = std::format("\"{}\"", content);
			else
				written_value = content;

			auto dot = key.find('.');
			if (dot != std::string_view::npos)
			{
				auto section_name = key.substr(0, dot);
				auto key_name     = key.substr(dot + 1);

				utf8::view view(m_data);
				for (u64 i = 0; i < tokens.size(); ++i)
				{
					const auto& tok = tokens[i];
					if (tok.type != TokenType::SECTION)
						continue;

					auto sv     = view.subview(tok.start, tok.length);
					auto sv_str = std::string_view{sv.c_str(), sv.size_in_bytes()};
					if (sv_str != section_name)
						continue;

					// Found matching section; locate last newline before next section or EOF
					u32       last_nl_start  = 0;
					u32       last_nl_length = 0;
					TokenType last_nl_type   = TokenType::NEWLINE_POSIX;
					bool      found_nl       = false;
					for (u64 j = i + 1; j < tokens.size(); ++j)
					{
						if (tokens[j].type == TokenType::SECTION)
							break;
						if (tokens[j].type == TokenType::NEWLINE_POSIX or tokens[j].type == TokenType::NEWLINE_WINDOWS)
						{
							last_nl_start  = tokens[j].start;
							last_nl_length = tokens[j].length;
							last_nl_type   = tokens[j].type;
							found_nl       = true;
						}
					}

					std::string entry = std::format("\n{} = {}", key_name, written_value);
					if (found_nl)
					{
						std::string_view nl_str = (last_nl_type == TokenType::NEWLINE_WINDOWS) ? "\r\n" : "\n";
						m_data.replace(last_nl_start, last_nl_length, entry + std::string{nl_str});
					}
					else
						m_data.append(entry);

					tokens.clear();
					key_hash_to_token_index.clear();
					m_errors.clear();
					parse();
					return;
				}
			}

			// Section not found or key has no section
			if (dot != std::string_view::npos)
			{
				// New section block: append after existing content
				std::string entry = std::format("\n[{}]\n{} = {}\n", key.substr(0, dot), key.substr(dot + 1), written_value);
				m_data.append(entry);
			}
			else
			{
				// Global key: insert after existing globals, before first section
				auto first_section = std::ranges::find_if(tokens, [](const TokenValue& t) { return t.type == TokenType::SECTION; });

				if (first_section == tokens.end())
				{
					// No sections: append to end
					m_data.append(std::format("\n{} = {}\n", key, written_value));
				}
				else
				{
					// Insert before '[', maintaining exactly one blank line before the section
					u32         bracket_pos = first_section->start - 1;
					const char* raw         = m_data.c_str();

					// Count consecutive '\n' immediately before '['
					u32 nl_count = 0;
					while (nl_count < bracket_pos and raw[bracket_pos - 1 - nl_count] == '\n')
						nl_count++;

					std::string entry = std::format("\n{} = {}\n\n", key, written_value);

					if (nl_count == 0)
					{
						// Nothing before '[': prepend with leading newline + blank line
						m_data.insert(m_data.begin(), entry);
					}
					else
					{
						// Replace existing newlines with: separator + entry + blank line
						u32 nl_start = bracket_pos - nl_count;
						m_data.replace(nl_start, nl_count, entry);
					}
				}
			}
			tokens.clear();
			key_hash_to_token_index.clear();
			m_errors.clear();
			parse();
		}

		void set_comment(std::string_view key, std::string_view comment)
		{
			auto it = key_hash_to_token_index.find(utils::stringhash(key));
			if (it == key_hash_to_token_index.end() or it->second.empty())
			{
				dbg::println("set_comment: key '{}' not found, cannot set comment", key);
				return;
			}

			u64        val_idx = it->second.front();
			auto&      val_tok = tokens[val_idx];
			utf8::view view(m_data);

			// Check if there's a COMMENT token immediately after the VALUE
			u64 comment_idx = val_idx + 1;
			while (comment_idx < tokens.size() and
				   (tokens[comment_idx].type == TokenType::NEWLINE_POSIX or tokens[comment_idx].type == TokenType::NEWLINE_WINDOWS))
				comment_idx++;

			if (comment_idx < tokens.size() and tokens[comment_idx].type == TokenType::COMMENT)
			{
				// Replace existing comment
				auto& comment_tok = tokens[comment_idx];
				m_data.replace(comment_tok.start, comment_tok.length, comment);

				// Update token positions after current comment
				i64 delta          = as<i64>(comment.size()) - as<i64>(comment_tok.length);
				comment_tok.length = as<u32>(comment.size());
				if (delta != 0)
				{
					for (u64 i = comment_idx + 1; i < tokens.size(); ++i)
						tokens[i].start = as<u32>(as<i64>(tokens[i].start) + delta);
				}
			}
			else
			{
				// Insert new comment after value token
				u32         insert_pos = val_tok.start + val_tok.length;
				std::string entry      = std::format(" # {}", comment);
				m_data.insert(m_data.begin() + insert_pos, entry);

				// Reparse to update all token positions
				tokens.clear();
				key_hash_to_token_index.clear();
				m_errors.clear();
				parse();
			}
		}

		value_proxy         operator[](std::string_view key) const;
		mutable_value_proxy operator[](std::string_view key);

		auto operator[](u32 index) const
		{
			assert::check(index < tokens.size(), "token index out of bounds");
			return tokens[index];
		}

		auto begin() const { return tokens.begin(); }

		auto end() const { return tokens.end(); }

		auto cbegin() const { return tokens.cbegin(); }

		auto cend() const { return tokens.cend(); }

		utf8::string to_string() const { return m_data; }

		std::span<u8> data() { return m_data.data(); }

		std::span<const u8> data() const { return m_data.data(); }

		auto save(fs::path file = "") -> std::expected<u32, std::string>
		{
			if (file.empty())
				file = filename;
			if (file.empty())
			{
				return std::unexpected("no filename specified");
			}
			return file::write({.filename = file, .buffer = data()});
		}
	};

	// ###############################################################################################

	template<typename T>
	T value_proxy::as() const
	{
		return cfg.get<T>(key);
	}

	template<typename T>
	std::vector<T> value_proxy::as_all() const
	{
		return cfg.get_all<T>(key);
	}

	inline value_proxy::operator bool() const { return as<bool>(); }

	inline value_proxy::operator std::string() const { return as<std::string>(); }

	inline value_proxy::operator int() const { return as<int>(); }

	inline value_proxy::operator float() const { return as<float>(); }

	inline value_proxy::operator double() const { return as<double>(); }

	inline value_proxy config::operator[](std::string_view key) const { return {key, *this}; }

	template<typename T>
	T mutable_value_proxy::as() const
	{
		return cfg.get<T>(key);
	}

	template<typename T>
	std::vector<T> mutable_value_proxy::as_all() const
	{
		return cfg.get_all<T>(key);
	}

	template<typename T>
	mutable_value_proxy& mutable_value_proxy::operator=(T value)
	{
		cfg.set(key, value);
		return *this;
	}

	#warning "Add ip address parsing support and test with config files containing IP addresses, url:port"

	inline mutable_value_proxy::operator bool() const { return as<bool>(); }

	inline mutable_value_proxy::operator std::string() const { return as<std::string>(); }

	inline mutable_value_proxy::operator int() const { return as<int>(); }

	inline mutable_value_proxy::operator float() const { return as<float>(); }

	inline mutable_value_proxy::operator double() const { return as<double>(); }

	inline mutable_value_proxy config::operator[](std::string_view key) { return {key, *this}; }
} // namespace deckard
