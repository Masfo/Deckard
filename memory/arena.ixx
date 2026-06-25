export module deckard.memory:arena;
import :helpers;

import std;
import deckard.types;

namespace deckard::memory
{
	template<typename T>
	concept trivially_destructible = std::is_trivially_destructible_v<T>;

	export class arena
	{
	private:
		bytearray m_buffer;
		u64       m_offset{0};
		u64       m_capacity{0};

	public:
		explicit arena(u64 capacity_in_bytes)
			: m_buffer(make_bytearray(capacity_in_bytes))
			, m_capacity(capacity_in_bytes)
		{
		}

		[[nodiscard]] void* allocate(u64 size_in_bytes, u64 align = alignof(std::max_align_t))
		{
			void* base  = m_buffer.get() + m_offset;
			u64   space = m_capacity - m_offset;

			void* aligned = std::align(align, size_in_bytes, base, space);
			if (not aligned)
				return nullptr;

			m_offset = static_cast<std::byte*>(aligned) - m_buffer.get() + size_in_bytes;
			return aligned;
		}

		template<trivially_destructible T>
		[[nodiscard]] std::span<T> allocate_block(u64 count)
		{
			u64   size_in_bytes = sizeof(T) * count;
			
			void* mem           = allocate(size_in_bytes, alignof(T));
			if (not mem)
				return {};

			return std::span<T>(static_cast<T*>(mem), count);
		}

		template<trivially_destructible T, typename... Args>
		[[nodiscard]] T* create(Args&&... args)
		{
			void* mem = allocate(sizeof(T), alignof(T));
			if (not mem)
				return nullptr;

			return std::construct_at(static_cast<T*>(mem), std::forward<Args>(args)...);
		}

		template<trivially_destructible T, typename... Args>
		[[nodiscard]] std::span<T> create_array(u64 count, Args&&... args)
		{
			void* mem = allocate(sizeof(T) * count, alignof(T));
			if (not mem)
				return {};

			std::span<T> result(static_cast<T*>(mem), count);
			for (auto& elem : result)
				std::construct_at(&elem, args...);

			return result;
		}

		void reset() { m_offset = 0; }

		[[nodiscard]] u64 capacity() const { return m_capacity; }

		[[nodiscard]] u64 used() const { return m_offset; }

		[[nodiscard]] u64 free() const { return m_capacity - m_offset; }

		[[nodiscard]] auto data() const { return std::span<const std::byte>(m_buffer.get(), m_offset); }
	};
} // namespace deckard::memory
