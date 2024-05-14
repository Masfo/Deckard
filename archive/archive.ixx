module;
#include <sqlite3.h>

export module deckard.archive;

import std;
import deckard.debug;

namespace fs = std::filesystem;

export namespace deckard::archive
{
	// TODO: Pivot to just holding binary data as data pak,
	// TODO: update from filesystem, check whats changed, update only changed files
	// TODO: Compress files before
	// TODO: path, size, compressed size, meta?
	//
	// TODO: db->load_folder("level01"); -> gives vector of handles?
	// TODO: db->load_file("level01/mainscript.txt");

	std::string sql3_version() { return sqlite3_libversion(); }

#if 1
	class file
	{
	public:
		file() = default;

		file(fs::path file) noexcept { open(file); }

		~file() { close(); }

		bool open(fs::path dbfile) noexcept { return true; }

		bool close() noexcept { return true; }

		bool is_open() const noexcept { return false; }


	private:
	};
#endif
} // namespace deckard::archive
