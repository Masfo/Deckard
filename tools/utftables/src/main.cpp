
import std;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;


namespace fs = std::filesystem;
using namespace std::string_literals;
using namespace std::string_view_literals;

constexpr std::string_view whitespace_string{" \t\f\n\r\v"};

enum class GeneralCategory : int
{
	// Table 4-4. General Category
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

	Unknown = -1,
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

enum class BiDirectionalCategory : int
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

	Unknown = -1,
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

	return BiDirectionalCategory::Unknown;
}

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

std::string to_upper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return (char)std::toupper(c); });
	return str;
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

template<typename T = i64>
auto to_number(std::string_view input, int base = 10) -> std::optional<T>
{
	T val{};
	auto [ptr, ec]{std::from_chars(input.data(), input.data() + input.size(), val, base)};
	if (ptr == input.data())
		return {};

	return val;
}

std::string replace_all(std::string str, std::string_view from, std::string_view to)
{
	if (from.empty())
		return str;

	size_t pos = 0;
	while ((pos = str.find(from, pos)) != std::string::npos)
	{
		str.replace(pos, from.length(), to);
		pos += to.length();
	}
	return str;
}

std::vector<std::string> split(const std::string_view str, const std::string_view delims = "\n")
{
	auto start = str.find_first_not_of(delims, 0);
	auto stop  = str.find_first_of(delims, start);

	std::vector<std::string> tokens;
	while (std::string::npos != stop || std::string::npos != start)
	{

		auto token = str.substr(start, stop - start);
		tokens.emplace_back(token.empty() ? "" : token);

		start = str.find_first_not_of(delims, stop);
		stop  = str.find_first_of(delims, start);
	}
	return tokens;
}

std::vector<std::string> split_csv(std::string_view str, std::string_view delimiter = ";")
{
	std::vector<std::string> result;
	size_t                   start = 0, end = 0;

	while ((end = str.find(delimiter, start)) != std::string_view::npos)
	{
		result.emplace_back(str.substr(start, end - start));
		start = end + 1;
	}

	result.emplace_back(str.substr(start));

	return result;
}

struct UnicodeDataField
{
	i64                   code_value{-1};                  // 0
	std::string           character_name;                  // 1
	GeneralCategory       category{-1};                    // 2
	i64                   canonical_combining_classes{-1}; // 3
	BiDirectionalCategory bidirectional_category{-1};      // 4
	std::vector<u32>      character_decomposed;            // 5
	i64                   decimal_digit_value{-1};         // 6
	i64                   digit_value{-1};                 // 7
	i64                   numeric_value{-1};               // 8
	i64                   mirrored{-1};                    // 9
	std::string           unicode10_name;                  // 10. unicode 1.0 name
	std::string           comment;                         // 11. 10646 comment field
	i64                   uppercase_mapping{-1};           // 12.
	i64                   lowercase_mapping{-1};           // 13
	i64                   titlecase_mapping{-1};           // 14
};

UnicodeDataField parse_field(std::string_view line)
{
	UnicodeDataField field{};

	auto split_line = split_csv(line);


#if 0

	for (const auto [i, str] : std::views::enumerate(split_line))
		std::println("{}. '{}'", i, str);

#endif


	auto code = to_number<int>(split_line[0], 16);
	// 0
	field.code_value = code ? *code : -1;

	// 1
	field.character_name = split_line[1];

	// 2
	field.category = to_GeneralCategory(split_line[2]);

	// 3
	auto combining                    = to_number<int>(split_line[3]);
	field.canonical_combining_classes = combining ? *combining : -1;

	// 4
	field.bidirectional_category = to_BiDirectionalCategory(split_line[4]);

	// 5
	auto decomp_str = split_line[5];
	if (not decomp_str.empty() and decomp_str[0] != '<')
	{
		auto decompose_split = split(decomp_str, " ");
		for (const auto &dstr : decompose_split)
		{
			auto value = to_number<u32>(dstr, 16);
			field.character_decomposed.push_back(*value);
		}
	}

	// 8
	field.numeric_value = to_number<i64>(split_line[8], 10).value_or(-1z);

	// 10
	field.unicode10_name = split_line[10];

	// 12
	auto uc                 = to_number<int>(split_line[12], 16);
	field.uppercase_mapping = uc ? *uc : -1;

	// 13
	auto lc                 = to_number<int>(split_line[13], 16);
	field.lowercase_mapping = lc ? *lc : -1;

	// 14
	auto tc                 = to_number<int>(split_line[14], 16);
	field.titlecase_mapping = tc ? *tc : -1;


	return field;
}

