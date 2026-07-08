export module deckard.geometry:rect;

// import std;
import deckard.types;
import deckard.vec;
import deckard.math.utils;

namespace deckard::geometry
{
	using namespace deckard::math;

	export struct rect
	{
		vec2 position{0.0f, 0.0f};
		vec2 size{1.0f, 1.0f};
		f32  rotation{0.0f}; // radians

		rect() = default;

		rect(vec2 pos)
			: rect(pos, vec2{1.0f, 1.0f})
		{
		}

		rect(vec2 p, vec2 s)
			: position(p)
			, size(s)
		{
		}

		[[nodiscard]] auto min() const { return position - (size * 0.5f); }

		[[nodiscard]] auto max() const { return position + (size * 0.5f); }

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

			// transform
			std::array<vec2, 4> world{};
			for (int i = 0; i < 4; ++i)
			{
				// rotate
				f32 rx = local[i].x * c - local[i].y * s;
				f32 ry = local[i].x * s + local[i].y * c;

				rx = clamp_zero(rx);
				ry = clamp_zero(ry);


				// translate
				world[i] = position + vec2{rx, ry};
			}
			return world;
		}
	};

}; // namespace deckard::geometry
