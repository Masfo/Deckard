export module deckard.ini;


import std;
import deckard.assert;
import deckard.lexer;
import deckard.types;
import deckard.file;
import deckard.utf8;

namespace fs = std::filesystem;

namespace deckard::ini
{

	// TODO: string tokens for ini
	// - EOL tokens to separete comments
	// store in vector, so order preserved
	//
	// Assign token type but do not parse the string

	template<typename T>
	concept codepoint_predicate = requires(T&& v, char32 cp) {
		{ v(cp) } -> std::same_as<bool>;
	};

	export enum struct TokenType : u8 {
		NEW_LINE = 0x00,
		SECTION,
		KEY,
		COMMENT,
		STRING,
		NUMBER,
		BOOLEAN,


		END_OF_FILE = 0xFF,
	};

	export struct Token
	{
		TokenType  type{};
		utf8::view value;

		bool operator==(const Token& other) const
		{
			if (type != other.type)
				return false;
			if (value != other.value)
				return false;
			return true;
		}
	};

	using TokenValue = std::variant<std::monostate, bool, i64, u64, f64, utf8::view>;

	/*
	 * [section] # comment
	 *  integer = 123
	 *  float = 3.14
	 *  string = "hello"
	 *  ip = 10.0.0.12
	 *  date = 2024-12-31
	 *  boolean = true
	 *  ports = [8001,8002,8002]
	 *  target = [ cpu=90, case=55 ]		# target.cpu, target.case, target[0].cpu
	 *  target = [ [cpu=90, case=55], [cpu=12, case=20]]		# target[0].cpu, target[1].cpu


	 * ini["section"]["key"] = "new string"s;
	 * ini["section", "key"]
	 * ini("section", "key")
	 * ini("section", "key", "new value")
	 *
	 * ini("section.key") = "new";
	 * ini["section.key"] = 10;			# section.subsection.key [section.subsection]
	 *
	 *	auto &ref = init("section","key");
	 * ref = 10;
	 * ref = 3.14;
	 * ref = "new string";
	 * ref = true;
	 *
	 */

	enum struct State
	{
		Key,
		Value,
	};

	// What happens if value written is large, should comment be moved or string split to multirow
	export class ini
	{
	private:
		using value = u64;

		utf8::iterator                      it;
		utf8::string                        data;
		std::vector<Token>                  tokens;
		std::unordered_map<utf8::view, u64> token_indexes;
		utf8::string                        current_section;

		u64 line{}, column{};

	private:
		bool eof() const { return it >= data.end(); }

		auto consume(codepoint_predicate auto&& pred) const -> u32
		{
			u32  count = 0;
			auto start = it;

			while (start != data.end())
			{
				if (auto cp = *start; pred(cp))
				{

					start++;
					count++;
				}
				else
				{
					break;
				}
			}

			return count;
		}

		auto consume_until(codepoint_predicate auto&& pred) const -> u32
		{

			u32  count = 0;
			auto start = it;

			while (start)
			{
				if (auto cp = *start; not pred(cp))
				{

					start++;
					count++;
				}
				else
				{
					break;
				}
			}

			return count;
		}

		auto peek(size_t offset = 0) const -> std::optional<char32>
		{
			if (it + offset)
				return *(it + offset);
			return std::nullopt;
		}


	public:
		explicit ini(utf8::view view)
			: data(view)
		{
		}

		explicit ini(fs::path filename) { data = read_text_file(filename); }

		value operator[](std::string_view key) const { return {}; }

		auto at(size_t index) const
		{
			assert::check(index < tokens.size(), "index out of range");

			return tokens[index];
		}

		void write() { }

		void tokenize()
		{
			using namespace utf8::basic_characters;

			auto skip_whitespace = [this]() mutable { it += consume([](char32 cp) { return utf8::is_whitespace(cp); }); };

			auto is_newline = [this]()
			{
				return peek() == LINE_FEED or peek() == CARRIAGE_RETURN or   // posix LF
					   (peek() == CARRIAGE_RETURN and peek(1) == LINE_FEED); // windows CR LF
			};

			//
			it = data.begin();

			State state = State::Key;
			utf8::string key;

			while (it)
			{
				// state

				auto cp = *it;

				if (cp == LINE_FEED)
				{
					it++;

					tokens.push_back({.type = TokenType::NEW_LINE});
					continue;
				}

				if (cp == CARRIAGE_RETURN)
				{
					it++;

					if (it and *it == LINE_FEED)
					{
						it++;
						tokens.push_back({.type = TokenType::NEW_LINE});
						continue;
					}
				}


				if (cp == NUMBER_SIGN)
				{
					it++;
					skip_whitespace();


					auto count = consume_until([](char32 cp) { return cp == LINE_FEED or cp == CARRIAGE_RETURN; });
					auto str   = data.subview(it, count);

					tokens.push_back({.type = TokenType::COMMENT, .value = str});
					it += count;
					continue;
				}

				if (cp == LEFT_SQUARE_BRACKET)
				{
					it++; // skip [
					skip_whitespace();
					auto count      = consume_until([](char32 cp) { return cp == RIGHT_SQUARE_BRACKET; });
					current_section = data.substr(it, count);

					tokens.push_back({.type = TokenType::SECTION, .value = data.subview(it, count)});

					it += count;
					it += 1; // skip ]


					continue;
				}

				if (state == State::Key)
				{
					if (utf8::is_identifier_start(cp))
					{
						u32 count = 0;
						auto start = it;
						while (start)
						{
							if (start and *start == NUMBER_SIGN)
							{
								// no equal sign, not valid key
								it += 1;
								continue;

							}
							if (start and (*start == EQUALS_SIGN))
							{
								count += 1;
								it += 1; // skip '='

								key  = data.substr(it, count);
								state = State::Value;
								it += count;
								continue;
							}

							start += 1;
							count += 1;
						}
						it += count;
						
					}
					continue;
				}

				if (state == State::Value)
				{
					continue;
				}



				if (cp == EQUALS_SIGN)
				{
					it++;
					skip_whitespace();
					auto count = consume_until([](char32 cp) { return cp == LINE_FEED or cp == CARRIAGE_RETURN; });
					auto str   = data.substr(it, count);
					str.prepend(FULL_STOP);
					str.prepend(current_section);

					tokens.push_back({.type = TokenType::KEY, .value = str});
					it += count;
					continue;
				}
				it++;
			}
			tokens.push_back({.type = TokenType::END_OF_FILE});

			int i = 0;
		}

		void save() { }

		bool save_as(const std::filesystem::path path) { return true; }
	};

} // namespace deckard::ini
