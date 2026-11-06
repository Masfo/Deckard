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

	export struct irect
	{
		ivec2 min{0, 0};
		ivec2 max{1, 1};

		constexpr irect() noexcept = default;

		constexpr explicit irect(ivec2 pos) noexcept
			: min{pos}
			, max{pos + ivec2{1, 1}}
		{
		}

		constexpr irect(ivec2 mmin, ivec2 mmax) noexcept
			: min{mmin}
			, max{mmax}
		{
		}

		[[nodiscard]] static constexpr irect from_size(ivec2 pos, ivec2 sz) noexcept { return {pos, pos + sz}; }

		[[nodiscard]] friend constexpr bool operator==(const irect&, const irect&) noexcept = default;

		[[nodiscard]] constexpr ivec2 size() const noexcept { return max - min; }

		[[nodiscard]] constexpr ivec2 position() const noexcept { return min; }

		//

		[[nodiscard]] constexpr bool contains(ivec2 p) const noexcept
		{
			return p.x >= min.x and p.x < max.x and p.y >= min.y and p.y < max.y;
		}

		[[nodiscard]] constexpr bool contains(const irect& b) const noexcept
		{
			return b.min.x >= min.x and b.max.x <= max.x and b.min.y >= min.y and b.max.y <= max.y;
		}

		[[nodiscard]] constexpr std::optional<irect> intersection(const irect& b) const noexcept
		{
			const ivec2 lo{std::max(min.x, b.min.x), std::max(min.y, b.min.y)};
			const ivec2 hi{std::min(max.x, b.max.x), std::min(max.y, b.max.y)};

			if (lo.x >= hi.x or lo.y >= hi.y)
				return std::nullopt;

			return irect{lo, hi};
		}

		[[nodiscard]] constexpr bool intersects(const irect& b) const noexcept { return intersection(b).has_value(); }

		[[nodiscard]] constexpr bool empty() const noexcept { return min.x >= max.x or min.y >= max.y; }

		[[nodiscard]] static constexpr irect none() noexcept { return {{0, 0}, {0, 0}}; }

		[[nodiscard]] constexpr irect union_with(const irect& b) const noexcept
		{
			if (empty())
				return b;
			if (b.empty())
				return *this;
			return {{std::min(min.x, b.min.x), std::min(min.y, b.min.y)},
					{std::max(max.x, b.max.x), std::max(max.y, b.max.y)}};
		}
	};

	export [[nodiscard]] constexpr bool intersects(const irect& a, const irect& b) noexcept { return a.intersects(b); }

	export [[nodiscard]] constexpr auto intersection(const irect& a, const irect& b) noexcept { return a.intersection(b); }

	export [[nodiscard]] constexpr bool contains(const irect& a, ivec2 p) noexcept { return a.contains(p); }

	export [[nodiscard]] constexpr bool contains(const irect& a, const irect& b) noexcept { return a.contains(b); }


}; // namespace deckard::geometry
