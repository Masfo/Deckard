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
		auto pointers = allocate_raw_array<T>(size);

		size = std::min(size, list.size());

		size_t i{0};
		auto   current = list.begin();
		while (i < size)
		{
			pointers[i] = *current;
			current += 1;
			i += 1;
		}

		return pointers;
	}

	template<typename T, size_t BUFFER_SIZE = 1024>
	class allocator
	{
	private:
		std::unique_ptr<T[]> buffer;
		size_t               offset;
		size_t               capacity;

		allocator(const allocator&)            = delete;
		allocator& operator=(const allocator&) = delete;

	public:
		using value_type      = T;
		using pointer         = T*;
		using const_pointer   = const T*;
		using reference       = T&;
		using const_reference = const T&;
		using size_type       = std::size_t;
		using difference_type = std::ptrdiff_t;

		allocator()
			: offset(0)
			, capacity(BUFFER_SIZE)
		{
			buffer = allocate_raw_array<T>(BUFFER_SIZE);
		}

		allocator(allocator&& other) noexcept
			: buffer(std::move(other.buffer))
			, offset(other.offset)
			, capacity(other.capacity)
		{
			other.offset = 0;
		}

		allocator& operator=(allocator&& other) noexcept
		{
			if (this != &other)
			{
				buffer       = std::move(other.buffer);
				offset       = other.offset;
				capacity     = other.capacity;
				other.offset = 0;
			}
			return *this;
		}

		~allocator() = default;

		[[nodiscard]] pointer allocate(size_type n)
		{
			if (n == 0)
				return nullptr;

			size_t alloc_size     = n * sizeof(T);
			size_t aligned_offset = (offset + alignof(T) - 1) & ~(alignof(T) - 1);

			if (aligned_offset + alloc_size > capacity)
			{
				return nullptr;
			}

			pointer result = reinterpret_cast<pointer>(buffer + aligned_offset);
			offset         = aligned_offset + alloc_size;
			return result;
		}

		void deallocate(pointer, size_type) noexcept { }

		void reset() noexcept { offset = 0; }

		size_type available() const noexcept { return (offset < capacity) ? capacity - offset : 0; }

		bool operator==(const allocator& other) const noexcept { return buffer == other.buffer; }

		bool operator!=(const allocator& other) const noexcept { return !(*this == other); }

		size_type get_capacity() const noexcept { return capacity; }

		size_type get_used() const noexcept { return offset; }

		template<typename U, typename... Args>
		U& construct(Args&&... args)
		{
			size_t alloc_size     = sizeof(U);
			size_t aligned_offset = (offset + alignof(U) - 1) & ~(alignof(U) - 1);

			if (aligned_offset + alloc_size > capacity)
			{
				std::println("no alloc");
				std::terminate();
			}


			U* ptr = reinterpret_cast<U*>(buffer.get() + aligned_offset);
			offset = aligned_offset + alloc_size;

			return *new (ptr) U(std::forward<Args>(args)...);
		}

		pointer get() const { return buffer.get(); }
	};
} // namespace deckard

export void MyFunc();
