export module deckard.bytepool;

import std;
import deckard.types;
import deckard.as;
import deckard.utils.hash;

namespace deckard
{
	export struct Bytepool_Hash
	{
		using is_transparent = void;

		size_t operator()(std::span<const u8> s) const { return utils::hash_values(s); }
	};

	export struct Bytepool_Equal
	{
		using is_transparent = void;

		bool operator()(std::span<const u8> a, std::span<const u8> b) const { return std::ranges::equal(a, b); }
	};

	export class bytepool
	{
	public:
		using byte_span = std::span<const u8>;
		using handle    = size_t;
		static constexpr handle invalid_handle = std::numeric_limits<size_t>::max();

	private:
		std::deque<std::vector<u8>>                                          m_storage;
		std::unordered_map<byte_span, handle, Bytepool_Hash, Bytepool_Equal> m_map;

	public:
		explicit bytepool(size_t initial_capacity = 256) { m_map.reserve(initial_capacity); }

		void reset()
		{
			m_storage.clear();
			m_map.clear();
		}

		[[nodiscard]] byte_span get(handle h) const
		{
			if (h >= m_storage.size())
				return {};
			return std::span(m_storage[h]);
		}

		[[nodiscard]] handle add(byte_span data)
		{
			if (data.empty())
				return invalid_handle;

			if (contains(data))
				return m_map[data];

			handle new_handle = m_storage.size();
			m_storage.emplace_back(data.begin(), data.end());
			m_map.emplace(std::span(m_storage.back()), new_handle);

			return new_handle;
		}

		[[nodiscard]] bool contains(byte_span data) const noexcept { return m_map.contains(data); }

		void merge(bytepool& other)
		{
			if (&other == this)
				return;

			if (other.empty())
				return;

			m_map.reserve(m_map.size() + other.size());

			for (const auto& data : other.m_storage)
				(void)add(std::span(data));
		}

		[[nodiscard]] size_t size() const noexcept { return m_map.size(); }

		[[nodiscard]] bool empty() const noexcept { return m_map.empty(); }
	};

} // namespace deckard
