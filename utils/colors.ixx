export module deckard.colors;


import std;
import deckard.types;
import deckard.math.utils;
import deckard.math;
import deckard.utils.hash;

namespace deckard
{
#ifdef __cpp_lib_constexpr_cmath
#error "Use constexpr round";
#endif
	using namespace deckard::literals;

	export constexpr u8 clamp_rgb(f32 color) { return static_cast<u8>((std::clamp(color, 0.0f, 1.0f) * 255.0f) + 0.5f); }

	export constexpr u8 clamp_rgb(i32 color) { return static_cast<u8>(std::clamp(color, 0, 255)); }

	export std::array<f32, 3> to_hsv(u8 r, u8 g, u8 b)
	{
		f32 rr = r;
		f32 gg = g;
		f32 bb = b;

		f32 cmax  = std::max({rr, gg, bb});
		f32 cmin  = std::min({rr, gg, bb});
		f32 delta = cmax - cmin;

		f32 h = 0.0;

		if (delta == 0)
			h = 0;
		else if (cmax == rr)
			h = fmod(((gg - bb) / delta), 6.0f);
		else if (cmax == gg)
			h = ((bb - rr) / delta) + 2.0f;
		else
			h = ((rr - gg) / delta) + 4.0f;

		h *= 60.0f;
		if (h < 0)
			h += 360.0f;

		f32 s = (cmax == 0) ? 0 : (delta / cmax);
		f32 v = cmax / 255.0f;

		return {h, s, v};
	}

	constexpr u8 mul_clamp(u8 x, u8 y)
	{
		i32 prod = (static_cast<i32>(x) * static_cast<i32>(y)) / 255;
		return static_cast<u8>(prod);
	}

	constexpr u8 div_clamp(u8 x, u8 y)
	{
		if (y == 0)
			return 255;

		i32 value = static_cast<i32>((255.0f * x) / y);
		return static_cast<u8>(std::clamp(value, 0, 255));
	}

	constexpr u8 add_clamp(u8 x, u8 y)
	{
		i32 sum = static_cast<i32>(x) + static_cast<i32>(y);
		return static_cast<u8>(std::min(sum, 255));
	}

	constexpr u8 sub_clamp(u8 x, u8 y)
	{
		i32 diff = static_cast<i32>(x) - static_cast<i32>(y);
		return static_cast<u8>(std::max(diff, 0));
	}

	export namespace detail
	{
		template<typename T, u32 Components>
		struct generic_color_storage
		{
			std::array<T, Components> data{};

			constexpr T& operator[](std::size_t index) noexcept { return data[index]; }

			constexpr const T& operator[](std::size_t index) const noexcept { return data[index]; }
		};

		template<typename T>
		struct generic_color_storage<T, 1>
		{
			T r{};

			constexpr T& operator[](std::size_t index) noexcept
			{
				if (index == 0)
					return r;
				std::unreachable();
			}

			constexpr const T& operator[](std::size_t index) const noexcept
			{
				if (index == 0)
					return r;
				std::unreachable();
			}
		};

		template<typename T>
		struct generic_color_storage<T, 3>
		{
			T r{}, g{}, b{};

			constexpr T& operator[](std::size_t index) noexcept
			{
				switch (index)
				{
					case 0: return r;
					case 1: return g;
					case 2: return b;
					default: std::unreachable();
				}
			}

			constexpr const T& operator[](std::size_t index) const noexcept
			{
				switch (index)
				{
					case 0: return r;
					case 1: return g;
					case 2: return b;
					default: std::unreachable();
				}
			}
		};

		template<typename T>
		struct generic_color_storage<T, 4>
		{
			T r{}, g{}, b{}, a{};

			constexpr T& operator[](std::size_t index) noexcept
			{
				switch (index)
				{
					case 0: return r;
					case 1: return g;
					case 2: return b;
					case 3: return a;
					default: std::unreachable();
				}
			}

			constexpr const T& operator[](std::size_t index) const noexcept
			{
				switch (index)
				{
					case 0: return r;
					case 1: return g;
					case 2: return b;
					case 3: return a;
					default: std::unreachable();
				}
			}
		};
	} // namespace detail

