export module deckard.spatial:hashgrid;

import std;
import deckard.types;
import deckard.vec;

namespace deckard::spatial
{
	using namespace deckard::vec;

	// Target world size
	//
	// Cell size

	ivec3 target_size{100, 50, 100};
	ivec3 cell_size{16, 16, 16};
	vec3  inv_cell_size{1.0f / cell_size.x, 1.0f / cell_size.y, 1.0f / cell_size.z};


	ivec3 cell_count{((target_size.x + cell_size.x - 1) / cell_size.x),
					 ((target_size.y + cell_size.y - 1) / cell_size.y),
					 ((target_size.z + cell_size.z - 1) / cell_size.z)};

	ivec3 world_size{cell_count.x * cell_size.x, cell_count.y* cell_size.y, cell_count.z* cell_size.z};
	vec3  world_min{-world_size.x * 0.5f, -world_size.y * 0.5f, -world_size.z * 0.5f};
	vec3  world_max{world_size.x * 0.5f, world_size.y * 0.5f, world_size.z * 0.5f};

	class hashgrid3d
	{
	private:
	public:
	};

}; // namespace deckard::spatial
