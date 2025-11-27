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
#ifdef _DEBUG
		dbg::trace(loc);


		T casted = static_cast<T>(value);



		if constexpr (std::is_floating_point_v<U>)
		{
			dbg::println(
			  "Unable to cast '{:f}' safely. Target range is {}...{}, cast to '{}",
			  value,
			  std::numeric_limits<T>::min(),
			  std::numeric_limits<T>::max(),
			  casted);
			return;
		}

		if constexpr (std::is_unsigned_v<U>)
		{
			dbg::println(
			  "Unable to cast '{}' safely. Target range is {}...{}, cast to '{}",
			  static_cast<u64>(value),
			  std::numeric_limits<T>::min(),
			  std::numeric_limits<T>::max(),
			  casted);
		}
		#if 0
		else if constexpr(std::is_signed_v<U>) // is signed
		{
			dbg::println(
			  "Unable to cast '{}' safely. Target range is {}...{}, casting to '{}'",
			  static_cast<i64>(value),
			  std::numeric_limits<T>::min(),
			  std::numeric_limits<T>::max(),
			  casted);
		}
		#endif
#endif
	}

	export template<typename Ret = void*, typename U>
	constexpr Ret as2(U u, [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
	}

#pragma warning(push)
#pragma warning(disable : 4172) // returning address of local




	export template<typename Ret = void*, bool give_warning = true, typename U>
	constexpr Ret as(U u, i32 base = 10, [[maybe_unused]] const std::source_location& loc = std::source_location::current()) noexcept
	{
		U value = u;

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
		else if constexpr (std::is_pointer_v<Ret> and std::is_integral_v<U>)
		{
			return reinterpret_cast<Ret>(u);
		}
		else if constexpr (std::is_integral_v<U> and std::is_floating_point_v<Ret>)
		{
			// integer -> float
			return static_cast<Ret>(u);
		}
		else if constexpr (std::is_floating_point_v<U> and std::is_floating_point_v<Ret>)
		{
			// float to float
			if constexpr(std::is_same_v<U, Ret>)
				return static_cast<Ret>(u);
			else if constexpr(std::is_same_v<U, f64> and std::is_same_v<Ret, f32>)
			{
				Ret new_value = static_cast<Ret>(u);

				if (std::isinf(new_value))
				{
					dbg::println("Casting '{}'(f64) to f32 resulted in INF. Consider using f64 instead", value);
					return static_cast<Ret>(value);
				}

				if(std::isnan(new_value))
				{
					dbg::println("Casting '{}'(f64) to f32 resulted in NaN. Consider using f64 instead", value);
					return static_cast<Ret>(value);
				}

				dbg::println("Casting '{}'(f64) to '{}'(f32), this may lose precision. Consider using f64 instead.", value, new_value);
				return static_cast<Ret>(value);
			}
			else
			{

				// f32 -> f64, no problem
				return static_cast<Ret>(u);
			}
		}
		else if constexpr (std::is_enum_v<U> && std::is_integral_v<Ret>)
		{
			// Enum
			return as<Ret>(std::to_underlying(u));
		}
		else if constexpr (std::is_integral_v<U> && std::is_integral_v<Ret>)
		{
			// integer -> integer
#ifdef _DEBUG
			if (std::in_range<Ret>(value))
				return static_cast<Ret>(value);

			if constexpr (give_warning)
				warn_cast_limit<Ret>(u, loc);
#endif
			return static_cast<Ret>(value);
		}
		else if constexpr (std::is_floating_point_v<U> && std::is_integral_v<Ret>)
		{
			// floating point -> integer
			Ret t = static_cast<Ret>(value);

			if(value >= std::numeric_limits<Ret>::min() and
			   value <= std::numeric_limits<Ret>::max())
			{
				return t;
			}

#ifdef _DEBUG
			if constexpr (give_warning)
				warn_cast_limit<Ret>(value, loc);
#endif

			return static_cast<Ret>(u);
		}
		else if constexpr (string_like_container<U>)
		{
			// TODO: try_to expected?
			auto v = try_to_number<Ret>(value);
			if (v)
				return as<Ret>(*v);
#ifdef _DEBUG
			if constexpr (give_warning)
			{
				dbg::trace(loc);
			}
#endif
			dbg::panic("Could not convert input from string");
		}
		else if constexpr (std::is_integral_v<U> and std::is_same_v<Ret, std::string>)
		{
			auto v = try_to_string<U>(value, base);
			if (v)
				return *v;
#ifdef _DEBUG
			if constexpr (give_warning)
			{
				dbg::trace(loc);
			}
#endif
			dbg::panic("Could not convert input to string");
		}
		else if constexpr (std::is_same_v<std::span<u8>, U> and std::is_arithmetic_v<Ret>)
			{
				assert::check(value.size() >= sizeof(Ret), std::format("Buffer must have {} bytes, was given {} byte buffer", sizeof(Ret), value.size()));

				Ret temp{};

				std::memcpy(&temp, u.data(), sizeof(temp));
				return temp;
				;
			}

		else
		{
#ifdef _DEBUG
			if constexpr (give_warning)
			{
				dbg::println("fallback <as> {}({})", loc.file_name(), loc.line());
			}
#endif
			return static_cast<Ret>(u);
		}
	}

#pragma warning(pop)

	// TODO: variadic to and as
	///		to<i64,i64>(input1, input2)
	//		as<i64,i64>(input1, input2)>
	// to<...>(inputs...)

	export template<typename Ret = void*, typename U>
	constexpr Ret to(U u, i32 base = 10, [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
		U value = u;


		// pointers
		if constexpr (std::is_pointer_v<U> and std::is_pointer_v<Ret>)
		{
#ifdef _DEBUG
			if (value == nullptr)
			{
				dbg::trace(loc);
				dbg::eprintln("Pointer is null");
				return nullptr;
			}
#endif
			return (Ret)u;
		}
		else if constexpr (std::is_enum_v<U> && std::is_integral_v<Ret>)
		{
			// Enum
			return static_cast<Ret>(std::to_underlying(u));
		}
		else if constexpr (std::is_integral_v<U> && std::is_integral_v<Ret>)
		{
// integers
#ifdef _DEBUG
			if (std::in_range<Ret>(value))
				return static_cast<Ret>(value);

			warn_cast_limit<Ret>(u, loc);
#endif
			return static_cast<Ret>(value);
		}
		else if constexpr (std::is_floating_point_v<U> && std::is_integral_v<Ret>)
		{
			// floating point
#ifdef _DEBUG

			std::int64_t max_cast = static_cast<std::int64_t>(value);

			if (std::in_range<Ret>(max_cast))
				return static_cast<Ret>(value);


			warn_cast_limit<Ret>(max_cast, loc);
#endif

			return static_cast<Ret>(u);
		}
		else if constexpr (string_like_container<U>)
		{
			// TODO: try_to expected?
			auto v = try_to_number<Ret>(value);
			if (v)
				return to<Ret>(*v);

			return Ret{0};
		}
		else if constexpr (std::is_integral_v<U> and std::is_same_v<Ret, std::string>)
		{

			auto v = try_to_string<U>(value, base);
			if (v)
				return *v;


			return "";
		}
		else
		{
			return static_cast<Ret>(value);
		}
	}

	export template<typename T = u64, typename U>
	T address(const U* address)
	{
		return std::bit_cast<T>(address);
	}

	export consteval auto as_constant(auto value) { return value; }

} // namespace deckard