	export template<typename T, u32 Components>
	struct generic_color : detail::generic_color_storage<T, Components>
	{
		static_assert((std::same_as<T, u8> or std::same_as<T, f32>) and (Components >= 1 and Components <= 4),
					  "generic_color supports u8 or f32 with 1 to 4 components");

		using value_type                     = T;
		static constexpr u32 component_count = Components;
		using storage_type                   = detail::generic_color_storage<T, Components>;
		using storage_type::operator[];

	private:
		template<typename U>
		static constexpr T convert_component(U value)
		{
			if constexpr (std::same_as<T, u8>)
			{
				if constexpr (std::floating_point<U>)
					return clamp_rgb(static_cast<f32>(value));
				else
					return clamp_rgb(static_cast<i32>(value));
			}
			else
			{
				if constexpr (std::floating_point<U>)
					return std::clamp(static_cast<f32>(value), 0.0f, 1.0f);
				else
					return std::clamp(static_cast<f32>(value) / 255.0f, 0.0f, 1.0f);
			}
		}

		static constexpr T opaque_alpha()
		{
			if constexpr (std::same_as<T, u8>)
				return 255;
			else
				return 1.0f;
		}

		static constexpr T add_component(T lhs, T rhs)
		{
			if constexpr (std::same_as<T, u8>)
				return add_clamp(lhs, rhs);
			else
				return std::clamp(lhs + rhs, 0.0f, 1.0f);
		}

		static constexpr T sub_component(T lhs, T rhs)
		{
			if constexpr (std::same_as<T, u8>)
				return sub_clamp(lhs, rhs);
			else
				return std::clamp(lhs - rhs, 0.0f, 1.0f);
		}

		static constexpr T mul_component(T lhs, T rhs)
		{
			if constexpr (std::same_as<T, u8>)
				return mul_clamp(lhs, rhs);
			else
				return std::clamp(lhs * rhs, 0.0f, 1.0f);
		}

		static constexpr T div_component(T lhs, T rhs)
		{
			if constexpr (std::same_as<T, u8>)
				return div_clamp(lhs, rhs);
			else if (rhs == 0.0f)
				return 1.0f;
			else
				return std::clamp(lhs / rhs, 0.0f, 1.0f);
		}

		static constexpr u8 to_byte(T value)
		{
			if constexpr (std::same_as<T, u8>)
				return value;
			else
				return clamp_rgb(value);
		}

	public:
		constexpr generic_color()
		{
			if constexpr (Components == 4)
				(*this)[3] = opaque_alpha();
		}

		template<typename... Values>
		requires((sizeof...(Values) == Components or (Components == 4 and sizeof...(Values) == 3))
				 and (std::is_arithmetic_v<Values> and ...))
		constexpr generic_color(Values... values)
		{
			const std::array<T, sizeof...(Values)> components{convert_component(values)...};
			for (std::size_t i = 0; i < components.size(); ++i)
				(*this)[i] = components[i];
			if constexpr (Components == 4 and sizeof...(Values) == 3)
				(*this)[3] = opaque_alpha();
		}

		template<typename U>
		requires std::is_arithmetic_v<U>
		constexpr generic_color(const std::array<U, Components>& values)
		{
			for (std::size_t i = 0; i < Components; ++i)
				(*this)[i] = convert_component(values[i]);
		}

		template<typename U, typename Alpha>
		requires(Components == 4 and std::is_arithmetic_v<U> and std::is_arithmetic_v<Alpha>)
		constexpr generic_color(const std::array<U, 3>& values, Alpha alpha)
		{
			(*this)[0] = convert_component(values[0]);
			(*this)[1] = convert_component(values[1]);
			(*this)[2] = convert_component(values[2]);
			(*this)[3] = convert_component(alpha);
		}

		template<typename U>
		requires(Components == 3)
		constexpr generic_color(const generic_color<U, 4>& color)
		{
			(*this)[0] = convert_component(color[0]);
			(*this)[1] = convert_component(color[1]);
			(*this)[2] = convert_component(color[2]);
		}

		template<typename U, typename Alpha>
		requires(Components == 4 and std::is_arithmetic_v<Alpha>)
		constexpr generic_color(const generic_color<U, 3>& color, Alpha alpha)
		{
			(*this)[0] = convert_component(color[0]);
			(*this)[1] = convert_component(color[1]);
			(*this)[2] = convert_component(color[2]);
			(*this)[3] = convert_component(alpha);
		}

