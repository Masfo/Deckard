
import std;

namespace fs = std::filesystem;
using namespace std::string_literals;

constexpr std::string_view whitespace_string{" \t\f\n\r\v"};

enum GeneralCategory : int
{
	Lu, // Letter, Uppercase
	Ll, // Letter, Lowercase
	Lt, // Letter, Titlecase
	Lm, // Letter, modifier
	Lo, // Letter, other

	Mn, // Mark, Non-spacing
	Mc, // Mark, Spacing Combining
	Me, // Mark, Enclosing

	Nd, // Number, Decimal Digit
	Nl, // Number, Letter
	No, // Number, Other

	Pc, // Punctuation, connector
	Pd, // Punctuation, dash
	Ps, // Punctuation, open
	Pe, // Punctuation, close
	Pi, // Punctuation, initial quote
	Pf, // Punctuation, final quote
	Po, // Punctuation, other

	Sm, // Symbol, math
	Sc, // Symbol, currency
	Sk, // Symbol, modifier
	So, // Symbol, other

	Zs, // Separator, Space
	Zl, // Seprarator, Line
	Zp, // Separator, Paragraph

	Cc, // Other, Control
	Cf, // Other, Format
	Cs, // Other, Surrogate
	Co, // Other, Private Use
	Cn, // Other, Not Assigned

	Unknown = 9'9999
};

GeneralCategory to_GeneralCategory(std::string_view input)
{

	if (input == "Lu")
		return GeneralCategory::Lu;
	if (input == "Ll")
		return GeneralCategory::Ll;
	if (input == "Lt")
		return GeneralCategory::Lt;
	if (input == "Lm")
		return GeneralCategory::Lm;
	if (input == "Lo")
		return GeneralCategory::Lo;


	if (input == "Mn")
		return GeneralCategory::Mn;
	if (input == "Mc")
		return GeneralCategory::Mc;
	if (input == "Me")
		return GeneralCategory::Me;

	if (input == "Nd")
		return GeneralCategory::Nd;
	if (input == "Nl")
		return GeneralCategory::Nl;
	if (input == "No")
		return GeneralCategory::No;

	if (input == "Pc")
		return GeneralCategory::Pc;
	if (input == "Pd")
		return GeneralCategory::Pd;
	if (input == "Ps")
		return GeneralCategory::Ps;
	if (input == "Pe")
		return GeneralCategory::Pe;
	if (input == "Pi")
		return GeneralCategory::Pi;
	if (input == "Pf")
		return GeneralCategory::Pf;
	if (input == "Po")
		return GeneralCategory::Po;

	if (input == "Sm")
		return GeneralCategory::Sm;
	if (input == "Sc")
		return GeneralCategory::Sc;
	if (input == "Sk")
		return GeneralCategory::Sk;
	if (input == "So")
		return GeneralCategory::So;

	if (input == "Zs")
		return GeneralCategory::Zs;
	if (input == "Zl")
		return GeneralCategory::Zl;
	if (input == "Zp")
		return GeneralCategory::Zp;
	if (input == "Cc")
		return GeneralCategory::Cc;
	if (input == "Cf")
		return GeneralCategory::Cf;
	if (input == "Cs")
		return GeneralCategory::Cs;
	if (input == "Co")
		return GeneralCategory::Co;
	if (input == "Cn")
		return GeneralCategory::Cn;

	return GeneralCategory::Unknown;
}

