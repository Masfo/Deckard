export module deckard.circular_buffer;

import std;
import deckard.types;
import deckard.assert;

namespace deckard
{
	template<typename T>
	class circular_buffer
	{
	private:
		std::vector<T> buffer;
		u32            head{0};

		u32 capacity{0};

	public:
		explicit circular_buffer(u64 size)
			: buffer(size)
			, capacity(size)
		{
		}

		void push(const T& item)
		{
			buffer[head] = item;
			head         = (head + 1) % capacity;
		}

		void push(T&& item)
		{
			buffer[head] = std::move(item);
			head         = (head + 1) % capacity;
		}

		const T& operator[](u64 index) const { return buffer[(head + index) % capacity]; }

		[[nodiscard]] u32 size() const noexcept { return capacity; }

		void clear() noexcept { head = 0; }
	};

} // namespace deckard
