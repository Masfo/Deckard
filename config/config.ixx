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

	public:
		explicit config(utf8::view view)
			: data(view)
		{
		}

		explicit config(fs::path f)
		{
			filename = f;
			data     = read_text_file(filename);
		}
	};

} // namespace deckard::config
