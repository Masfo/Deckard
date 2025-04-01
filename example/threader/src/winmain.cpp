
import std;
import deckard;
using namespace deckard;
using namespace std::string_view_literals;



i32 deckard_main(std::string_view commandline)
{



	// 1 byte: A   0x41			0x41
	// 2 byte: Ä   0xC4			0xC3 0x84
	// 3 byte: ↥   0x21A8		0xE2 0x86 0xA8
	// 4 byte: 🌍  0x1F30D		0xF0 0x9F 0x8C 0x8D
	std::string buffer("\41\xC3\x84\xE2\x86\xA8\xF0\x9F\x8C\x8D");


	utf8::view a(buffer);

	dbg::println("sizeof({}), {} - {}", sizeof a, a.size(), a.size_in_bytes());

	utf8::view b("\x41\xC3\x84\xE2\x86\xA8\xF0\x9F\x8C\x8D"sv);
	dbg::println("sizeof({}), {} - {}", sizeof b, b.size(), b.size_in_bytes());

	std::array<u8, 10> buffer_arr{0x41, 0xC3, 0x84, 0xE2, 0x86, 0xA8, 0xF0, 0x9F, 0x8C, 0x8D};
	utf8::view c{buffer_arr};
	dbg::println("sizeof({}), {} - {}", sizeof c, c.size(), c.size_in_bytes());


	return 0;
}
