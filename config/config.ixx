export module deckard.config;

import deckard.assert;
import deckard.utf8;
import deckard.types;
import deckard.file;
import deckard.debug;
import deckard.helpers;
import deckard.stringhelper;
import deckard.as;
import deckard.net;
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
		std::vector<T> as_vector() const;

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
		std::vector<T> as_vector() const;

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
		utf8::string                              m_data;
		std::vector<TokenValue>                   tokens;
		std::unordered_map<u64, std::vector<u64>> key_hash_to_token_index;
		std::vector<parse_error>                  m_errors;
		fs::path                                  filename;

		void skip_until_newline(utf8::scanner& scan)
		{
			while (scan and not utf8::is_newline(*scan))
				scan++;
		}

		void skip_whitespace(utf8::scanner& scan)
		{
			using namespace utf8::basic_characters;
			while (scan and (*scan == SPACE or *scan == CHARACTER_TABULATION))
				scan++;
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
					section.assign(view.subview(tok.start, tok.length).as_string_view());
				}
				else if (tok.type == TokenType::KEY and i + 1 < tokens.size())
				{
					auto key_sv  = view.subview(tok.start, tok.length).as_string_view();
					auto key_str = std::string{key_sv};

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

			if (p != end and (*p == '+' or *p == '-'))
				++p;

			const char* int_start = p;
			while (p != end and *p >= '0' and *p <= '9')
				++p;

			if (p == int_start)
				return TokenType::VALUE_STRING;

			if (p == end)
				return TokenType::VALUE_INTEGER;

			if (*p != '.')
				return TokenType::VALUE_STRING;
			++p;

			const char* frac_start = p;
			while (p != end and *p >= '0' and *p <= '9')
				++p;

			if (p == frac_start)
				return TokenType::VALUE_STRING;

			if (p != end and (*p == 'e' or *p == 'E'))
			{
				++p;
				if (p != end and (*p == '+' or *p == '-'))
					++p;
				const char* exp_start = p;
				while (p != end and *p >= '0' and *p <= '9')
					++p;
				if (p == exp_start)
					return TokenType::VALUE_STRING;
			}

			return (p == end) ? TokenType::VALUE_FLOAT : TokenType::VALUE_STRING;
		}

		void parse_key(utf8::scanner& scan)
		{
			using namespace utf8::basic_characters;

			skip_whitespace(scan);

			u32 char_start = 0;
			{
				utf8::scanner counting_scan(m_data);
				while (counting_scan and counting_scan.byte_position() < scan.byte_position())
				{
					counting_scan++;
					char_start++;
				}
			}

			u32  char_len          = 0;
			auto is_valid_key_char = [](char32 cp)
			{
				if (cp == EQUALS_SIGN or cp == LEFT_SQUARE_BRACKET or cp == HASH)
					return false;
				if (utf8::is_whitespace(cp) or utf8::is_newline(cp))
					return false;
				return utf8::is_xid_continue(cp);
			};

			while (scan and is_valid_key_char(*scan))
			{
				scan++;
				char_len++;
			}

			tokens.push_back({TokenType::KEY, char_start, char_len});

			skip_whitespace(scan);
			if (not scan.expect(EQUALS_SIGN))
			{
				skip_until_newline(scan);
				return;
			}
		}

		void parse_value(utf8::scanner& scan)
		{
			using namespace utf8::basic_characters;

			skip_whitespace(scan);
			if (not scan)
				return;

			u32 start = 0;
			{
				utf8::scanner counting_scan(m_data);
				while (counting_scan and counting_scan.byte_position() < scan.byte_position())
				{
					counting_scan++;
					start++;
				}
			}

			if (*scan == QUOTATION_MARK)
			{
				scan++;
				start++;

				u32 codepoints = 0;
				while (scan and *scan != QUOTATION_MARK and not utf8::is_newline(*scan))
				{
					scan++;
					codepoints++;
				}

				if (scan and *scan == QUOTATION_MARK)
					scan++;
				else
					m_errors.push_back({"missing closing quotation mark for value string"});

				tokens.push_back({TokenType::VALUE_STRING, start, codepoints});
				return;
			}

			u32 codepoints     = 0;
			u32 trailing_space = 0;

			while (scan and not utf8::is_newline(*scan))
			{
				if (*scan == HASH)
					break;

				if (*scan == SPACE or *scan == CHARACTER_TABULATION)
					trailing_space++;
				else
					trailing_space = 0;

				scan++;
				codepoints++;
			}

			u32 len = codepoints - trailing_space;

			utf8::view       view(m_data);
			std::string_view val_sv = view.subview(start, len).as_string_view();
			TokenType        type   = classify_value(val_sv);

			tokens.push_back({type, start, len});
		}

		void parse_comment(utf8::scanner& scan)
		{
			using namespace utf8::basic_characters;

			if (not scan.expect(HASH))
				return;

			skip_whitespace(scan);

			u32 start = 0;
			{
				utf8::scanner counting_scan(m_data);
				while (counting_scan and counting_scan.byte_position() < scan.byte_position())
				{
					counting_scan++;
					start++;
				}
			}

			u32 codepoints     = 0;
			u32 trailing_space = 0;

			while (scan and not utf8::is_newline(*scan))
			{
				char32 ch = *scan;
				if (ch == SPACE or ch == CHARACTER_TABULATION)
					trailing_space++;
				else
					trailing_space = 0;

				scan++;
				codepoints++;
			}

			u32 len = codepoints - trailing_space;
			tokens.push_back({TokenType::COMMENT, start, len});
		}

		void parse_section(utf8::scanner& scan)
		{
			using namespace utf8::basic_characters;

			if (not scan.expect(LEFT_SQUARE_BRACKET))
				return;

			u32 start = 0;
			{
				utf8::scanner counting_scan(m_data);
				while (counting_scan and counting_scan.byte_position() < scan.byte_position())
				{
					counting_scan++;
					start++;
				}
			}

			u32 codepoints     = 0;
			u32 trailing_space = 0;

			while (scan and *scan != RIGHT_SQUARE_BRACKET and not utf8::is_newline(*scan))
			{
				char32 ch = *scan;
				if (ch == SPACE or ch == CHARACTER_TABULATION)
					trailing_space++;
				else
					trailing_space = 0;

				scan++;
				codepoints++;
			}

			u32        len                 = codepoints - trailing_space;
			const bool has_closing_bracket = scan.expect(RIGHT_SQUARE_BRACKET);

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

			utf8::scanner scan(m_data);
			u32           current_cp_index = 0;

			while (scan)
			{
				// New lines
				if (*scan == LINE_FEED) // \n
				{
					tokens.push_back({TokenType::NEWLINE_POSIX, current_cp_index, 1});
					scan++;
					current_cp_index++;
					continue;
				}

				if (*scan == CARRIAGE_RETURN) // \r
				{
					u32 start_idx = current_cp_index;
					scan++;
					current_cp_index++;

					if (scan and *scan == LINE_FEED) // \r\n
					{
						tokens.push_back({TokenType::NEWLINE_WINDOWS, start_idx, 2});
						scan++;
						current_cp_index++;
						continue;
					}

					tokens.push_back({TokenType::NEWLINE_POSIX, start_idx, 1});
					continue;
				}

				// Comment
				if (*scan == HASH)
				{
					parse_comment(scan);

					u32 total_cps    = as<u32>(utf8::view(m_data).length());
					current_cp_index = total_cps - as<u32>(scan.remaining());
					continue;
				}

				// Section
				if (*scan == LEFT_SQUARE_BRACKET)
				{
					parse_section(scan);

					u32 total_cps    = as<u32>(utf8::view(m_data).length());
					current_cp_index = total_cps - as<u32>(scan.remaining());
					continue;
				}

				// Whitespace before key is ignored
				if (utf8::is_whitespace(*scan))
				{
					scan++;
					current_cp_index++;
					continue;
				}

				// Key followed by its value string
				if (utf8::is_xid_start(*scan))
				{
					parse_key(scan);
					parse_value(scan);

					u32 total_cps    = as<u32>(utf8::view(m_data).length());
					current_cp_index = total_cps - as<u32>(scan.remaining());
					continue;
				}

				dbg::println("config: unrecognized token starting with '{}'", static_cast<char>(*scan));
				scan++;
				current_cp_index++;
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
					section.assign(sv.as_string_view());
				}
				else if (tok.type == TokenType::KEY and i + 1 < tokens.size())
				{
					auto key_sv   = view.subview(tok.start, tok.length);
					auto key_str  = std::string{key_sv.as_string_view()};
					auto full_key = section.empty() ? key_str : section + '.' + key_str;
					auto hash     = utils::stringhash(full_key);
					auto it       = key_hash_to_token_index.find(hash);
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
			std::string_view sv = view.subview(tok.start, tok.length).as_string_view();

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
			else if constexpr (std::is_same_v<T, deckard::net::endpoint>)
			{
				if (sv.empty())
					return {};

				auto host_and_port = string::split_once(sv, ":");

				const auto port = try_to_number<u16>(host_and_port[1]);
				return net::endpoint(host_and_port[0], port.value_or(0));
			}
			else
				return T{};
		}

		template<typename T>
		std::vector<T> get_vector(std::string_view key) const
		{
			auto it = key_hash_to_token_index.find(utils::stringhash(key));
			if (it == key_hash_to_token_index.end())
				return {};

			std::vector<T> results;
			results.reserve(it->second.size());
			utf8::view view(m_data);
			for (u64 idx : it->second)
			{
				const auto&      tok = tokens[idx];
				std::string_view sv  = view.subview(tok.start, tok.length).as_string_view();

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
				else if constexpr (std::is_same_v<T, deckard::net::endpoint>)
				{
					if (sv.empty())
						continue;

					auto host_and_port = string::split_once(sv, ":");
					auto port          = try_to_number<u16>(host_and_port[1]);
					results.push_back(net::endpoint(host_and_port[0], port.value_or(0)));
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
				auto& val_tok = tokens[it->second.front()];

				utf8::scanner scan(m_data);
				u32           cp_idx     = 0;
				u32           byte_start = 0;
				u32           byte_end   = 0;

				while (scan)
				{
					if (cp_idx == val_tok.start)
						byte_start = as<u32>(scan.byte_position());

					scan++;
					cp_idx++;

					if (cp_idx == val_tok.start + val_tok.length)
					{
						byte_end = as<u32>(scan.byte_position());
						break;
					}
				}

				m_data.replace(byte_start, byte_end - byte_start, content);

				u32 new_cp_len = as<u32>(utf8::view(content).length());
				i64 delta      = as<i64>(new_cp_len) - as<i64>(val_tok.length);
				val_tok.length = new_cp_len;

				if (delta != 0)
				{
					u64 val_idx = it->second.front();
					for (u64 i = val_idx + 1; i < tokens.size(); ++i)
					{
						if (tokens[i].start >= val_tok.start + val_tok.length)
							tokens[i].start = as<u32>(as<i64>(tokens[i].start) + delta);
					}
				}
				return;
			}

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

				std::string_view base_view = m_data.as_string_view();
				for (u64 i = 0; i < tokens.size(); ++i)
				{
					const auto& tok = tokens[i];
					if (tok.type != TokenType::SECTION)
						continue;

					auto sv_str = base_view.substr(tok.start, tok.length);
					if (sv_str != section_name)
						continue;

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

			if (dot != std::string_view::npos)
			{
				std::string entry = std::format("\n[{}]\n{} = {}\n", key.substr(0, dot), key.substr(dot + 1), written_value);
				m_data.append(entry);
			}
			else
			{
				auto first_section =
				  std::ranges::find_if(tokens, [](const TokenValue& t) { return t.type == TokenType::SECTION; });

				if (first_section == tokens.end())
				{
					m_data.append(std::format("\n{} = {}\n", key, written_value));
				}
				else
				{
					u32              bracket_pos = first_section->start - 1;
					std::string_view raw         = m_data.as_string_view();
					u32              nl_count    = 0;

					while (nl_count < bracket_pos and raw[bracket_pos - 1 - nl_count] == '\n')
						nl_count++;

					std::string entry = std::format("\n{} = {}\n\n", key, written_value);
					if (nl_count == 0)
					{
						auto updated_buffer = entry + std::string{raw};
						m_data              = updated_buffer;
					}
					else
					{
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

			u64 comment_idx = val_idx + 1;
			while (comment_idx < tokens.size() and (tokens[comment_idx].type == TokenType::NEWLINE_POSIX or
													tokens[comment_idx].type == TokenType::NEWLINE_WINDOWS))
				comment_idx++;
			if (comment_idx < tokens.size() and tokens[comment_idx].type == TokenType::COMMENT)
			{
				auto& comment_tok = tokens[comment_idx];
				m_data.replace(comment_tok.start, comment_tok.length, comment);
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
				u32         insert_pos = val_tok.start + val_tok.length;
				std::string entry      = std::format(" # {}", comment);
				m_data.insert(m_data.begin() + insert_pos, entry);
				tokens.clear();
				key_hash_to_token_index.clear();
				m_errors.clear();
				parse();
			}
		}

		std::generator<std::string> sections() const
		{
			utf8::view                      view(m_data);
			std::unordered_set<std::string> seen;

			for (const auto& tok : tokens)
			{
				if (tok.type != TokenType::SECTION)
					continue;

				auto s = std::string{view.subview(tok.start, tok.length).as_string_view()};
				if (seen.insert(s).second)
				{
					co_yield s;
				}
			}
		}

		std::generator<std::string> keys(std::string_view section = "") const
		{
			utf8::view  view(m_data);
			std::string current_section;
			bool        in_target = section.empty();

			for (const auto& tok : tokens)
			{
				if (tok.type == TokenType::SECTION)
				{
					current_section.assign(view.subview(tok.start, tok.length).as_string_view());
					in_target = (current_section == section);
					continue;
				}

				if (not in_target)
					continue;

				if (tok.type == TokenType::KEY)
				{
					co_yield std::string{view.subview(tok.start, tok.length).as_string_view()};
				}
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
	std::vector<T> value_proxy::as_vector() const
	{
		return cfg.get_vector<T>(key);
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
	std::vector<T> mutable_value_proxy::as_vector() const
	{
		return cfg.get_vector<T>(key);
	}

	template<typename T>
	mutable_value_proxy& mutable_value_proxy::operator=(T value)
	{
		cfg.set(key, value);
		return *this;
	}

	inline mutable_value_proxy::operator bool() const { return as<bool>(); }

	inline mutable_value_proxy::operator std::string() const { return as<std::string>(); }

	inline mutable_value_proxy::operator int() const { return as<int>(); }

	inline mutable_value_proxy::operator float() const { return as<float>(); }

	inline mutable_value_proxy::operator double() const { return as<double>(); }

	inline mutable_value_proxy config::operator[](std::string_view key) { return {key, *this}; }
} // namespace deckard
