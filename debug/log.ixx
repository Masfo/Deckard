export module deckard.log;
import std;
import deckard.types;
import deckard.as;
import deckard.file;

namespace deckard::log
{
	export struct log_to_file
	{
		std::filesystem::path    path;
		std::vector<std::string> lines;

		explicit log_to_file(std::filesystem::path p = "log.txt")
		{
			path                     = p;
			constexpr u32 linecount  = 1024;
			constexpr u32 linelength = 64;

			lines.reserve(linecount);

			for (int i = 0; i < linelength; ++i)
			{
				lines.emplace_back();
				lines.back().reserve(linelength);
			}
		}

		~log_to_file()
		{
			if (lines.empty())
				return;

			file f(path);
			if (not f)
				return;

			f.write(std::span{as<u8*>(lines[0].data()), lines[0].size()});
			f.close();
		}

		void log(std::string_view msg)
		{
			if (msg.empty())
				return;

			lines.push_back(std::string(msg));
		}
	};


} // namespace deckard::log