struct decompose_table
{
	std::vector<u32> to;
	i64              ccc{0};
};

using DecomposeTable = std::map<i64, decompose_table>;

struct char32_range
{
	u64 start;
	u64 end;
};

using Tables   = std::unordered_map<std::string, std::vector<char32_range>>;
using IntTable = std::map<u64, u64>;

using Character  = std::pair<std::string, i64>;
using Characters = std::vector<Character>;

using CharacterNames = std::map<i64, std::string>;

struct RangeCmp
{
	using is_transparent = void;

	bool operator()(const std::pair<i64, i64> &lhs, const std::pair<i64, i64> &rhs) const { return lhs.first < rhs.first; }
};

using Range    = std::pair<i64, i64>;
using Entry    = std::pair<Range, std::string>;
using RangeMap = std::map<Range, std::string, RangeCmp>;

std::optional<Entry> lookup(const RangeMap &m, i64 value)
{
	auto it = m.upper_bound({value, {}});

	if (it != m.begin())
	{
		--it;
		if (value <= it->first.second)
			return *it;
	}
	return {};
}

auto compress_runs(std::vector<char32_range> &input) -> std::vector<char32_range>
{
	std::vector<char32_range> ret;
	ret.reserve(input.size() * 2);

	char32_range current_run{input[0].start, input[0].end};

	for (size_t i = 1; i < input.size(); i++)
	{
		if (input[i].start == current_run.end + 1)
		{
			current_run.end = input[i].end;
		}
		else
		{
			ret.push_back(current_run);
			current_run = input[i];
		}
	}

	ret.push_back(current_run);

	ret.shrink_to_fit();
	return ret;
};

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
	ranges = compress_runs(ranges);
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

