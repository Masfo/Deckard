export module deckard.allocator;

import std;

namespace deckard
{
	export template<typename T, typename... Args>
	constexpr std::unique_ptr<T> allocate_raw(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	export template<typename T>
	constexpr std::unique_ptr<T[]> allocate_raw_array(size_t size)
	{
		return std::make_unique<T[]>(size);
	}

	export template<typename T>
	constexpr std::unique_ptr<T[]> allocate_raw_array(size_t size, std::initializer_list<T> list)
	{
		auto pointers = std::make_unique<T[]>(size);

		auto current = list.begin();

		size = std::min(size, list.size());

		size_t i{0};
		while (i < size)
		{
			pointers[i] = *current;
			current += 1;
			i += 1;
		}


		return pointers;
	}

	class bump_allocator
	{
	private:
	public:
	};
} // namespace deckard

export void MyFunc();
