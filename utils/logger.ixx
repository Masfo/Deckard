export module deckard.logger;

import std;
import deckard.types;
import deckard.debug;

namespace deckard
{


	class logger final
	{
	private:
		std::vector<u8>  buffer;
		std::atomic<u64> index;

		void push(std::string_view str)
		{
			u64 needed = str.size() + 1;
			u64 idx    = index.fetch_add(needed, std::memory_order_acq_rel);

			if (index + needed > buffer.size())
			{
				// TODO: full, dump to file?
				return;
			}

			std::memcpy(buffer.data() + idx, str.data(), str.size());
			buffer[index + str.size()] = '\n';
		}

	public:
		logger()
			: logger(1_MiB)
		{
		}

		logger(u64 len)
		{
			index = 0;
			buffer.resize(len);
		}

		u64 size() const { return buffer.size(); }

		template<typename... Args>
		void operator()(std::string_view fmt, Args&&... args)
		{
			auto str = std::vformat(fmt, std::make_format_args(args...));

			push(str);
		}
	};

	export logger logger;

} // namespace deckard
