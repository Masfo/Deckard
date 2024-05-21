export module deckard.as;

import std;
import deckard.debug;
import deckard.types;

namespace deckard
{

	template<typename T, typename U>
	void warn_cast_limit([[maybe_unused]] U value, [[maybe_unused]] const std::source_location& loc) noexcept
	{
#ifdef _DEBUG
		dbg::trace(loc);
		dbg::println(
		  "Unable to cast '{}' safely. Target range is {}...{}", (T)(value), std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
#endif
	}

	export template<typename Ret = void*, typename U>
	constexpr Ret as(U u, [[maybe_unused]] const std::source_location& loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG
		U         value      = u;
		const Ret return_max = std::numeric_limits<Ret>::max();
		const Ret return_min = std::numeric_limits<Ret>::min();
#endif


		// pointers
		if constexpr (std::is_pointer_v<U> and std::is_pointer_v<Ret>)
		{
#ifdef _DEBUG
			if (value == nullptr)
			{
				dbg::trace(loc);
				dbg::panic("Pointer is null");
			}
#endif
			return (Ret)u;
		}
		else if constexpr (std::is_enum_v<U> && std::is_integral_v<Ret>)
		{
			// Enum
			return as<Ret>(std::to_underlying(u));
		}
		else if constexpr (std::is_integral_v<U> && std::is_integral_v<Ret>)
		{
// integers
#ifdef _DEBUG
			if (value >= return_min and value <= return_max)
				return static_cast<Ret>(u);

			warn_cast_limit<Ret>(u, loc);
#endif
			return static_cast<Ret>(u);
		}
		else if constexpr (std::is_floating_point_v<U> && std::is_integral_v<Ret>)
		{
			// floating point
#ifdef _DEBUG
			std::int64_t max_cast     = static_cast<std::int64_t>(value);
			Ret          casted_value = static_cast<Ret>(value);

			auto diff = std::abs(max_cast - value);

			// If diff is near zero and it fits
			if (max_cast >= return_min and max_cast <= return_max and diff <= 0.00000001)
				return static_cast<Ret>(u);

			dbg::trace(loc);
			dbg::println("Casting '{:f}' to range of {}...{} resulted in value '{}", value, return_min, return_max, casted_value);


			warn_cast_limit<Ret>(value, loc);
#endif

			return static_cast<Ret>(u);
		}
		else
		{
			return static_cast<Ret>(u);
		}
	}

	export template<typename T = u64, typename U>
	T address(const U* address)
	{
		return std::bit_cast<T>(address);
	}


} // namespace deckard
