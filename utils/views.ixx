export module deckard.views;

import std;

namespace deckard
{
	// group_by<T>(value) / group_by(predicate)
	//
	// Splits a range on a specific value, or on any element for which the
	// predicate returns true, and drops the resulting empty groups.
	//
	//   for (const auto& group : vs | group_by<std::string>("hello"))
	//       ...
	//
	//   for (const auto& group : vs | group_by([](const std::string& s) { return s.empty(); }))
	//       ...
	template<typename T>
	struct group_by_fn : std::ranges::range_adaptor_closure<group_by_fn<T>>
	{
		T arg;

		constexpr explicit group_by_fn(T v) : arg(std::move(v)) {}

		template<std::ranges::viewable_range R>
		auto operator()(R&& range) const
		{
			using Value = std::ranges::range_value_t<R>;

			if constexpr (std::predicate<T&, const Value&>)
			{
				auto pred = arg;
				return std::forward<R>(range)
					 | std::views::chunk_by(
						 [pred](const auto& a, const auto& b) { return not pred(a) and not pred(b); })
					 | std::views::filter(
						 [pred](const auto& group) { return not pred(*std::ranges::begin(group)); });
			}
			else
			{
				return std::forward<R>(range) | std::views::split(T{arg})
					 | std::views::filter([](const auto& group) { return not std::ranges::empty(group); });
			}
		}
	};

	export template<typename T>
	constexpr auto group_by(T value) -> group_by_fn<T>
	{
		return group_by_fn<T>{std::move(value)};
	}

	// group_by_empty
	//
	// Splits a range on empty elements and drops the resulting empty groups.
	//
	//   std::vector<std::string> vs{"100", "200", "", "300"};
	//   for (const auto& group : vs | group_by_empty)
	//       ...
	export constexpr auto group_by_empty = group_by([](const auto& value) { return std::ranges::empty(value); });
}