		constexpr std::array<T, Components> to_array() const
		{
			std::array<T, Components> result{};
			for (std::size_t i = 0; i < Components; ++i)
				result[i] = (*this)[i];
			return result;
		}

		constexpr generic_color operator+(const generic_color& rhs) const
		{
			generic_color result;
			for (std::size_t i = 0; i < Components; ++i)
				result[i] = add_component((*this)[i], rhs[i]);
			return result;
		}

		constexpr generic_color operator-(const generic_color& rhs) const
		{
			generic_color result;
			for (std::size_t i = 0; i < Components; ++i)
				result[i] = sub_component((*this)[i], rhs[i]);
			return result;
		}

		constexpr generic_color operator*(const generic_color& rhs) const
		{
			generic_color result;
			for (std::size_t i = 0; i < Components; ++i)
				result[i] = mul_component((*this)[i], rhs[i]);
			return result;
		}

		constexpr generic_color operator/(const generic_color& rhs) const
		{
			generic_color result;
			for (std::size_t i = 0; i < Components; ++i)
				result[i] = div_component((*this)[i], rhs[i]);
			return result;
		}

		constexpr bool operator==(const generic_color& rhs) const
		{
			for (std::size_t i = 0; i < Components; ++i)
			{
				if ((*this)[i] != rhs[i])
					return false;
			}
			return true;
		}

		constexpr u32 to_u32() const
		requires(Components >= 3)
		{
			u32 value = static_cast<u32>(to_byte(this->r)) | (static_cast<u32>(to_byte(this->g)) << 8)
						| (static_cast<u32>(to_byte(this->b)) << 16);
			if constexpr (Components == 4)
				value |= static_cast<u32>(to_byte(this->a)) << 24;
			return value;
		}

		constexpr explicit operator u32() const
		requires(Components >= 3)
		{
			return to_u32();
		}
	};

	export struct hsv
	{
		f32 h = 0.0f, s = 0.0f, v = 0.0f, a = 1.0f;

		hsv(f32 hh, f32 ss, f32 vv)
			: h(std::clamp(hh, 0.0f, 360.0f))
			, s(std::clamp(ss / 100.0f, 0.0f, 1.0f))
			, v(std::clamp(vv / 100.0f, 0.0f, 1.0f))
		{
		}

		template<typename T, u32 Components>
		requires((Components == 3 or Components == 4) and (std::same_as<T, u8> or std::same_as<T, f32>))
		explicit hsv(const generic_color<T, Components>& color)
		{
			const auto normalize = [](T channel)
			{
				if constexpr (std::same_as<T, u8>)
					return static_cast<f32>(channel) / 255.0f;
				else
					return channel;
			};

			const f32 red   = normalize(color.r);
			const f32 green = normalize(color.g);
			const f32 blue  = normalize(color.b);
			const f32 cmax  = std::max({red, green, blue});
			const f32 cmin  = std::min({red, green, blue});
			const f32 delta = cmax - cmin;

			if (delta != 0.0f)
			{
				if (cmax == red)
					h = 60.0f * std::fmod((green - blue) / delta, 6.0f);
				else if (cmax == green)
					h = 60.0f * (((blue - red) / delta) + 2.0f);
				else
					h = 60.0f * (((red - green) / delta) + 4.0f);
				if (h < 0.0f)
					h += 360.0f;
			}

			s = cmax == 0.0f ? 0.0f : delta / cmax;
			v = cmax;
			if constexpr (Components == 4)
				a = normalize(color.a);
		}

		constexpr f32& operator[](std::size_t index) noexcept
		{
			switch (index)
			{
				case 0: return h;
				case 1: return s;
				case 2: return v;
				case 3: return a;
				default: std::unreachable();
			}
		}

		constexpr const f32& operator[](std::size_t index) const noexcept
		{
			switch (index)
			{
				case 0: return h;
				case 1: return s;
				case 2: return v;
				case 3: return a;
				default: std::unreachable();
			}
		}

		constexpr std::array<f32, 3> data() const { return {h, s, v}; }

		constexpr std::array<f32, 4> to_array() const { return {h, s, v, a}; }

