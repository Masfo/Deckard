module deckard.log;

import std;
import deckard.file;
import deckard.types;
import deckard.helpers;

namespace deckard::log
{

	void FileSink::write(std::string_view text)
	{
		if (text.empty())
			return;

		std::scoped_lock lock(mutex);
		if (text.size() > BUFFER_SIZE - offset and not flush_locked())
		{
			ConsoleSink{}.ewrite("log: could not write message after failed buffer flush\n");
			return;
		}

		if (text.size() > BUFFER_SIZE)
		{
			append_locked(to_span(text));
			return;
		}
		std::memcpy(buffer.data() + offset, text.data(), text.size());
		offset += text.size();
	}



	bool FileSink::append_locked(std::span<const u8> bytes)
	{
		if (path.empty())
		{
			clean_older_logfiles();
			path = std::filesystem::current_path() / std::format("log.{}.{}.txt", date_string(), time_string("."));
		}

		while (not bytes.empty())
		{
			const auto count = std::min(bytes.size(), static_cast<std::size_t>(std::numeric_limits<u32>::max()));
			const auto result = file::write({.filename = path, .buffer = bytes.first(count), .mode = file::filemode::append});
			if (not result)
			{
				ConsoleSink{}.ewrite(std::format("log: {}\n", result.error()));
				return false;
			}
			bytes = bytes.subspan(count);
		}
		return true;
	}

} // namespace deckard::log