enum BiDirectionalCategory : int
{
	L,   // Left-to-Right				LRM, most alphabetic, syllabic, Han ideographs, non-European or non-Arabic digits,...
	R,   // Right-to-Left				RLM, Hebrew alphabet, and related punctuation
	AL,  // Right-to-Left Arabic		ALM, Arabic, Thaana, and Syriac alphabets, most punctuation specific to those scripts, ...
	EN,  // European Number				European digits, Eastern Arabic-Indic digits, ...
	ES,  // European Number Separator	PLUS SIGN, MINUS SIGN
	ET,  // European Number Terminator	DEGREE SIGN, currency symbols, ...
	AN,  // Arabic Number				Arabic-Indic digits, Arabic decimal and thousands separators, ...
	CS,  // Common Number Separator		COLON, COMMA, FULL STOP, NO-BREAK SPACE, ...
	NSM, // Nonspacing Mark				Characters with the General_Category values: Mn (Nonspacing_Mark) and Me (Enclosing_Mark)
	BN,  // Boundary Neutral			Default ignorables, non-characters, and control characters, other than those explicitly given other
		 // types.
	B,   // Paragraph Separator			PARAGRAPH SEPARATOR, appropriate Newline Functions, higher-level protocol paragraph determination
	S,   // Segment Separator			Tab
	WS,  // Whitespace					SPACE, FIGURE SPACE, LINE SEPARATOR, FORM FEED, General Punctuation spaces, ...
	ON,  // Other Neutrals				All other characters, including OBJECT REPLACEMENT CHARACTER
	LRE, // Left-to-Right Embedding		LRE
	LRO, // Left-to-Right Override		LRO
	RLE, // Right-to-Left Embedding		RLE
	RLO, // Right-to-Left Override		RLO
	PDF, // Pop Directional Format		PDF
	LRI, // Left-to-Right Isolate		LRI
	RLI, // Right-to-Left Isolate		RLI
	FSI, // First Strong Isolate		FSI
	PDI, // Pop Directional Isolate		PDI
};

BiDirectionalCategory to_BiDirectionalCategory(std::string_view input)
{
	if (input == "L")
		return BiDirectionalCategory::L;
	if (input == "R")
		return BiDirectionalCategory::R;
	if (input == "AL")
		return BiDirectionalCategory::AL;
	if (input == "EN")
		return BiDirectionalCategory::EN;
	if (input == "ES")
		return BiDirectionalCategory::ES;
	if (input == "ET")
		return BiDirectionalCategory::ET;
	if (input == "AN")
		return BiDirectionalCategory::AN;
	if (input == "CS")
		return BiDirectionalCategory::CS;
	if (input == "NSM")
		return BiDirectionalCategory::NSM;
	if (input == "BN")
		return BiDirectionalCategory::BN;
	if (input == "B")
		return BiDirectionalCategory::B;
	if (input == "S")
		return BiDirectionalCategory::S;
	if (input == "WS")
		return BiDirectionalCategory::WS;
	if (input == "ON")
		return BiDirectionalCategory::ON;
	if (input == "LRE")
		return BiDirectionalCategory::LRE;
	if (input == "LRO")
		return BiDirectionalCategory::LRO;
	if (input == "PDF")
		return BiDirectionalCategory::PDF;
	if (input == "LRI")
		return BiDirectionalCategory::LRI;
	if (input == "RLI")
		return BiDirectionalCategory::RLI;
	if (input == "FSI")
		return BiDirectionalCategory::FSI;
	if (input == "PDI")
		return BiDirectionalCategory::PDI;
}

struct UnicodeDataField
{
	int                   code_value{-1};                      // 0
	std::string           character_name;                      // 1
	GeneralCategory       category;                            // 2
	int                   canonical_combining_classes{-1};     // 3
	BiDirectionalCategory bidirectional_category{-1};          // 4
	int                   character_decomposition_mapping{-1}; // 5
	int                   decimal_digit_value{-1};             // 6
	int                   digit_value{-1};                     // 7
	int                   numeric_value{-1};                   // 8
	int                   mirrored{-1};                        // 9
	std::string           unicode10_name;                      // 10. unicode 1.0 name
	std::string           comment;                             // 11. 10646 comment field
	int                   uppercase_mapping{-1};               // 12.
	int                   lowercase_mapping{-1};               // 13
	int                   titlecase_mapping{-1};               // 14
};

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

template<typename T = int>
auto to_number(std::string_view input, int base = 10) -> std::optional<T>
{
	T val{};
	auto [ptr, ec]{std::from_chars(input.data(), input.data() + input.size(), val, base)};
	if (ec == std::errc{})
		return val;

	return {};
}

std::vector<std::string> split_csv(const std::string &str)
{
	std::vector<std::string> result;
	std::istringstream       iss(str);
	std::string              token;

	while (std::getline(iss, token, ';'))
	{
		result.push_back(token);
	}

	return result;
}

