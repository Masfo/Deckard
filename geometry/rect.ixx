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

			std::array<vec2, 4> local = { {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}}};

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

}; // namespace deckard::geometry
