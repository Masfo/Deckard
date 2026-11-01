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
	//
	//   // split on multiples of 3
	//   for (const auto& group : nums | group_by([](i64 n) { return n % 3 == 0; }))
	//       ...
	enum class delimiter { drop, trailing, leading };

	template<typename T, delimiter D = delimiter::drop>
	struct group_by_fn : std::ranges::range_adaptor_closure<group_by_fn<T, D>>
	{
		T arg;

		constexpr explicit group_by_fn(T v)
			: arg(std::move(v))
		{
		}

		template<std::ranges::viewable_range R>
		auto operator()(R&& range) const
		{
			using Value = std::ranges::range_value_t<R>;

			// Normalise "value" and "predicate" into one predicate
			auto pred = [&]
			{
				if constexpr (std::predicate<const T&, const Value&>)
					return arg;
				else
					return [v = arg](const Value& e) { return e == v; };
			}();

			if constexpr (D == delimiter::trailing)
			{
				return std::forward<R>(range)
					   | std::views::chunk_by([pred](const auto& a, const auto&) { return not pred(a); });
			}
			else if constexpr (D == delimiter::leading)
			{
				return std::forward<R>(range)
					   | std::views::chunk_by([pred](const auto&, const auto& b) { return not pred(b); });
			}
			else
			{
				return std::forward<R>(range)
					   | std::views::chunk_by([pred](const auto& a, const auto& b) { return not pred(a) and not pred(b); })
					   | std::views::filter([pred](const auto& g) { return not pred(*std::ranges::begin(g)); });
			}
		}
	};

	export template<typename T>
	constexpr auto group_by(T value) -> group_by_fn<T>
	{
		return group_by_fn<T, delimiter::drop>{std::move(value)};
	}

	export template<typename T>
	constexpr auto group_by_trailing(T value) -> group_by_fn<T, delimiter::trailing>
	{
		return group_by_fn<T, delimiter::trailing>{std::move(value)};
	}

	export template<typename T>
	constexpr auto group_by_leading(T value) -> group_by_fn<T, delimiter::leading>
	{
		return group_by_fn<T, delimiter::leading>{std::move(value)};
	}

	// group_by_empty
	//
	// Splits a range on empty elements and drops the resulting empty groups.
	// Empty strings/containers and disengaged optionals are treated as empty.
	//
	//   std::vector<std::string> vs{"100", "200", "", "300"};
	//   for (const auto& group : vs | group_by_empty)
	//       ...
	export constexpr auto group_by_empty = group_by([](const auto& value)
	{
		if constexpr (requires { value.has_value(); })
			return not value.has_value();
		else
			return std::ranges::empty(value);
	});
}
