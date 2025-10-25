export module deckard.logger;

import std;
import deckard.types;
import deckard.as;
import deckard.debug;

namespace deckard
{


	class logger final
	{
	private:
		constexpr static u64 BUFFER_LEN = 2_MiB;
		std::vector<u8>  buffer;
		std::atomic<u64> index;

		std::atomic<u64> linecount;


		void push(std::string_view str)
		{
			u64 needed = str.size() + 1;
			u64 idx    = index.fetch_add(needed, std::memory_order_acq_rel);

			if (index + needed > buffer.size() )
				return;

			std::memcpy(buffer.data() + idx, str.data(), str.size());
			buffer[idx + str.size()] = '\n';
			linecount.fetch_add(1);
		}

	public:
		logger()
			: logger(BUFFER_LEN)
		{
		}

		logger(u64 len)
		{
			index = 0;
			buffer.resize(len);
		}

		u64 remaining() const
		{
			u64 total = buffer.size();
			u64 used  = index.load(std::memory_order_acquire);
			return used >= total ? 0 : total - used;
		}

		u64 size() const { return buffer.size(); }

		template<typename... Args>
		void operator()(std::string_view fmt, Args&&... args)
		{
			auto str = std::vformat(fmt, std::make_format_args(args...));

			push(str);
		}


		u64 line_count() const { return linecount.load(std::memory_order_acquire); }

		std::string_view view() const
		{
			size_t n = index.load(std::memory_order_acquire);
			if (n > buffer.size())
				n = buffer.size();
			return std::string_view(as<const char*>(buffer.data()), n);
		}
	};

	export logger logger;

} // namespace deckard
