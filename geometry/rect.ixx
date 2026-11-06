export module deckard.geometry:rect;

// import std;
import deckard.types;
import deckard.vec;
import deckard.math.utils;

namespace deckard::geometry
{
	using namespace deckard::vec;
	using namespace deckard::math;

	export struct rect
	{
		vec2 position{0.0f, 0.0f};
		vec2 size{1.0f, 1.0f};
		f32  rotation{0.0f}; // radians

		constexpr rect() = default;

		constexpr rect(vec2 pos)
			: rect(pos, vec2{1.0f, 1.0f})
		{
		}

		constexpr rect(vec2 p, vec2 s)
			: position(p)
			, size(s)
		{
		}

		[[nodiscard]] constexpr auto min() const { return position - (size * 0.5f); }

		[[nodiscard]] constexpr auto max() const { return position + (size * 0.5f); }

		[[nodiscard]] bool intersect(const vec2& b) const
		{
			const vec2 relative = b - position;
			const f32  c        = std::cos(rotation);
			const f32  s        = std::sin(rotation);
			const f32  x        = relative.x * c + relative.y * s;
			const f32  y        = -relative.x * s + relative.y * c;
			const vec2 half     = size * 0.5f;
			return (x >= -half.x and x <= half.x) and (y >= -half.y and y <= half.y);
		}

		[[nodiscard]] vec2 transform(const vec2& local) const
		{
			f32 c = std::cos(rotation);
			f32 s = std::sin(rotation);

			f32 x = local.x * c - local.y * s;
			f32 y = local.x * s + local.y * c;
			return position + vec2{x, y};
		}

		void rotate(const vec2& pivot, f32 radians)
		{
			f32  c        = std::cos(radians);
			f32  s        = std::sin(radians);
			vec2 relative = position - pivot;
			position      = vec2{relative.x * c - relative.y * s, relative.x * s + relative.y * c} + pivot;

			rotation = normalize_radians(rotation + radians);
		}

		void rotate(f32 radians) { rotate(position, radians); }

		std::array<vec2, 4> vertices() const
		{
			f32 hw = size.x * 0.5f;
			f32 hh = size.y * 0.5f;

			std::array<vec2, 4> local = {{{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}}};

			f32 c = std::cos(rotation);
			f32 s = std::sin(rotation);

			std::array<vec2, 4> world{};
			for (int i = 0; i < 4; ++i)
			{
				f32 rx = local[i].x * c - local[i].y * s;
				f32 ry = local[i].x * s + local[i].y * c;

				world[i] = position + vec2{rx, ry};
			}
			return world;
		}
	};

	export [[nodiscard]] bool intersect(const rect& a, const vec2& b) { return a.intersect(b); }

	export [[nodiscard]] bool intersect(const vec2& b, const rect& a) { return intersect(a, b); }

	// ########################
	export template<typename T>
	struct generic_irect
	{
		static_assert(
		  std::is_same_v<T, ivec2> or std::is_same_v<T, uvec2> or std::is_same_v<T, i64vec2> or std::is_same_v<T, u64vec2>);

		T min{0, 0};
		T max{1, 1};

		constexpr generic_irect() noexcept = default;

		constexpr explicit generic_irect(T pos) noexcept
			: min{pos}
			, max{pos + T{1, 1}}
		{
		}

		constexpr generic_irect(T mmin, T mmax) noexcept
			: min{mmin}
			, max{mmax}
		{
		}

		[[nodiscard]] static constexpr generic_irect<T> from_size(T pos, T sz) noexcept { return {pos, pos + sz}; }

		[[nodiscard]] friend constexpr bool operator==(const generic_irect<T>&, const generic_irect<T>&) noexcept = default;

		[[nodiscard]] constexpr T size() const noexcept { return max - min; }

		[[nodiscard]] constexpr i64 area() const noexcept
		{
			if (empty())
				return 0;
			const T s = size();
			return static_cast<i64>(s.x) * s.y;
		}

		[[nodiscard]] constexpr T position() const noexcept { return min; }

		//

		[[nodiscard]] constexpr bool contains(T p) const noexcept
		{
			return p.x >= min.x and p.x < max.x and p.y >= min.y and p.y < max.y;
		}

		[[nodiscard]] constexpr bool contains(const generic_irect<T>& b) const noexcept
		{
			return b.min.x >= min.x and b.max.x <= max.x and b.min.y >= min.y and b.max.y <= max.y;
		}

		[[nodiscard]] constexpr std::optional<generic_irect<T>> intersection(const generic_irect<T>& b) const noexcept
		{
			const T lo{std::max(min.x, b.min.x), std::max(min.y, b.min.y)};
			const T hi{std::min(max.x, b.max.x), std::min(max.y, b.max.y)};

			if (lo.x >= hi.x or lo.y >= hi.y)
				return std::nullopt;

			return generic_irect{lo, hi};
		}

		[[nodiscard]] constexpr bool intersects(const generic_irect<T>& b) const noexcept { return intersection(b).has_value(); }

		[[nodiscard]] constexpr bool empty() const noexcept { return min.x >= max.x or min.y >= max.y; }

		[[nodiscard]] static constexpr generic_irect<T> none() noexcept { return generic_irect<T>{{0, 0}, {0, 0}}; }

		[[nodiscard]] constexpr generic_irect<T> union_with(const generic_irect<T>& b) const noexcept
		{
			if (empty())
				return b;
			if (b.empty())
				return *this;
			return {{std::min(min.x, b.min.x), std::min(min.y, b.min.y)},
					{std::max(max.x, b.max.x), std::max(max.y, b.max.y)}};
		}
	};

	export template<typename T>
	[[nodiscard]] constexpr bool intersects(const generic_irect<T>& a, const generic_irect<T>& b) noexcept
	{
		return a.intersects(b);
	}

	export template<typename T>
	[[nodiscard]] constexpr auto intersection(const generic_irect<T>& a, const generic_irect<T>& b) noexcept
	{
		return a.intersection(b);
	}

	export template<typename T>
	[[nodiscard]] constexpr bool contains(const generic_irect<T>& a, T p) noexcept
	{
		return a.contains(p);
	}

	export template<typename T>
	[[nodiscard]] constexpr bool contains(const generic_irect<T>& a, const generic_irect<T>& b) noexcept
	{
		return a.contains(b);
	}

	export using irect = generic_irect<ivec2>;
	export using irect64 = generic_irect<i64vec2>;
	export using urect   = generic_irect<uvec2>;
	export using urect64 = generic_irect<u64vec2>;


}; // namespace deckard::geometry
