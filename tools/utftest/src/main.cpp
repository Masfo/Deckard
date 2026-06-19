
import std;
import deckard;

using namespace deckard;
namespace fs = std::filesystem;

auto to_codepoints(std::string_view hexline)
{
	auto codepoints = string::split(hexline, " ") | std::views::transform([](std::string_view hex) { return to_number<u32>(hex, 16); }) |
					  std::ranges::to<std::vector<char32>>();
	return codepoints;
}

auto to_codepoints_from_string(utf8::string s) { return utf8::to_codepoints(s); }

utf8::string from_hex_to_utf8(std::string_view hexline) { return utf8::string{to_codepoints(hexline)}; }

utf8::string toNFC(const utf8::string& str) { return deckard::utf8::to_nfc(str); }

utf8::string toNFD(const utf8::string& str) { return deckard::utf8::to_nfd(str); }

int main()
{
	using namespace string;

	auto lines = file::read_lines("utf\\NormalizationTest.txt");

	u32 nfc_count  = 0;
	u32 nfd_count  = 0;
	u32 test_count = 0;

	const auto all_normalize_to = [&](const auto& target, auto fn, const auto&... inputs) { return ((target == fn(inputs)) and ...); };


	for (const auto& [index, line] : lines | std::views::enumerate)
	{
		if (line.empty() or line[0] == '#' or line[0] == '@')
			continue;

		auto columns = split(line, ";") | std::views::take(3) | std::ranges::to<std::vector>();

		if (columns.size() != 3)
		{
			std::println(std::cerr, "Should be three columns");
			break;
		}

		++test_count;

		auto c1 = from_hex_to_utf8(columns[0]);
		auto c2 = from_hex_to_utf8(columns[1]);
		auto c3 = from_hex_to_utf8(columns[2]);

		bool nfc = all_normalize_to(c2, toNFC, c1, c2, c3); //  c2 ==  toNFC(c1) ==  toNFC(c2) ==  toNFC(c3)
		bool nfd = all_normalize_to(c3, toNFD, c1, c2, c3); //  c3 ==  toNFD(c1) ==  toNFD(c2) ==  toNFD(c3)

		if (nfc == false)
		{
			auto c1_nfc = toNFC(from_hex_to_utf8(columns[0]));
			auto c2_nfc = toNFC(from_hex_to_utf8(columns[1]));
			auto c3_nfc = toNFC(from_hex_to_utf8(columns[2]));

			dbg::println("{}. {}", index, line);

			dbg::println("c2      bytes: {}", to_hex_string(c2.data()));
			dbg::println("nfc(c1) bytes: {}", to_hex_string(c1_nfc.data()));
			dbg::println("nfc(c2) bytes: {}", to_hex_string(c2_nfc.data()));
			dbg::println("nfc(c3) bytes: {}", to_hex_string(c3_nfc.data()));

			dbg::println("c2==nfc(c1): {}, c2==nfc(c2): {}, c2==nfc(c3): {}", c2 == c1_nfc, c2 == c2_nfc, c2 == c3_nfc);
		}

		if (nfd == false)
		{
			auto c1_nfd = toNFD(from_hex_to_utf8(columns[0]));
			auto c2_nfd = toNFD(from_hex_to_utf8(columns[1]));
			auto c3_nfd = toNFD(from_hex_to_utf8(columns[2]));

			dbg::println("{}. {}", index, line);

			dbg::println("c2      bytes: {}", to_hex_string(c2.data()));
			dbg::println("nfd(c1) bytes: {}", to_hex_string(c1_nfd.data()));
			dbg::println("nfd(c2) bytes: {}", to_hex_string(c2_nfd.data()));
			dbg::println("nfd(c3) bytes: {}", to_hex_string(c3_nfd.data()));

			dbg::println("c2==nfd(c1): {}, c2==nfd(c2): {}, c2==nfd(c3): {}", c2 == c1_nfd, c2 == c2_nfd, c2 == c3_nfd);
		}

		nfc_count += nfc ? 1 : 0;
		nfd_count += nfd ? 1 : 0;
	}


	dbg::println("NFC: {}/{} passed {}", nfc_count, test_count, nfc_count == test_count ? "✅" : "❌");
	dbg::println("NFD: {}/{} passed {}", nfd_count, test_count, nfd_count == test_count ? "✅" : "❌");


	return 0;
}
