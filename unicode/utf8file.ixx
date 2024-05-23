export module deckard.utf8:file;
import :codepoints;

import deckard.file;

import std;

namespace fs = std::filesystem;

namespace deckard::utf8
{
	// TODO: forward/output iterator
	// deref, ++

	export class utf8file
	{
	public:
		utf8file(fs::path filename)
			: file(filename)
		{
		}

	private:
		file       file;
		codepoints points;
		char32_t   current_cp{REPLACEMENT_CHARACTER};
	};

} // namespace deckard::utf8