void collapse_runs(std::vector<char32_range> &ranges)
{
	std::ranges::sort(ranges, {}, &char32_range::start);


	for (size_t i = 1; i < ranges.size(); ++i)
	{
		if (ranges[i - 1].end + 1 == ranges[i].start)
		{
			ranges[i - 1].end = ranges[i].end;
			ranges[i]         = ranges[ranges.size() - 1];
			ranges.pop_back();
			--i;
		}
		std::ranges::sort(ranges, {}, &char32_range::start);
	}
	std::ranges::sort(ranges, {}, &char32_range::start);
}

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

	if (not tables.contains(table_name))
	{
		std::println("Table does not contain key \"{}\"", table_name);
		return;
	}
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

void process_unicode_data()
{
	std::vector<UnicodeDataField> fields;


	//  41	LATIN CAPITAL LETTER A	Lu	0	L					N				61
	//  61	LATIN SMALL LETTER A	Ll	0	L					N			41		41
	// 0061;LATIN SMALL LETTER A;Ll;0;L;;;;;N;;;0041;;0041
	// 001C;<control>;Cc;0;B;;;;;N;INFORMATION SEPARATOR FOUR;;;;

	// 0009;<control>			;Cc;0;S;;;;;N;CHARACTER TABULATION;;;;
	// 000A;<control>			;Cc;0;B;;;;;N;LINE FEED (LF);;;;
	// 000B;<control>			;Cc;0;S;;;;;N;LINE TABULATION;;;;
	// 000C;<control>			;Cc;0;WS;;;;;N;FORM FEED (FF);;;;
	// 000D;<control>			;Cc;0;B;;;;;N;CARRIAGE RETURN (CR);;;;
	// 0020;SPACE				;Zs;0;WS;;;;;N;;;;;
	// 001C;<control>			;Cc;0;B;;;;;N;INFORMATION SEPARATOR FOUR;;;;
	// 0085;<control>			;Cc;0;B;;;;;N;NEXT LINE (NEL);;;;
	// 00A0;NO-BREAK SPACE		;Zs;0;CS;<noBreak> 0020;;;;N;NON-BREAKING SPACE;;;;
	// 1680;OGHAM SPACE MARK	;Zs;0;WS;;;;;N;;;;;
	// 2028;LINE SEPARATOR		;Zl;0;WS;;;;;N;;;;;
	// 2029;PARAGRAPH SEPARATOR	;Zp;0;B;;;;;N;;;;;
	// 2060;WORD JOINER			;Cf;0;BN;;;;;N;;;;;
	auto lines = read_lines("UnicodeData.txt");

	fields.reserve(lines.size());

	if (lines.empty())
		return;

	Tables whitespaces;
	Tables dashes;


	for (const auto &line : lines)
	{
		auto split_line = split_csv(line);

		UnicodeDataField field{};

		auto code = to_number(split_line[0], 16);
		if (code)
			field.code_value = *code;

		field.category               = to_GeneralCategory(split_line[2]);
		field.bidirectional_category = to_BiDirectionalCategory(split_line[4]);
		fields.emplace_back(field);

		if (code and *code == 0x2010)
		{
			int k = 0;
		}

		if (field.category == GeneralCategory::Pd)
		{
			dashes["dashes"].push_back({(unsigned int)field.code_value, (unsigned int)field.code_value});
			continue;
		}

		// Whitespaces
		if (field.category == GeneralCategory::Cc)
			continue;

		if (field.bidirectional_category == BiDirectionalCategory::WS or field.category == GeneralCategory::Zs or
			field.category == GeneralCategory::Zl or field.category == GeneralCategory::Zp)
		{
			whitespaces["whitespace"].push_back({(unsigned int)field.code_value, (unsigned int)field.code_value});
			continue;
		}


		//
	}
	// Force inlcude some controls
	whitespaces["whitespace"].push_back({0x09, 0x0D});     // controls
	whitespaces["whitespace"].push_back({0x85, 0x85});     // NEXT LINE
	whitespaces["whitespace"].push_back({0x2060, 0x2060}); // WORD JOINER (like U+00A0)


	collapse_runs(whitespaces["whitespace"]);
	collapse_runs(dashes["dashes"]);


	write_lines(whitespaces, "whitespace", "whitespaces.ixx");
	write_lines(dashes, "dashes", "dashes.ixx");


	int k = 0;
}

void process_casefolding()
{
	auto lines = read_lines("CaseFolding.txt");

	if (lines.empty())
		return;
}

int main()
{

	process_core_properties();
	process_unicode_data();
	process_casefolding();

	return 0;
}
