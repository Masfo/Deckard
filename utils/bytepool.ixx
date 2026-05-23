export module deckard.bytepool;

import std;
import deckard.types;
import deckard.as;
import deckard.utils.hash;


namespace deckard
{
	struct ByteHash
	{
		using is_transparent = void;

		size_t operator()(const std::span<const u8> s) const { return utils::hash_values(s); }
	};

	struct ByteEqual
	{
		using is_transparent = void;

		bool operator()(const std::span<const u8> a, const std::span<const u8> b) const
		{
			return a.size() == b.size() and std::equal(a.begin(), a.end(), b.begin());
		}
	};



	export class byte_pool
	{
	public:
		using byte_span = std::span<const u8>;
		using handle    = u32;

		explicit byte_pool(std::size_t initial_capacity = 4096) { m_map.reserve(256); }

		void reset()
		{
			m_storage.clear();
			m_map.clear();
		}

		u32 count() const noexcept { return as<u32>(m_map.size()); }

		[[nodiscard]] byte_span get(handle h) const
		{
			if (h >= m_storage.size())
				return {};
			return std::span(m_storage[h]);
		}

		[[nodiscard]] handle add(const byte_span data)
		{
			if (data.empty())
				return limits::max<u32>;

			if (contains(data))
				return m_map[data];

			handle new_handle = as<u32>(m_storage.size());
			m_storage.emplace_back(data.begin(), data.end());
			m_map.emplace(std::span(m_storage.back()), new_handle);

			return new_handle;
		}

		[[nodiscard]] bool contains(const byte_span data) const noexcept { return m_map.contains(data); }

		void merge(byte_pool& other)
		{
			if (&other == this)
				return;

			if (other.empty())
				return;

			m_map.reserve(m_map.size() + other.size());

			for (const auto& data : other.m_storage)
			{
				(void)add(std::span(data));
			}

			other.reset();
		}

		[[nodiscard]] std::size_t size() const noexcept { return m_map.size(); }

		[[nodiscard]] bool empty() const noexcept { return m_map.empty(); }

	private:
		std::deque<std::vector<u8>>                                m_storage;
		std::unordered_map<byte_span, handle, ByteHash, ByteEqual> m_map;
	};

}
