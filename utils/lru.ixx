export module deckard.lru;

import std;
import deckard.assert;
import deckard.types;

namespace deckard
{
	export template<typename Key, typename Value>
	class lru_cache
	{
	private:
		static_assert(std::is_copy_constructible_v<Key>, "Key must be copy constructible");
		static_assert(std::is_copy_constructible_v<Value>, "Value must be copy constructible");
		static_assert(std::is_copy_assignable_v<Value>, "Value must be copy assignable");

		using key_value_pair = std::pair<Key, Value>;
		using list_iterator  = std::list<key_value_pair>::iterator;

		std::list<key_value_pair>              cache_items_list;
		std::unordered_map<Key, list_iterator> cache_items_map;
		u64                                    max_size;

	public:
		explicit lru_cache(u64 max_size)
			: max_size{max_size}
		{
			assert::check(max_size > 0, "lru_cache max_size must be greater than zero");
		}

		void put(Key key, Value value)
		{
			auto it = cache_items_map.find(key);
			if (it != cache_items_map.end())
			{
				it->second->second = std::move(value);
				cache_items_list.splice(cache_items_list.begin(), cache_items_list, it->second);
				return;
			}

			cache_items_list.emplace_front(key, std::move(value));
			cache_items_map.emplace(std::move(key), cache_items_list.begin());

			if (cache_items_map.size() > max_size)
			{
				cache_items_map.erase(cache_items_list.back().first);
				cache_items_list.pop_back();
			}
		}

#ifdef __cpp_lib_optional_ref
#error ("use optional ref instead");
		std::optional<const & Value> get(const Key& key)
#else
		std::optional<const Value> get(const Key& key)
#endif
		{
			if (auto it = cache_items_map.find(key); it != cache_items_map.end())
			{
				cache_items_list.splice(cache_items_list.begin(), cache_items_list, it->second);
				return it->second->second;
			}

			return {};
		}

		[[nodiscard]] bool exists(const Key& key) const { return cache_items_map.find(key) != cache_items_map.end(); }

		[[nodiscard]] u64 size() const { return cache_items_map.size(); }

		[[nodiscard]] u64 capacity() const { return max_size; }

		// iterators

		auto begin() { return cache_items_list.begin(); }

		auto end() { return cache_items_list.end(); }

		auto begin() const { return cache_items_list.begin(); }
		
		auto end() const { return cache_items_list.end(); }
	};

} // namespace deckard

template<typename K, typename V>
struct std::formatter<deckard::lru_cache<K, V>>
{
	constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

	auto format(const deckard::lru_cache<K, V>& cache, std::format_context& ctx) const
	{
		auto out                = ctx.out();
		*out++                  = '{';
		std::string_view prefix = "";
		for (const auto& [key, value] : cache)
		{
			out    = std::format_to(out, "{}{}: {}", prefix, key, value);
			prefix = ", ";
		}
		*out++ = '}';
		return out;
	}
};