		// RGB(0,0,0)		-> HSV(0,0,0)
		// RGB(255,0,0)		-> HSV(0,100,100)
		// RGB(0,255,0)		-> HSV(120,100,100)
		// RGB(0,0,255)		-> HSV(240,100,100)
		// RGB(255,0,255)	-> HSV(300,100,100)
		// RGB(255,255,255) -> HSV(0,0,100)

		// RGB(0,128,196) -> HSV(200.8, 100, 76.9)
	};

	export template<typename T>
	using gray = generic_color<T, 1>;

	export template<typename T>
	using rgb = generic_color<T, 3>;

	export template<typename T>
	using rgba = generic_color<T, 4>;

	export using rgb_f32  = rgb<f32>;
	export using rgba_f32 = rgba<f32>;

	export rgb<u8> to_rgb(f32 h, f32 s, f32 v)
	{
		f32 hh = h;

		if (s == 0.0)
		{
			auto vt = clamp_rgb(v);
			return {vt, vt, vt};
		}
		else
		{
			hh = fmod(hh, 360.0f);
			hh /= 60.0f;
			int i = static_cast<int>(std::floor(hh));
			f32 f = hh - i;
			f32 p = v * (1.0f - s);
			f32 q = v * (1.0f - s * f);
			f32 t = v * (1.0f - s * (1.0f - f));

			switch (i)
			{
				case 0: return {clamp_rgb(v), clamp_rgb(t), clamp_rgb(p)};
				case 1: return {clamp_rgb(q), clamp_rgb(v), clamp_rgb(p)};
				case 2: return {clamp_rgb(p), clamp_rgb(v), clamp_rgb(t)};
				case 3: return {clamp_rgb(p), clamp_rgb(q), clamp_rgb(v)};
				case 4: return {clamp_rgb(t), clamp_rgb(p), clamp_rgb(v)};
				default: return {clamp_rgb(v), clamp_rgb(p), clamp_rgb(q)};
			}
		}
	}

	export rgb<u8> to_rgb(const std::array<f32, 3>& value) { return to_rgb(value[0], value[1], value[2]); }

	export rgba<u8> to_rgba(const std::array<f32, 3>& value)
	{
		const auto color = to_rgb(value);
		return {color.r, color.g, color.b, 255_u8};
	}

	export namespace colors
	{
		export constexpr rgb<u8> black{0, 0, 0};
		export constexpr rgb<u8> white{255, 255, 255};
		export constexpr rgb<u8> red{255, 0, 0};
		export constexpr rgb<u8> green{0, 255, 0};
		export constexpr rgb<u8> blue{0, 0, 255};
		export constexpr rgb<u8> yellow{255, 255, 0};
		export constexpr rgb<u8> cyan{0, 255, 255};
		export constexpr rgb<u8> magenta{255, 0, 255};
	} // namespace colors

	u8 mix(u8 a, u8 b, f32 t)
	{
		//
		return static_cast<u8>(a + t * (b - a));
		// return static_cast<u8>((std::round(1.0f - t) * a + t * b));
	}

	export rgba<u8> lerp(const rgba<u8>& a, const rgba<u8>& b, f32 t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return {mix(a.r, b.r, t), mix(a.g, b.g, t), mix(a.b, b.b, t), mix(a.a, b.a, t)};
	}

	bool equal(const hsv& lhs, const rgb<u8>& rhs)
	{
		const auto c = to_rgb(lhs.data());
		return rhs.r == c.r and rhs.g == c.g and rhs.b == c.b;
	}

	bool equal(const hsv& lhs, const rgba<u8>& rhs)
	{
		const auto c = to_rgba(lhs.data());
		return rhs.r == c.r and rhs.g == c.g and rhs.b == c.b and rhs.a == clamp_rgb(lhs.a);
	}

	//

	export bool operator==(const hsv& lhs, const rgb<u8>& rhs) { return equal(lhs, rhs); }

	export bool operator==(const hsv& lhs, const rgba<u8>& rhs) { return equal(lhs, rhs); }
}; // namespace deckard

export namespace std
{
	using namespace deckard;

