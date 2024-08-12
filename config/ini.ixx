export module deckard.ini;


import std;
import deckard.lexer;
import deckard.types;

namespace fs = std::filesystem;

namespace deckard::ini
{
	using inivalue = std::variant<std::monostate, bool, i64, u64, double, std::string>;

	struct value
	{
		std::string_view      sectionkey; // section.key
		std::vector<inivalue> value;      // value
		std::string_view      comment;    // # comment
	};

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

	export class ini
	{
	public:
		ini(fs::path filename) { m_tokens.open(filename); }

		value& operator[](std::string_view key) noexcept
		{
			//
		}

		value operator[](std::string_view key) const noexcept
		{
			//
		}

		void write() { }

	private:
		std::vector<value> kv_map;
		lexer::tokenizer   m_tokens;
	};

	// TODO: read to vector so writeback doesnt change order.

} // namespace deckard::ini
