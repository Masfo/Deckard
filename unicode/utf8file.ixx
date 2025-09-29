export module deckard.utf8:file;
import :string;

import std;
import deckard.file;

namespace fs = std::filesystem;

namespace deckard::utf8
{

	export utf8::string read_utf8_file(fs::path path)
	{
		(path);
		return {};

	}



	//	export utf8file operator""_utf8file(const char* filename, size_t) noexcept { return utf8file{filename}; }
} // namespace deckard::utf8
