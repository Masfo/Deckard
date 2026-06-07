export module deckard.stringpool;

import std;
import deckard.types;
import deckard.bytepool;
import deckard.utf8;
import deckard.debug;
import deckard.assert;

namespace deckard
{

	export class string_pool
	{
	public:
		using string_type = utf8::string;
		using view_type   = utf8::view;

		using handle                           = bytepool::handle;
		static constexpr handle invalid_handle = bytepool::invalid_handle;

	private:
		bytepool m_pool;

		[[nodiscard]] view_type span_to_view(bytepool::byte_span span) const noexcept
		{
			return view_type(std::string_view(reinterpret_cast<const char*>(span.data()), span.size()));
		}

	public:
		explicit string_pool(size_t initial_capacity = 256)
			: m_pool(initial_capacity)
		{
		}

		void reset() noexcept { m_pool.reset(); }

		[[nodiscard]] size_t size() const noexcept { return m_pool.size(); }

		[[nodiscard]] view_type get(handle h) const
		{
			assert::check(h != invalid_handle, "invalid handle");
			auto span = m_pool.get(h);
			assert::check(!span.empty(), "handle out of range");
			return span_to_view(span);
		}

		[[nodiscard]] handle add(view_type str) noexcept
		{
			if (str.empty())
				return invalid_handle;

			return m_pool.add(str.data());
		}

		[[nodiscard]] handle add(std::string_view str) noexcept { return add(utf8::view(str)); }

		[[nodiscard]] bool contains(const view_type str) const noexcept { return m_pool.contains(str.data()); }

		[[nodiscard]] bool contains(std::string_view str) const noexcept { return contains(utf8::view(str)); }

		void merge(string_pool& other) noexcept
		{
			m_pool.merge(other.m_pool);
			other.reset();
		}


		void combine(string_pool& other) noexcept
		{
			m_pool.combine(other.m_pool);
		}


		[[nodiscard]] bool empty() const noexcept { return m_pool.empty(); }

		void dump() const noexcept
		{
			if constexpr (is_debug_build)
			{
				dbg::println("String pool dump ({} entries):", size());
				for (handle i = 0; i < static_cast<handle>(m_pool.size()); ++i)
				{
					auto view = span_to_view(m_pool.get(i));
					dbg::println("  [{}] '{}' - {}", i, view, view.data() | std::views::take(15));
				}
			}
		}
	};
} // namespace deckard