void write_lines(CharacterNames &input, fs::path filename)
{
	if (input.empty())
	{
		std::println("No input");
		return;
	}
	std::ofstream f(filename);


	auto skip_between = [&f](i64 start, i64 end, std::string_view name)
	{
		f << "\t\t";
		f << std::format("if(codepoint >= 0x{:04X} and codepoint <= 0x{:04X})\n\t\t\t return \"{}\";\n ", start, end, name);
	};

	auto comment_between = [&](i64 start, i64 end, std::string_view name)
	{
		if (start == end)
		{
			f << "\n\t\t";
			f << std::format("case {:04X}-U+{:04X} is a {} range\n\n", start, end, name);
		}
		else
		{
			f << "\t\t\t";
			f << std::format("// U+{:04X}-U+{:04X} is a {} range\n", start, end, name);
		}
	};


	const RangeMap skip_ranges{
		{{0x3400, 0x4DBF}, "CJK Ideograph Extension A"},
		{{0x4E00, 0x9FFF}, "CJK Ideograph"},
		{{0xAC00, 0xD7A3}, "Hangul Syllable"},
		{{0xD800, 0xDB7F}, "Non Private Use High Surrogate"},
		{{0xDB80, 0xDBFF}, "Private Use High Surrogate"},
		{{0xDC00, 0xDFFF}, "Low Surrogate"},
		{{0xE000, 0xF8FF}, "Private Use"},
		{{0x17000, 0x187FF}, "Tangut Ideograph"},
		{{0x18D00, 0x18D1E}, "Tangut Ideograph Supplement"},
		{{0x20000, 0x2A6DF}, "CJK Ideograph Extension B"},
		{{0x2A700, 0x2B73F}, "CJK Ideograph Extension C"},
		{{0x2B740, 0x2B81D}, "CJK Ideograph Extension D"},
		{{0x2B820, 0x2CEAD}, "CJK Ideograph Extension E"},
		{{0x2CEB0, 0x2EBE0}, "CJK Ideograph Extension F"},
		{{0x2EBF0, 0x2EE5D}, "CJK Ideograph Extension I"},
		{{0x30000, 0x3134A}, "CJK Ideograph Extension G"},
		{{0x31350, 0x323AF}, "CJK Ideograph Extension H"},
		{{0x323B0, 0x33479}, "CJK Ideograph Extension J"},
		{{0xF0000, 0xFFFFD}, "Plane 15 Private Use"},
		{{0x10'0000, 0x10'FFFD}, "Plane 16 Private Use"},
	};


	f << "export module deckard.utf8:names;\n";
	f << "import deckard.types;\n";
	f << "import std;\n";

	f << "\n";
	f << "namespace deckard::utf8\n{\n";
	f << "\texport std::string_view codepoint_to_name(char32 codepoint)\n";
	f << "\t{\n";

	f << "#ifndef _DEBUG\n";
	f << "#else\n";

	for (const auto &[range, name] : skip_ranges)
		skip_between(range.first, range.second, name);

	f << "\n";


	f << "\t\tswitch(codepoint)\n";
	f << "\t\t{\n";


	std::unordered_set<i64> seen;

	for (const auto &i : input)
	{
		auto [id, name] = i;

		if (seen.contains(id))
			continue;

		if (auto vl = lookup(skip_ranges, id); vl)
		{
			auto [r, n] = *vl;
			f << "\t\t\t";
			f << std::format("// U+{:04X}-U+{:04X} is a {} range\n", r.first, r.second, n);
			seen.insert(r.first);
			seen.insert(r.second);
		}
		else
		{
			f << "\t\t\t";
			f << std::format("case 0x{:04X}: return \"{}\";\n", id, name);
		}
		continue;
	}

	f << "\n";
	f << "\t\t\t default: return \"<invalid utf codepoint>\";\n";

	f << "\t\t}\n"; // switch
	f << "\t\t#endif";
	f << "\t}\n";   // codepoint_to_name


	f << "}";       // namespace

	f.flush();
	f.close();
}

void write_lines(Characters &input, fs::path filename)
{
	if (input.empty())
	{
		std::println("No input");
		return;
	}

	std::ranges::sort(input, {}, &Character::second);


	std::ofstream f(filename);

	f << "export module deckard.utf8:characters;\n";
	f << "import deckard.types;\n";
	f << "\n";
	f << "export namespace deckard::utf8::characters\n{\n";


	for (const auto &i : input)
	{
		auto [name, id] = i;

		name = replace_all(name, " ", "_");
		name = replace_all(name, "-", "_");
		name = to_upper(name);

		bool        is_escape = id == 0x5c;
		std::string comment;
		if (id > 0x20)
			comment = std::format("// {0}{1:c}{0}", is_escape ? "'" : "", id);

		f << "\t";
		f << std::format(R"(constexpr char32 {} = u8'\u{:04X}';  {})", name, id, comment);
		f << "\n";
	}
	f << "\n";
	f << "}";

	f.flush();
	f.close();
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
	table      = compress_runs(table);
	collapse_runs(table);

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

	f.flush();
	f.close();
}

void write_lines(const IntTable &table, const std::string &table_name, fs::path filename, bool in_hex = true)
{
	std::ofstream f(filename);


	f << std::format("inline constexpr std::array<char32_range, {}> {}{{{{\n", table.size(), table_name);

	for (const auto [from, to] : table)
	{
		f << "{";
		if (in_hex)
			f << std::format("{:#0x}, {:#0x}", from, to);
		else
			f << std::format("{:#0x}, {}", from, to);

		f << "},\n";
	}
	f << "}};\n";

	f << std::format("export constexpr uint32_t max_{}{{{}}};\n\n", table_name, table.size() - 1);

	f.flush();
	f.close();
}

