export module deckard.as;

import std;
import deckard.debug;
import deckard.types;
import deckard.math.utils;
import deckard.assert;

namespace deckard
{


	template<typename T, typename U>
	void warn_cast_limit([[maybe_unused]] U value, [[maybe_unused]] const std::source_location& loc)
	{
		if constexpr (is_debug_build)
		{
			dbg::trace(loc);


			T casted = static_cast<T>(value);


			if constexpr (std::is_floating_point_v<U>)
			{
				dbg::println(
				  "Unable to cast '{:f}' safely. Target range is {}...{}, cast to '{}'",
				  value,
				  std::numeric_limits<T>::min(),
				  std::numeric_limits<T>::max(),
				  casted);
				return;
			}

			if constexpr (std::is_unsigned_v<U>)
			{
				dbg::println(
				  "Unable to cast '{}' safely. Target range is {}...{}, cast to '{}'",
				  static_cast<u64>(value),
				  std::numeric_limits<T>::min(),
				  std::numeric_limits<T>::max(),
				  casted);
			}
		}
	}

#pragma warning(push)
#pragma warning(disable : 4172) // returning address of local

	export template<typename Ret = void*, bool give_warning = true, typename U>
	[[nodiscard]] constexpr Ret
	as(U u, i32 base = 10, [[maybe_unused]] const std::source_location& loc = std::source_location::current()) noexcept
	{
		U value = u;

		// pointers
		if constexpr (std::is_pointer_v<U> and std::is_pointer_v<Ret>)
		{
			if constexpr (is_debug_build)
			{
				if (value == nullptr)
				{
					dbg::trace(loc);
					dbg::panic("Pointer is null");
				}
			}
			return (Ret)u;
		}

		else if constexpr (std::is_pointer_v<Ret> and std::is_integral_v<U>)
		{
			return reinterpret_cast<Ret>(u);
		}
		else if constexpr (std::is_integral_v<U> and std::is_floating_point_v<Ret>)
		{
			// integer -> float
			return static_cast<Ret>(u);
		}
		else if constexpr (std::is_pointer_v<U> and std::is_same_v<Ret, usize>)
		{
			// pointer -> usize
			return reinterpret_cast<Ret>(u);
		}
		else if constexpr (std::is_floating_point_v<U> and std::is_floating_point_v<Ret>)
		{
			// float to float
			if constexpr (std::is_same_v<U, Ret>)
				return static_cast<Ret>(u);
			else if constexpr (std::is_same_v<U, f64> and std::is_same_v<Ret, f32>)
			{
				Ret new_value = static_cast<Ret>(u);

				if (std::isinf(new_value))
				{
					dbg::println("Casting '{}'(f64) to f32 resulted in INF. Consider using f64 instead", value);
					return new_value;
				}

				if (std::isnan(new_value))
				{
					dbg::println("Casting '{}'(f64) to f32 resulted in NaN. Consider using f64 instead", value);
					return new_value;
				}

				if (static_cast<f64>(new_value) != value)
				{
					dbg::println("Casting '{}'(f64) to '{}'(f32), this may lose precision. Consider using f64 instead.",
								 value,
								 new_value);
				}
				return new_value;
			}
			else
			{

				// f32 -> f64, no problem
				return static_cast<Ret>(u);
			}
		}
		else if constexpr (std::is_enum_v<U> and std::is_integral_v<Ret>)
		{
			// Enum
			return as<Ret>(std::to_underlying(u), base, loc);
		}
		else if constexpr (std::is_integral_v<U> and std::is_integral_v<Ret>)
		{
			// integer -> integer
			if constexpr (is_debug_build)
			{

				if (std::in_range<Ret>(value))
					return static_cast<Ret>(value);

				if constexpr (give_warning)
					warn_cast_limit<Ret>(u, loc);
			}
			return static_cast<Ret>(value);
		}
		else if constexpr (std::is_floating_point_v<U> and std::is_integral_v<Ret>)
		{
			// floating point -> integer
			if (value >= std::numeric_limits<Ret>::min() and value <= std::numeric_limits<Ret>::max())
				return static_cast<Ret>(value);

			if constexpr (is_debug_build)
			{
				if constexpr (give_warning)
					warn_cast_limit<Ret>(value, loc);
			}
			return static_cast<Ret>(u);
		}
		else if constexpr (std::is_arithmetic_v<Ret> and (std::is_same_v<U, char*> or std::is_same_v<U, const char*>))
		{
			// char* -> arithmetic
			std::string_view str{value};
			auto             v = try_to_number<Ret>(str, base);
			if (v)
				return *v;
			if constexpr (is_debug_build)
			{
				if constexpr (give_warning)
				{
					dbg::trace(loc);
				}
			}
			dbg::panic(std::format("Could not convert string to arithmetic: '{}'", str));
		}
		else if constexpr (std::is_arithmetic_v<Ret> and string_like_container<U>)
		{
			// string -> arithmetic
			auto v = try_to_number<Ret>(value, base);
			if (v)
				return *v;
			if constexpr (is_debug_build)
			{
				if constexpr (give_warning)
				{
					dbg::trace(loc);
				}
			}
			dbg::panic(std::format("Could not convert string to arithmetic: '{}'", value));
		}
		else if constexpr (std::is_integral_v<U> and std::is_same_v<Ret, std::string>)
		{
			// integral -> string
			auto v = try_to_string<U>(value, base);
			if (v)
				return *v;

			if constexpr (is_debug_build)
			{
				if constexpr (give_warning)
				{
					dbg::trace(loc);
				}
			}
			dbg::panic(std::format("Could not convert integral to string: {}", value));
		}
		else if constexpr (std::is_same_v<std::span<const u8>, U> or std::is_same_v<std::span<u8>, U>)
		{
			assert::check(value.size() <= sizeof(Ret),
						  std::format("Buffer must have {} bytes, was given {} byte buffer", sizeof(Ret), value.size()));

			std::array<u8, sizeof(Ret)> buffer{};
			std::ranges::copy_n(value.begin(), value.size(), buffer.begin());
			return std::bit_cast<Ret>(buffer);
		}
		else
		{
			if constexpr (is_debug_build)
			{
				if constexpr (give_warning)
				{
					dbg::println("fallback <as> {}({})", loc.file_name(), loc.line());
				}
			}
			return static_cast<Ret>(u);
		}
	}

#pragma warning(pop)

	export template<typename T = u64, typename U>
	T address(const U* address)
	{
		return std::bit_cast<T>(address);
	}

	export consteval auto as_constant(auto value) { return value; }

} // namespace deckard
