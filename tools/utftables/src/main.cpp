
import std;

namespace fs = std::filesystem;
using namespace std::string_literals;

constexpr std::string_view whitespace_string{" \t\f\n\r\v"};

struct char32_range
{
	unsigned int start;
	unsigned int end;
};

using Tables = std::unordered_map<std::string, std::vector<char32_range>>;

std::optional<unsigned int> to_char32(std::string_view str)
{
	if (str.empty())
		return {};

	unsigned int val{};
	auto [ptr, ec]{std::from_chars(str.data(), str.data() + str.size(), val, 16)};
	if (ec == std::errc())
		return val;
	return {};
}

std::string_view trim_front(std::string_view s) noexcept
{
	if (s.empty())
		return s;

	s.remove_prefix(s.find_first_not_of(whitespace_string));
	return s;
}

std::string_view trim_back(std::string_view s) noexcept
{
	if (s.empty())
		return s;

	s.remove_suffix(s.size() - s.find_last_not_of(whitespace_string) - 1);
	return s;
}

std::string_view trim(std::string_view s) noexcept
{
	s = trim_front(s);
	return trim_back(s);
};

std::vector<std::string> split(const std::string_view str, const std::string_view delims = "\n")
{
	auto start = str.find_first_not_of(delims, 0);
	auto stop  = str.find_first_of(delims, start);

	std::vector<std::string> tokens;
	while (std::string::npos != stop || std::string::npos != start)
	{
		tokens.emplace_back(str.substr(start, stop - start));

		start = str.find_first_not_of(delims, stop);
		stop  = str.find_first_of(delims, start);
	}
	return tokens;
}

std::vector<std::string> read_lines(fs::path file)
{
	if (not fs::exists(file))
	{
		std::println("File {} does not exist", file.string());
		return {};
	}

	std::vector<std::string> lines;

	std::ifstream ifile(file);
	std::string   str{(std::istreambuf_iterator<char>(ifile)), std::istreambuf_iterator<char>()};

	if (str.empty())
		return {};

	return split(str);
}

void write_lines(const Tables &tables, const std::string &table_name, fs::path filename)
{
	std::ofstream f(filename);

	auto table = tables.at(table_name);

	auto ctable_name = table_name;

	std::transform(
		ctable_name.begin(), ctable_name.end(), ctable_name.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });

	f << std::format("inline constexpr std::array<char32_range, {}> {}{{{{\n", table.size(), ctable_name);
	for (const auto [start, end] : table)
	{
		f << "{";
		f << std::format("{:#0x}, {:#0x}", start, end);
		f << "},\n";
	}
	f << "}};\n";

	f << std::format("constexpr char32_t max_{} = {}[{}].end;\n\n", ctable_name, ctable_name, table.size() - 1);

	f.close();
}

void process_core_properties()
{
	auto lines = read_lines("DerivedCoreProperties.txt");

	if (lines.empty())
		return;

	Tables tables;

	for (const auto &line : lines)
	{
		if (line[0] == '#')
			continue;

		auto spl = split(line, ";");

		auto proptmp  = split(trim(spl[1]), "#");
		auto propname = std::string(trim(proptmp[0]));


		auto ranges = trim(spl[0]);

		auto split_range = split(ranges, "..");


		switch (split_range.size())
		{
			case 1:
			{
				if (auto r = to_char32(split_range[0]); r.has_value())
					tables[propname].push_back({*r, *r});
				break;
			}
			case 2:
			{
				if (auto s = to_char32(split_range[0]); s.has_value())
				{
					if (auto e = to_char32(split_range[1]); e.has_value())
					{
						tables[propname].push_back({*s, *e});
					}
				}
				break;
			}
			default: std::println("split gone wrong"); break;
		}
	}

	write_lines(tables, "Math", "math.ixx");
	write_lines(tables, "Alphabetic", "alphabetic.ixx");
	write_lines(tables, "Lowercase", "lowercase.ixx");
	write_lines(tables, "Uppercase", "Uppercase.ixx");
	write_lines(tables, "Cased", "cased.ixx");
	write_lines(tables, "Case_Ignorable", "case_ignorable.ixx");
	write_lines(tables, "Changes_When_Lowercased", "changes_when_lowercased.ixx");
	write_lines(tables, "Changes_When_Uppercased", "changes_when_uppercased.ixx");
	write_lines(tables, "Changes_When_Titlecased", "changes_when_titlecased.ixx");
	write_lines(tables, "Changes_When_Casefolded", "changes_when_casefolded.ixx");
	write_lines(tables, "Changes_When_Casemapped", "changes_when_casemapped.ixx");
	write_lines(tables, "Default_Ignorable_Code_Point", "default_ignorable_pode_point.ixx");
	write_lines(tables, "Grapheme_Extend", "grapheme_extend.ixx");
	write_lines(tables, "Grapheme_Base", "grapheme_base.ixx");
	write_lines(tables, "Grapheme_Link", "grapheme_link.ixx");
	write_lines(tables, "Grapheme_Extend", "grapheme_extend.ixx");
	write_lines(tables, "Grapheme_Extend", "grapheme_extend.ixx");

	write_lines(tables, "ID_Start", "id_start.ixx");
	write_lines(tables, "ID_Continue", "id_continue.ixx");


	write_lines(tables, "XID_Start", "xid_start.ixx");
	write_lines(tables, "XID_Continue", "xid_continue.ixx");
}

enum GeneralCategory
{
	Lu, // Letter, Uppercase
	Ll, // Letter, Lowercase
	Lt, // Letter,Titlecase
	Mn, // Mark, Non-spacing
	Mc, // Mark, Spacing Combining
	Me, // Mark, Enclosing
	Nd, // Number, Decimal Digit
	Nl, // Number, Letter
	No, // Number, Other
	Zs, // Separator, Space
	Zl, // Seprarator, Line
	Zp, // Separator, Paragraph
	Cc, // Other, Control
	Cf, // Other, Format
	Cs, // Other, Surrogate
	Co, // Other, Private Use
	Cn, // Other, Not Assigned
};

struct Field
{
	int             code_value{-1};                      // 0
	std::string     character_name;                      // 1
	GeneralCategory category;                            // 2
	int             canonical_combining_classes{-1};     // 3
	int             bidirectional_category{-1};          // 4
	int             character_decomposition_mapping{-1}; // 5
	int             decimal_digit_value{-1};             // 6
	int             digit_value{-1};                     // 7
	int             numeric_value{-1};                   // 8
	int             mirrored{-1};                        // 9
	std::string     unicode10_name;                      // 10. unicode 1.0 name
	std::string     comment;                             // 11. 10646 comment field
	int             uppercase_mapping{-1};               // 12.
	int             lowercase_mapping{-1};               // 13
	int             titlecase_mapping{-1};               // 14
};

// 0041;LATIN CAPITAL LETTER A;Lu;0;L;;;;;N;;;;0061;

void process_unicode_data()
{
	// 0.	Code value
	// 1.	Character name
	// 2.	General category
	// 3.	Canonical combining classes
	// 4.	Bidirectional category
	// 5.	Character decomposition mapping
	// 6.	Decimal digit value
	// 7.	Digit value
	// 8.	Numeric value
	// 8.	Mirrored
	// 10.	Unicode 1.0 Name
	// 11.	1'0646 comment field
	// 12.	Uppercase mapping
	// 13.	Lowercase mapping
	// 14.	Titlecase mapping
	//  41	LATIN CAPITAL LETTER A	Lu	0	L					N				61
	//  61	LATIN SMALL LETTER A	Ll	0	L					N			41		41
}

int main()
{

	process_core_properties();

	return 0;
}
