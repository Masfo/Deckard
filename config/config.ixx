export module deckard.config;

import deckard.utf8;
import deckard.types;
import deckard.file;

namespace fs = std::filesystem;

namespace deckard::config
{
	export enum struct TokenType : u8 {
		NEWLINE = 0x00,
		NEWLINE_POSIX,
		NEWLINE_WINDOWS,

		SECTION,
		KEY,
		VALUE,
		COMMENT,

		END_OF_FILE = 0xFF,
	};

	// if [, consume_section, until ], if \n or EOF fail error
	

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

	export class config
	{
	private:
		utf8::string data;
		fs::path     filename;
		std::vector<Token> tokens;
		std::expected<utf8::view, std::string> consume_section(utf8::iterator start)
		{
			auto section_start = start + 1; // skip '['
			auto end           = data.end();

			while (start != end && *start != ']')
			{
				if (utf8::is_whitespace(*start))
					return std::unexpected("Invalid section header");
				++start;
			}
			if (start == end)
				return std::unexpected("Invalid section header");
			auto section = data.subview(section_start, start-section_start);
			return section;
		}

		void tokenize()
		{
			if (data.empty())
				return;

			auto start = data.begin();
			auto end   = data.end();
			while (start != end)
			{
				if (*start == '\n' || *start == '\r')
				{
					if (start + 1 != end && *(start + 1) == '\n')
					{
						//tokens.push_back({TokenType::NEWLINE_POSIX, utf8::view(start, start + 2)});
						start += 2;
					}
					else
					{
						//tokens.push_back({TokenType::NEWLINE, utf8::view(start, start + 1)});
						start += 1;
					}
				}
				else if (*start == '[')
				{
					auto section = consume_section(start);
					if (section.has_value())
						tokens.push_back({TokenType::SECTION, section.value()});

					start += section.value().size();
					continue;
				}
				else if (*start == '#')
				{
					auto comment_start = start;
					while (start != end && *start != '\n' && *start != '\r')
						++start;
					//tokens.push_back({TokenType::COMMENT, utf8::view(comment_start, start)});
				}
				else
				{
					auto key_start = start;
					while (start != end && *start != '=' && *start != '\n' && *start != '\r')
						++start;
					//auto key = utf8::view(key_start, start);
					if (start != end && *start == '=')
					{
						++start; // consume '='
						auto value_start = start;
						while (start != end && *start != '\n' && *start != '\r')
							++start;
						//auto value = utf8::view(value_start, start-value_start);
						//tokens.push_back({TokenType::KEY, key});
						//tokens.push_back({TokenType::VALUE, value});
					}
				}
			}
		
		}

	public:
		explicit config(utf8::view view)
			: data(view)
		{
			tokenize();
		}

		explicit config(fs::path f)
		{
			filename = f;
			data     = read_text_file(filename);

			tokenize();
		}
	};

} // namespace deckard::config
