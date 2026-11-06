export module deckard.geometry:aabb;

import deckard.vec;

namespace deckard::geometry
{
	using namespace deckard::vec;

	export struct aabb
	{
		vec3 min{0};
		vec3 max{0};
		constexpr aabb() = default;

		constexpr aabb(const vec3& min, const vec3& max)
			: min(min)
			, max(max)
		{
		}

		constexpr bool contains(const vec3& point) const
		{
			return point.x >= min.x and point.x <= max.x and point.y >= min.y and point.y <= max.y and point.z >= min.z
				   and point.z <= max.z;
		}

		constexpr bool intersects(const aabb& other) const
		{
			return (min.x <= other.max.x and max.x >= other.min.x) and (min.y <= other.max.y and max.y >= other.min.y)
				   and (min.z <= other.max.z and max.z >= other.min.z);
		}

		constexpr vec3 center() const { return (min + max) * 0.5f; }

		constexpr auto area() const -> f32
		{
			vec3 size = max - min;
			return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
		}

		constexpr auto volume() const -> f32
		{
			vec3 size = max - min;
			return size.x * size.y * size.z;
		}

		constexpr auto merge(const aabb& other) const -> aabb
		{
			return aabb{vec3{std::min(min.x, other.min.x), std::min(min.y, other.min.y), std::min(min.z, other.min.z)},
						vec3{std::max(max.x, other.max.x), std::max(max.y, other.max.y), std::max(max.z, other.max.z)}};
		}

		constexpr auto operator+(const aabb& other) const -> aabb { return merge(other); }

		constexpr auto operator+=(const aabb& other) -> aabb&
		{
			*this = merge(other);
			return *this;
		}
	};

	export struct ibox
	{
		ivec3 min{0, 0, 0};
		ivec3 max{1, 1, 1};

		constexpr ibox() noexcept = default;

		constexpr explicit ibox(ivec3 pos) noexcept
			: min{pos}
			, max{pos + ivec3{1, 1, 1}}
		{
		}

		constexpr ibox(ivec3 lo, ivec3 hi) noexcept
			: min{lo}
			, max{hi}
		{
		}

		[[nodiscard]] static constexpr ibox from_size(ivec3 pos, ivec3 sz) noexcept { return {pos, pos + sz}; }

		[[nodiscard]] static constexpr ibox none() noexcept { return {{0, 0, 0}, {0, 0, 0}}; }

		[[nodiscard]] friend constexpr bool operator==(const ibox&, const ibox&) noexcept = default;

		[[nodiscard]] constexpr ivec3 position() const noexcept { return min; }

		[[nodiscard]] constexpr ivec3 size() const noexcept { return max - min; }

		[[nodiscard]] constexpr bool empty() const noexcept { return min.x >= max.x or min.y >= max.y or min.z >= max.z; }

		[[nodiscard]] constexpr bool contains(ivec3 p) const noexcept
		{
			return p.x >= min.x and p.x < max.x and p.y >= min.y and p.y < max.y and p.z >= min.z and p.z < max.z;
		}

		[[nodiscard]] constexpr bool contains(const ibox& other) const noexcept
		{
			return other.empty()
				   or (other.min.x >= min.x and other.max.x <= max.x and other.min.y >= min.y and other.max.y <= max.y
					   and other.min.z >= min.z and other.max.z <= max.z);
		}

		[[nodiscard]] constexpr std::optional<ibox> intersection(const ibox& other) const noexcept
		{
			const ivec3 lo{std::max(min.x, other.min.x), std::max(min.y, other.min.y), std::max(min.z, other.min.z)};
			const ivec3 hi{std::min(max.x, other.max.x), std::min(max.y, other.max.y), std::min(max.z, other.max.z)};

			if (lo.x >= hi.x or lo.y >= hi.y or lo.z >= hi.z)
				return std::nullopt;

			return ibox{lo, hi};
		}

		[[nodiscard]] constexpr bool intersects(const ibox& other) const noexcept { return intersection(other).has_value(); }

		[[nodiscard]] constexpr ibox union_with(const ibox& other) const noexcept
		{
			if (empty())
				return other;
			if (other.empty())
				return *this;

			return {{std::min(min.x, other.min.x), std::min(min.y, other.min.y), std::min(min.z, other.min.z)},
					{std::max(max.x, other.max.x), std::max(max.y, other.max.y), std::max(max.z, other.max.z)}};
		}

		[[nodiscard]] constexpr i64 volume() const noexcept
		{
			if (empty())
				return 0;
			const auto [x, y, z] = extent64();
			return x * y * z;
		}

		[[nodiscard]] constexpr i64 surface_area() const noexcept
		{
			if (empty())
				return 0;
			const auto [x, y, z] = extent64();
			return 2 * (x * y + y * z + z * x);
		}

	private:
		[[nodiscard]] constexpr std::array<i64, 3> extent64() const noexcept
		{
			return {static_cast<i64>(max.x) - min.x, static_cast<i64>(max.y) - min.y, static_cast<i64>(max.z) - min.z};
		}
	};

	export {
		[[nodiscard]] constexpr bool contains(const ibox& a, ivec3 p) noexcept { return a.contains(p); }

		[[nodiscard]] constexpr bool contains(const ibox& a, const ibox& b) noexcept { return a.contains(b); }

		[[nodiscard]] constexpr bool intersects(const ibox& a, const ibox& b) noexcept { return a.intersects(b); }

		[[nodiscard]] constexpr std::optional<ibox> intersection(const ibox& a, const ibox& b) noexcept
		{
			return a.intersection(b);
		}
	}
} // namespace deckard::geometry
