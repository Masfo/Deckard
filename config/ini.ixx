export module deckard.ini;


import std;
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


	enum TokenType : u8
	{
		EOL = 0x00,
		SECTION,
		KEY,
		COMMENT,
		STRING,
		NUMBER,
		BOOLEAN,


		EOF = 0xFF,
	};

	using inivalue = std::variant<std::monostate, bool, i64, u64, double, std::string>;


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


// What happens if value written is large, should comment be moved or string split to multirow
#if 0
	export class ini
	{
	public:
		 ini(fs::path filename) 
		 { data = read_text_file(filename);
		 }


		value& operator[](std::string_view key)
		{
			//
		}

		value operator[](std::string_view key) const
		{
			//
		}

		void write() { }

	private:
		std::string        data;
		std::vector<value> kv_map;
		//	lexer::tokenizer   m_tokens;
	};
#endif
	// TODO: read to vector so writeback doesnt change order.

} // namespace deckard::ini
