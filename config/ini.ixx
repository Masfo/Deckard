export module deckard.ini;


import std;
import deckard.lexer;
import deckard.types;
import deckard.file;
import deckard.utf8;

namespace fs = std::filesystem;

namespace deckard::ini
{
	struct Section
	{
		utf8::view name;
		u32        index{0};
	};

	struct KeyValue
	{
		Section    section;
		utf8::view key;
		utf8::view value;
		u32        index{0};
	};

	using inivalue = std::variant<std::monostate, bool, i64, u64, double, std::string>;

	struct value
	{
		std::string_view      sectionkey; // section.key
		std::vector<inivalue> value;      // value
		std::string_view      comment;    // # comment
	};

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
										   */

	/*
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
