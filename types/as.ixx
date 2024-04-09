export module deckard.as;

import std;
import deckard.debug;

namespace deckard
{

	template<typename T, typename U>
	void warn_cast_limit(U value, const std::source_location &loc) noexcept
	{
		dbg::trace(loc);
		dbg::println("Could not convert value '{}' safely. Target too small: {} < {} < {} ",
					 value,
					 std::numeric_limits<T>::min(),
					 value,
					 std::numeric_limits<T>::max());
	}

	export template<typename T, typename U>
	constexpr T as(U u, const std::source_location &loc = std::source_location::current()) noexcept
	{


		if constexpr (std::is_integral_v<U> && std::is_integral_v<T>)
		{
			if (((u <= static_cast<U>(std::numeric_limits<T>::max())) && (u >= static_cast<U>(std::numeric_limits<T>::min()))))
			{
				return static_cast<T>(u);
			}
			else
			{
				warn_cast_limit<T>(u, loc);
				return static_cast<T>(u);
			}
		}
		else if constexpr (std::is_floating_point_v<U> && std::is_integral_v<T>)
		{
			T diff = u - static_cast<T>(u);

			if (u - static_cast<T>(u) == 0)
			{
				return as<T>(static_cast<T>(u));
			}
			else
			{
				if constexpr (std::is_signed_v<T>)
				{
					std::int64_t value{};
					value = as<T>(static_cast<std::int64_t>(u));

					dbg::trace(loc);
					dbg::println("Loss of precision casting '{:f}' to '{}'", u, value);
					return value;
				}
				if constexpr (std::is_unsigned_v<T>)
				{
					std::uint64_t value{};
					value = as<T>(static_cast<std::uint64_t>(u));

					dbg::trace(loc);
					dbg::println("Loss of precision casting '{:f}' to '{}'", u, value);
					return value;
				}
			}
		}
		else
		{

			return static_cast<T>(u);
		}
	}

} // namespace deckard