void write_lines(DecomposeTable table, u32 max_decomposition, fs::path filename)
{
	constexpr size_t RESERVE_COUNT = 5000;
	std::ofstream    f(filename);

	std::vector<std::string> lines;
	lines.reserve(RESERVE_COUNT);

	lines.push_back(std::format("struct decomposed_entry {{\n"));
	lines.push_back(std::format("\tuint32_t codepoint{{}};\n"));
	lines.push_back(std::format("\tuint32_t mapping[{}]{{}};\n", max_decomposition));
	lines.push_back(std::format("\tuint8_t len{{}};\n"));
	lines.push_back(std::format("\tuint8_t ccc{{}};\n"));
	lines.push_back(std::format("}};)\n"));
	lines.push_back("\n");

	// change line 7
	lines.push_back(std::format("constexpr decomposed_entry decompose_table[] = {{\n"));


	std::vector<u32> codepoints;
	codepoints.reserve(RESERVE_COUNT);

	std::vector<std::pair<u32, u64>> compressed_decomposition;
	compressed_decomposition.reserve(RESERVE_COUNT);

	u32 count{};
	for (const auto [codepoint, decomp] : table)
	{
		auto [to, ccc] = decomp;

		std::vector<u32> itable{};
		itable.resize(max_decomposition);

		for (const auto [index, entry] : to | std::views::enumerate)
			itable[index] = entry;

		if (not to.empty() or ccc > 0)
		{
			codepoints.push_back((u32)codepoint);
			++count;

			lines.push_back(std::format("{{ {:#0x}, {{", codepoint));

			for (u32 i = 0; i < max_decomposition; ++i)
			{
				lines.push_back(std::format("{:#0x}{}", itable[i], i < max_decomposition - 1 ? "," : ""));
			}
			u64 v{};
			v |= static_cast<u64>(ccc);             // 8-bits
			v |= static_cast<u64>(to.size()) << 8;  // 2 bits	8
			v |= static_cast<u64>(itable[1]) << 10; // 21 bits	10
			v |= static_cast<u64>(itable[0]) << 31; // 21 bits  31

			u8  dcc   = static_cast<u8>(v & 0xFF);
			u32 dsize = static_cast<u32>((v >> 8) & 0x03);
			u32 dit1  = static_cast<u32>((v >> 10) & 0x1F'FFFF);
			u32 dit0  = static_cast<u32>((v >> 31) & 0x1F'FFFF);

			if (ccc != dcc or dsize != to.size() or dit0 != itable[0] or dit1 != itable[1])
			{
				std::println("{} error", codepoint);
			}

			compressed_decomposition.push_back({(u32)codepoint, v});


			lines.push_back(std::format("}}, {}, {:#0x} }},\n", to.size(), ccc));
		}
	}
	lines.push_back("};\n");

	lines[7] = std::format("constexpr decomposed_entry decompose_table[{}] = {{\n", count);

	lines.push_back("\n\n");

	// compressed version

	lines.push_back("// compressed\n");
	lines.push_back(std::format("struct compresseddecomposed_entry {{\n"));
	lines.push_back(std::format("\tuint32_t codepoint{{}};\n"));
	lines.push_back(std::format("\tuint64_t data{{}};\n"));
	lines.push_back(std::format("}};)\n"));
	lines.push_back("\n");
	lines.push_back(std::format("constexpr compressed_decomposed_entry compressed_decompose_table[{}] = {{\n", count));

	for (const auto &[codepoint, data] : compressed_decomposition)
	{
		lines.push_back(std::format("{{{:#0x}, {:#0x}}},\n", codepoint, data));
	}
	lines.push_back("};\n");


	for (const auto &line : lines)
		f << line;

	f.flush();
	f.close();


	u32 chunks{0};
	for (const auto chunk : codepoints | std::views::chunk_by([](u32 a, u32 b) { return b == a + 1; }))
	{
		// std::println("first: {:#0x}, last {:#0x}", *chunk.begin(), *std::prev(chunk.end()));
		++chunks;
	}
	std::println("Chunk count: {}", chunks);
}

void process_core_properties()
{
	auto lines = read_lines("utf/DerivedCoreProperties.txt");

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
	// write_lines(tables, "Lowercase", "lowercase.ixx");
	// write_lines(tables, "Uppercase", "uppercase.ixx");
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

void generate_trie_tables(std::map<u32, u32> const &ccc_map)
{
	constexpr u32 BLOCK_SIZE       = 64;
	constexpr u32 TOTAL_CODEPOINTS = 0x11'0000;
	constexpr u32 DIRECTORY_SIZE   = TOTAL_CODEPOINTS / BLOCK_SIZE;

	using CccBlock = std::array<u8, BLOCK_SIZE>;


	std::vector<CccBlock> raw_blocks(DIRECTORY_SIZE, CccBlock{});

	u32 min_codepoint{TOTAL_CODEPOINTS};
	u32 max_codepoint{0};

	for (auto const &[codepoint, value] : ccc_map)
	{
		if (codepoint >= TOTAL_CODEPOINTS)
			continue;

		u32 index              = codepoint / BLOCK_SIZE;
		u32 low                = codepoint % BLOCK_SIZE;
		raw_blocks[index][low] = static_cast<u8>(value);

		min_codepoint = std::min(min_codepoint, codepoint);
		max_codepoint = std::max(max_codepoint, codepoint);
	}


	std::vector<CccBlock> unique_blocks;
	std::vector<u8>       directory(DIRECTORY_SIZE, 0);

	unique_blocks.emplace_back(CccBlock{});

	for (size_t i = 0; i < raw_blocks.size(); ++i)
	{
		auto const &current_block = raw_blocks[i];

		if (std::ranges::all_of(current_block, [](u8 v) { return v == 0; }))
		{
			directory[i] = 0;
			continue;
		}

		auto it = std::ranges::find(unique_blocks, current_block);
		if (it != unique_blocks.end())
		{
			directory[i] = static_cast<u8>(std::distance(unique_blocks.begin(), it));
		}
		else
		{
			directory[i] = static_cast<u8>(unique_blocks.size());
			unique_blocks.push_back(current_block);
		}
	}

	std::println("// Generated Meta-Information");
	std::println("// min: U+{:06X}, max: U+{:06X}", min_codepoint, max_codepoint);
	std::println("inline constexpr size_t BLOCK_SIZE     = {};", BLOCK_SIZE);
	std::println("inline constexpr size_t UNIQUE_BLOCKs  = {};\n", unique_blocks.size());

	auto last_nonzero = std::ranges::find_if(directory | std::views::reverse, [](u16 id) { return id != 0; });

	size_t truncated_size = directory.size();
	if (last_nonzero != directory.rend())
		truncated_size = std::distance(directory.begin(), last_nonzero.base());


	std::println("// directory size: {} bytes", truncated_size);
	std::println("inline constexpr std::array<u8, {}> trie_directory = {{", truncated_size);
	for (size_t i = 0; i < truncated_size; i += 16)
	{
		std::print("    ");
		for (size_t j = 0; j < 16 and (i + j) < truncated_size; ++j)
		{
			std::print("{}, ", directory[i + j]);
		}
		std::println("");
	}
	std::println("}};");

	std::println("// data size: {} bytes", unique_blocks.size() * BLOCK_SIZE);
	std::println("inline constexpr std::array<u8, {}> trie_data = {{", unique_blocks.size() * BLOCK_SIZE);
	for (size_t b = 0; b < unique_blocks.size(); ++b)
	{
		std::println("    // Block {}", b);
		for (size_t i = 0; i < BLOCK_SIZE; i += 16)
		{
			std::print("    ");
			for (size_t j = 0; j < 16; ++j)
			{
				std::print("{:3}, ", unique_blocks[b][i + j]);
			}
			std::println("");
		}
	}
	std::println("}};");

	std::println("// total table sizes: {} bytes + {} bytes  = {} bytes",
				 truncated_size,
				 unique_blocks.size() * BLOCK_SIZE,
				 truncated_size + unique_blocks.size() * BLOCK_SIZE);

	std::string ccc_generator = std::format("[[nodiscard]] constexpr auto get_ccc(u32 cp) noexcept -> u8\n"
											"{{\n"
											"	if (cp < {:#0x})\n"
											"		return 0;\n"
											"\n"
											"	if( cp > {:#0x})\n"
											"		return 0;\n"
											"\n"
											"	const u32 block_idx = cp >> 6;\n"
											"	const u32 low_idx   = cp & 63;\n"
											"\n"
											"	if (block_idx >= detail::trie_directory.size()) \n"
											"		return 0;\n"
											"\n"
											"	const u16    block_id = trie_directory[block_idx];\n"
											"	const size_t offset   = (static_cast<size_t>(block_id) << 6) + low_idx;\n"
											"\n"
											"	return trie_data[offset];\n"
											"}}",
											min_codepoint,
											max_codepoint);

	std::println("{}", ccc_generator);
}

// ##########################################

struct TrieTables
{
	std::vector<u32> stage2;
	std::vector<u32> stage1;
	u32              page_size;
	u32              cp_min; // first codepoint covered
	u32              cp_max; // last codepoint covered (inclusive)
};

[[nodiscard]]
auto build_trie(std::map<u32, u32> const &ccc, u32 page_size = 256) -> TrieTables
{
	if (ccc.empty())
		return {};

	u32 const cp_min    = ccc.begin()->first;
	u32 const cp_max    = ccc.rbegin()->first;
	u32 const pg_first  = cp_min / page_size;
	u32 const pg_last   = cp_max / page_size;
	u32 const num_pages = pg_last - pg_first + 1;

	// --- build raw pages (only the covered range) ---
	std::vector<std::vector<u32>> raw(num_pages, std::vector<u32>(page_size, 0));

	for (auto const &[cp, val] : ccc)
		raw[cp / page_size - pg_first][cp % page_size] = val;

	// --- deduplicate ---
	std::vector<u32>                stage2;
	std::vector<u32>                stage1(num_pages);
	std::map<std::vector<u32>, u32> seen;

	for (u32 p = 0; p < num_pages; ++p)
	{
		auto [it, inserted] = seen.emplace(raw[p], static_cast<u32>(stage2.size() / page_size));
		if (inserted)
			stage2.insert(stage2.end(), raw[p].begin(), raw[p].end());
		stage1[p] = it->second;
	}

	return {std::move(stage2), std::move(stage1), page_size, cp_min, cp_max};
}

void emit_cpp_tables(TrieTables const &t, std::string_view name)
{
	u32 const pg_first     = t.cp_min / t.page_size;
	u32 const unique_pages = static_cast<u32>(t.stage2.size()) / t.page_size;

	// --- constants ---
	std::println("constexpr u32 {}_page_size   = {};", name, t.page_size);
	std::println("constexpr u32 {}_cp_min      = 0x{:X};", name, t.cp_min);
	std::println("constexpr u32 {}_cp_max      = 0x{:X};", name, t.cp_max);
	std::println("constexpr u32 {}_pg_first    = {};", name, pg_first);
	std::println("");

	// --- stage1 ---
	std::println("constexpr u32 {}_stage1[{}] = {{", name, t.stage1.size());
	for (u32 i = 0; i < t.stage1.size(); ++i)
	{
		if (i % 16 == 0)
			std::print("    ");
		std::print("{:3}", t.stage1[i]);
		if (i + 1 < t.stage1.size())
			std::print(", ");
		if (i % 16 == 15)
			std::println("");
	}
	std::println("\n}};");
	std::println("");

	// --- stage2 ---
	std::println("constexpr u32 {}_stage2[{}] = {{  // {} unique pages", name, t.stage2.size(), unique_pages);
	for (u32 i = 0; i < t.stage2.size(); ++i)
	{
		if (i % t.page_size == 0)
			std::println("    // page {}", i / t.page_size);
		if (i % 16 == 0)
			std::print("    ");
		std::print("{:3}", t.stage2[i]);
		if (i + 1 < t.stage2.size())
			std::print(", ");
		if (i % 16 == 15)
			std::println("");
	}
	std::println("\n}};");
	std::println("");

	// --- lookup ---
	std::println("[[nodiscard]] auto {}_lookup(u32 cp) -> u32 {{", name);
	std::println("    if (cp < {0}_cp_min || cp > {0}_cp_max) [[unlikely]] return 0;", name);
	std::println("    constexpr u32 shift = std::countr_zero({}_page_size);", name);
	std::println("    constexpr u32 mask  = {}_page_size - 1;", name);
	std::println("    u32 const page   = {0}_stage1[(cp >> shift) - {0}_pg_first];", name);
	std::println("    u32 const offset = cp & mask;", name);
	std::println("    return {}_stage2[page * {}_page_size + offset];", name, name);
	std::println("}}");
}

// ##########################################

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
	// 202F;NARROW NO-BREAK SPACE;Zs;0;CS;<noBreak> 0020;;;;N;;;;;
	// 205F;MEDIUM MATHEMATICAL SPACE;Zs;0;WS;<compat> 0020;;;;N;;;;;
	// 2060;WORD JOINER			;Cf;0;BN;;;;;N;;;;;
	auto lines = read_lines("utf/UnicodeData.txt");

	// https://mothereff.in/utf-8

	// proplist: Pattern_White_Space

	fields.reserve(lines.size());

	if (lines.empty())
		return;

	Tables whitespaces;
	Tables dashes;


	IntTable to_lowercase;
	IntTable to_uppercase;
	IntTable digits;

	Characters     characters;
	CharacterNames character_names;

	DecomposeTable decomposetable;

	// TODO: to_uppercase, to_lowercase mappings
	// 0x41, 0x61  ; LATIN CAPITAL LETTER A -> LATIN SMALL LETTER A

	u32 longest_decomposition = 0;
	i64 first = 0, last = 0;

	std::map<u32, u32> ccc;

	for (const auto &line : lines)
	{
		auto split_line = split_csv(line);
#if 0
		for (const auto &i : split_line)
			std::print("'{}', ", i);
		std::println("");
		auto is = split_line.size();
#endif


		// TODO: detect ranges FIRST-LAST, write them to file
		// like: if (codepoint >= 0x3400 and codepoint <= 0x4DBF) // "CJK Ideograph Extension A";

		UnicodeDataField field{0};
		field = parse_field(line);

		longest_decomposition = std::max(longest_decomposition, (u32)field.character_decomposed.size());

		if (field.canonical_combining_classes > 0)
			ccc[(u32)field.code_value] = (u32)field.canonical_combining_classes;

		//
		// TODO: collect all non-zero CCC,
		//
		// ---------------------------------------------------------------------------------

		decomposetable[field.code_value] = {.to = field.character_decomposed, .ccc = field.canonical_combining_classes};

		int x = 0;


		//

		auto name = field.character_name == "<control>" ? field.unicode10_name : field.character_name;
		if (name.empty())
			name = field.character_name;

		character_names[field.code_value] = name;


		if (field.uppercase_mapping != -1)
			to_uppercase[field.code_value] = field.uppercase_mapping;

		if (field.lowercase_mapping != -1)
			to_lowercase[field.code_value] = field.lowercase_mapping;

		if (field.numeric_value != -1)
			digits[field.code_value] = field.numeric_value;


		// basic characters
		if (field.code_value == 0x09)
			characters.push_back({"character tabulation", field.code_value});

		if (field.code_value == 0x0A)
			characters.push_back({"line feed", field.code_value});
		if (field.code_value == 0x0D)
			characters.push_back({"carriage return", field.code_value});

		if (field.code_value >= 0x20 and field.code_value <= 0x7E)
			characters.push_back({field.character_name, field.code_value});


		fields.emplace_back(field);

		if (field.code_value == 0x3400)
		{
		}

		// no-name ranges
		if (field.character_name.contains("First>"))
			first = field.code_value;

		if (field.character_name.contains("Last>"))
			last = field.code_value;

		if (first != 0 and last != 0)
		{
			std::string rangename = field.character_name.substr(1, field.character_name.find(", Last>") - 1);
			std::println("{{{{0x{:04X}, 0x{:04X}}}, \"{}\"}},", first, last, rangename);
			first = last = 0;
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


	std::println("Longest decomposition: {}", longest_decomposition);

	if (longest_decomposition > 2)
	{
		std::println(std::cerr, "Unexpected decomposition length: {}, should be 2", longest_decomposition);
		return; // or std::terminate(), depending on how fatal this is
	}
	// -----------------------------------

	std::vector<u32> ccc_blocks;
	for (const auto &[codepoint, cc] : ccc)
	{
		ccc_blocks.push_back(codepoint);
		//		std::println("codepoint: {:#0x}, ccc: {}", codepoint, cc);
	}
	std::println("ccc count: {}", ccc.size());


	generate_trie_tables(ccc);

	std::println();
	std::println();
	std::println();
	std::println();


	auto trie = build_trie(ccc);
	emit_cpp_tables(trie, "ccc");


#if 0
	for (const auto chunk : ccc | std::views::values| std::views::chunk_by([](u32 a, u32 b) { return b == a + 1; }))
	{
		std::println("{:#0x}, {:#0x}", *chunk.begin(), *std::prev(chunk.end()));
		chunks++;
	}

	int x = 0;
	// TODO: ccc chunks wrong, should be chunk be ccc
	for (const auto chunk : ccc_blocks | std::views::chunk_by([](u32 a, u32 b) { return b == a + 1; }))
	{
		std::println("{:#0x}, {:#0x}", *chunk.begin(), *std::prev(chunk.end()));
		++chunks;
	}
#endif
	write_lines(decomposetable, longest_decomposition, "decompose_table.ixx");


	// Force include some controls
	whitespaces["whitespace"].push_back({0x09, 0x0D});     // controls
	whitespaces["whitespace"].push_back({0x85, 0x85});     // NEXT LINE
	whitespaces["whitespace"].push_back({0x2060, 0x2060}); // WORD JOINER (like U+00A0)


	std::println("Upper-to-lower: {}", to_lowercase.size());
	std::println("Lower-to-upper: {}", to_uppercase.size());

	write_lines(to_lowercase, "upper_to_lower", "to_lowercase.ixx");
	write_lines(to_uppercase, "lower_to_upper", "to_uppercase.ixx");
	write_lines(digits, "digits", "digits.ixx", false);


	collapse_runs(whitespaces["whitespace"]);
	collapse_runs(dashes["dashes"]);


	write_lines(whitespaces, "whitespace", "whitespaces.ixx");
	write_lines(dashes, "dashes", "dashes.ixx");

	write_lines(characters, "characters.ixx");

	write_lines(character_names, "character_names.ixx");
}

void process_casefolding()
{
	auto lines = read_lines("utf/CaseFolding.txt");

	if (lines.empty())
		return;
}

void process_ascii_character_list()
{
	//
}

int main()
{

	process_core_properties();
	process_unicode_data();
	process_casefolding();

	return 0;
}
