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

	export template<typename Ret, typename U>
	constexpr Ret as(U u, [[maybe_unused]] const std::source_location &loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG

		if constexpr (std::is_integral_v<U> && std::is_integral_v<Ret>)
		{
			if (((u <= static_cast<U>(std::numeric_limits<Ret>::max())) && (u >= static_cast<U>(std::numeric_limits<Ret>::min()))))
			{
				return static_cast<Ret>(u);
			}
			else
			{
				warn_cast_limit<Ret>(u, loc);
				return static_cast<Ret>(u);
			}
		}
		else if constexpr (std::is_floating_point_v<U> && std::is_integral_v<Ret>)
		{
			Ret diff = u - static_cast<Ret>(u);

			if (u - static_cast<Ret>(u) == 0)
			{
				return as<Ret>(static_cast<Ret>(u));
			}
			else
			{
				if constexpr (std::is_signed_v<Ret>)
				{
					std::int64_t value{};
					value = as<Ret>(static_cast<std::int64_t>(u));

					dbg::trace(loc);
					dbg::println("Loss of precision casting '{:f}' to '{}'", u, value);
					return value;
				}
				if constexpr (std::is_unsigned_v<Ret>)
				{
					std::uint64_t value{};
					value = as<Ret>(static_cast<std::uint64_t>(u));

					dbg::trace(loc);
					dbg::println("Loss of precision casting '{:f}' to '{}'", u, value);
					return value;
				}
			}
		}
		else
#endif
		{
			return static_cast<Ret>(u);
		}
	}

} // namespace deckard
