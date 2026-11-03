export module deckard.mesh;

import std;
import deckard.types;
import deckard.vec;
import deckard.vulkan.common;
import deckard.json;

namespace deckard::vulkan
{

	// Vertex format used by the 3D mesh pipeline. Positions and normals are in model space.
	export struct mesh_vertex
	{
		vec::vec3 position;
		vec::vec3 normal;
		vec::vec2 uv;
	};

	export struct mesh
	{
		std::vector<mesh_vertex> vertices;
		std::vector<u32>         indices;

		[[nodiscard]] bool empty() const noexcept { return vertices.empty() or indices.empty(); }
	};


	// Axis-aligned cube centered on the origin. `size` is the full edge length.
	export mesh create_cube(f32 size = 1.0f) noexcept
	{
		mesh      m;
		const f32 h = size * 0.5f;

		const auto add_face = [&m](const vec::vec3& p0, const vec::vec3& p1, const vec::vec3& p2, const vec::vec3& p3,
								   const vec::vec3& normal)
		{
			const u32 base = static_cast<u32>(m.vertices.size());
			m.vertices.push_back({p0, normal, {0.0f, 0.0f}});
			m.vertices.push_back({p1, normal, {1.0f, 0.0f}});
			m.vertices.push_back({p2, normal, {1.0f, 1.0f}});
			m.vertices.push_back({p3, normal, {0.0f, 1.0f}});

			const std::array<u32, 6> face_indices{base, base + 1, base + 2, base, base + 2, base + 3};
			m.indices.insert(m.indices.end(), face_indices.begin(), face_indices.end());
		};

		add_face({-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}, {0.0f, 0.0f, 1.0f});   // front
		add_face({h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}, {0.0f, 0.0f, -1.0f}); // back
		add_face({h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h}, {1.0f, 0.0f, 0.0f});    // right
		add_face({-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}, {-1.0f, 0.0f, 0.0f}); // left
		add_face({-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}, {0.0f, 1.0f, 0.0f});    // top
		add_face({-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}, {0.0f, -1.0f, 0.0f}); // bottom

		return m;
	}


	// Quad lying in the XZ plane, centered on the origin and facing +Y.
	export mesh create_plane(f32 width = 1.0f, f32 depth = 1.0f) noexcept
	{
		mesh      m;
		const f32 w = width * 0.5f;
		const f32 d = depth * 0.5f;
		const vec::vec3 up{0.0f, 1.0f, 0.0f};

		m.vertices = {
		  {{-w, 0.0f, d}, up, {0.0f, 0.0f}},
		  {{w, 0.0f, d}, up, {1.0f, 0.0f}},
		  {{w, 0.0f, -d}, up, {1.0f, 1.0f}},
		  {{-w, 0.0f, -d}, up, {0.0f, 1.0f}},
		};

		m.indices = {0, 1, 2, 0, 2, 3};
		return m;
	}


} // namespace deckard::vulkan
