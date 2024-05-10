export module deckard.as;

import std;
import deckard.debug;

namespace deckard
{

	template<typename T, typename U>
	void warn_cast_limit(U value, const std::source_location& loc) noexcept
	{
		dbg::trace(loc);
		dbg::println(
		  "Unable to cast '{}' safely. Target range is {}...{}", value, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
	}

	export template<typename Ret, typename U>
	constexpr Ret as(U u, [[maybe_unused]] const std::source_location& loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG

		const Ret return_max = std::numeric_limits<Ret>::max();
		const Ret return_min = std::numeric_limits<Ret>::min();
		const U   value      = u;


		if constexpr (std::is_enum_v<U> && std::is_integral_v<Ret>)
		{
			return as<Ret>(std::to_underlying(u));
		}


		// i
		if constexpr (std::is_integral_v<U> && std::is_integral_v<Ret>)
		{
			if (value >= return_min and value <= return_max)
				return static_cast<Ret>(u);

			warn_cast_limit<Ret>(u, loc);
			return static_cast<Ret>(u);
		}
		else if constexpr (std::is_floating_point_v<U> && std::is_integral_v<Ret>)
		{
			// FP
			std::int64_t max_cast     = static_cast<std::int64_t>(value);
			Ret          casted_value = static_cast<Ret>(value);

			auto diff = std::abs(max_cast - value);

			// If diff is near zero and it fits
			if (max_cast >= return_min and max_cast <= return_max and diff <= 0.00000001)
				return static_cast<Ret>(u);

			dbg::trace(loc);
			dbg::println("Casting '{:f}' to range of {}...{} resulted in value '{}", value, return_min, return_max, casted_value);


			warn_cast_limit<Ret>(value, loc);
			return static_cast<Ret>(u);
		}
		else
#endif
		{
			return static_cast<Ret>(u);
		}
	}

} // namespace deckard