	template<typename T, deckard::u32 Components>
	struct hash<deckard::generic_color<T, Components>>
	{
		size_t operator()(const deckard::generic_color<T, Components>& value) const
		{
			return deckard::utils::hash_values(value.to_array());
		}
	};

	template<>
	struct hash<hsv>
	{
		size_t operator()(const hsv& value) const { return deckard::utils::hash_values(value.h, value.s, value.v, value.a); }
	};

	template<typename T>
	struct formatter<deckard::generic_color<T, 1>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const deckard::generic_color<T, 1>& v, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "gray({})", v.r);
		}
	};

	template<typename T>
	struct formatter<deckard::generic_color<T, 2>>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const deckard::generic_color<T, 2>& v, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "color({}, {})", v[0], v[1]);
		}
	};

	template<typename T>
	struct formatter<deckard::generic_color<T, 3>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const deckard::generic_color<T, 3>& v, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "rgb({}, {}, {})", v.r, v.g, v.b);
		}
	};

	template<typename T>
	struct formatter<deckard::generic_color<T, 4>>
	{
		bool show_as_float = false;
		bool show_as_hex   = false;
		bool show_upper    = false;

		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto pos = ctx.begin();

			while (pos != ctx.end() and *pos != '}')
			{
				if (*pos == 'f' or *pos == 'F')
					show_as_float = true;
				else if (*pos == 'x')
					show_as_hex = true;
				else if (*pos == 'X')
				{
					show_as_hex = true;
					show_upper  = true;
				}

				pos++;
			}

			if (pos != ctx.end() and *pos != '}')
				throw std::format_error("Invalid format");


			return pos;
		}

		auto format(const deckard::generic_color<T, 4>& v, std::format_context& ctx) const
		{
			if (show_as_float)
			{
				std::format_to(ctx.out(), "rgba(");

				const auto to_float = [](T value) constexpr
				{
					if constexpr (std::same_as<T, u8>)
						return static_cast<f32>(value) / 255.0f;
					else
						return value;
				};

				f32 r = to_float(v.r);
				f32 g = to_float(v.g);
				f32 b = to_float(v.b);
				f32 a = to_float(v.a);


				// r
				if (math::is_close_enough_zero(r) or math::is_close_enough_one(r))
					std::format_to(ctx.out(), "{:.1f}, ", r);
				else
					std::format_to(ctx.out(), "{:.3f}, ", r);

				// g
				if (math::is_close_enough_zero(g) or math::is_close_enough_one(g))
					std::format_to(ctx.out(), "{:.1f}, ", g);
				else
					std::format_to(ctx.out(), "{:.3f}, ", g);

				// b
				if (math::is_close_enough_zero(b) or math::is_close_enough_one(b))
					std::format_to(ctx.out(), "{:.1f}, ", b);
				else
					std::format_to(ctx.out(), "{:.3f}, ", b);

				// a
				if (math::is_close_enough_zero(a) or math::is_close_enough_one(a))
					std::format_to(ctx.out(), "{:.1f}", a);
				else
					std::format_to(ctx.out(), "{:.3f}", a);

				return std::format_to(ctx.out(), ")");
			}

			if (show_as_hex)
			{
				const auto to_byte = [](T value) constexpr
				{
					if constexpr (std::same_as<T, u8>)
						return value;
					else
						return deckard::clamp_rgb(value);
				};

				if (show_upper)
					return std::format_to(
					  ctx.out(), "rgba(#{:02X}{:02X}{:02X}{:02X})", to_byte(v.r), to_byte(v.g), to_byte(v.b), to_byte(v.a));

				return std::format_to(
				  ctx.out(), "rgba(#{:02x}{:02x}{:02x}{:02x})", to_byte(v.r), to_byte(v.g), to_byte(v.b), to_byte(v.a));
			}


			return std::format_to(ctx.out(), "rgba({}, {}, {}, {})", v.r, v.g, v.b, v.a);
		}
	};

	template<>
	struct formatter<hsv>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const hsv& v, std::format_context& ctx) const
		{
			if (v.a == 1.0f)
				return std::format_to(ctx.out(), "hsv({}, {}, {})", v.h, v.s, v.v);
			return std::format_to(ctx.out(), "hsva({}, {}, {}, {})", v.h, v.s, v.v, v.a);
		}
	};
} // namespace std
