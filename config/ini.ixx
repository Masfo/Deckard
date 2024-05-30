export module ideckard.ini;


import std;
import deckard.lexer;
import deckard.types;

namespace fs = std::filesystem;

namespace deckard::ini
{
	using inivalue = std::variant<std::monostate, bool, i64, u64, double, std::string>;

	struct value
	{
		inivalue         value;
		std::string_view key; // section.key
		std::string_view comment;
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
		ini(fs::path filename) { }

		value& operator[](std::string_view key) noexcept
		{
			//
			return kv_map.at(key);
		}

		value operator[](std::string_view key) const noexcept
		{
			//
			return kv_map.at(key);
		}

		void write() { }

	private:
		std::unordered_map<std::string_view, value> kv_map;
	};

} // namespace deckard::ini
