export module deckard.memory:arena;
import :helpers;

import std;
import deckard.types;

namespace deckard::memory
{
	template<typename T>
	concept trivially_destructible = std::is_trivially_destructible_v<T>;

	// ###########################################################################


	export template<size_t N>
	class stackarena
	{
	private:
		std::array<std::byte, N> m_buffer{};
		u64                      m_offset{0};

		[[nodiscard]] void* raw_allocate(u64 size_in_bytes, u64 align = alignof(std::max_align_t))
		{
			void* base    = m_buffer.data() + m_offset;
			u64   space   = N - m_offset;
			void* aligned = std::align(align, size_in_bytes, base, space);
			if (not aligned)
				return nullptr;

			m_offset = static_cast<std::byte*>(aligned) - m_buffer.data() + size_in_bytes;
			return aligned;
		}

	public:
		template<trivially_destructible T = std::byte>
		[[nodiscard]] std::span<T> allocate(u64 count = 1)
		{
			void* mem = raw_allocate(sizeof(T) * count, alignof(T));
			if (not mem)
				return {};
			return {static_cast<T*>(mem), count};
		}

		template<trivially_destructible T, typename... Args>
		[[nodiscard]] T* create(Args&&... args)
		{
			void* mem = raw_allocate(sizeof(T), alignof(T));
			if (not mem)
				return nullptr;

			return std::construct_at(static_cast<T*>(mem), std::forward<Args>(args)...);
		}

		template<trivially_destructible T, typename... Args>
		[[nodiscard]] std::span<T> create_array(u64 count, const Args&... args)
		{
			void* mem = raw_allocate(sizeof(T) * count, alignof(T));
			if (not mem)
				return {};

			std::span<T> result(static_cast<T*>(mem), count);
			for (auto& elem : result)
				std::construct_at(&elem, args...);

			return result;
		}

		void reset() { m_offset = 0; }

		[[nodiscard]] u64 capacity() const { return N; }

		[[nodiscard]] u64 used() const { return m_offset; }

		[[nodiscard]] u64 free() const { return N - m_offset; }
	};

	// ###########################################################################

	export class arena
	{
	private:
		bytearray m_buffer;
		u64       m_offset{0};
		u64       m_capacity{0};

		[[nodiscard]] void* raw_allocate(u64 size_in_bytes, u64 align = alignof(std::max_align_t))
		{
			void* base  = m_buffer.get() + m_offset;
			u64   space = m_capacity - m_offset;

			void* aligned = std::align(align, size_in_bytes, base, space);
			if (not aligned)
				return nullptr;

			m_offset = static_cast<std::byte*>(aligned) - m_buffer.get() + size_in_bytes;
			return aligned;
		}

	public:
		explicit arena(u64 capacity_in_bytes)
			: m_buffer(make_bytearray(capacity_in_bytes))
			, m_capacity(capacity_in_bytes)
		{
		}

		template<trivially_destructible T = std::byte>
		[[nodiscard]] std::span<T> allocate(u64 count = 1)
		{
			void* mem = raw_allocate(sizeof(T) * count, alignof(T));
			if (not mem)
				return {};
			return {static_cast<T*>(mem), count};
		}

		template<trivially_destructible T, typename... Args>
		[[nodiscard]] T* create(Args&&... args)
		{
			void* mem = raw_allocate(sizeof(T), alignof(T));
			if (not mem)
				return nullptr;

			return std::construct_at(static_cast<T*>(mem), std::forward<Args>(args)...);
		}

		template<trivially_destructible T, typename... Args>
		[[nodiscard]] std::span<T> create_array(u64 count, Args&&... args)
		{
			void* mem = raw_allocate(sizeof(T) * count, alignof(T));
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
