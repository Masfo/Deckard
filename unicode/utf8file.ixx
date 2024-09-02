export module deckard.utf8:file;
import :string;

import std;
import deckard.file;

namespace fs = std::filesystem;

namespace deckard::utf8
{
	class file
	{
	private:
		deckard::file m_file;
		string        m_data;

	public:
		file(fs::path path)
			: m_file(path)
		{
		}
	};

	//	export utf8file operator""_utf8file(const char* filename, size_t) noexcept { return utf8file{filename}; }
} // namespace deckard::utf8
