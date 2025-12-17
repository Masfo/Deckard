export module deckard.graph:undirected;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.arrays;

namespace deckard::graph
{


	export template<typename T>
	class undirected_graph
	{
	private:
		std::vector<std::vector<u64>> adjacent_list{};
		std::unordered_map<T, u64>    index_map{};
		std::vector<T>                reverse_index{};

		bool has_edge(u64 u, u64 v) const
		{
			const auto& neighbors = adjacent_list[u];
			return std::find(neighbors.begin(), neighbors.end(), v) != neighbors.end();
		}

	public:
		undirected_graph() = default;

		void add(const T& node)
		{
			if (not index_map.contains(node))
			{
				index_map[node] = adjacent_list.size();
				reverse_index.push_back(node);
				adjacent_list.emplace_back();
			}
		}

		void connect(const T& a, const T& b)
		{
			if (a == b)
				return;

			add(a);
			add(b);

			u64 ia = index_map[a];
			u64 ib = index_map[b];

			if (not has_edge(ia, ib))
			{
				adjacent_list[ia].push_back(ib);
				adjacent_list[ib].push_back(ia);
			}
		}

		std::vector<T> neighbors(const T& node) const
		{
			auto it = index_map.find(node);
			if (it == index_map.end())
				return {};

			const auto&    adjacent_indices = adjacent_list[it->second];
			std::vector<T> result;
			result.reserve(adjacent_indices.size());

			for (const auto &idx : adjacent_indices)
				result.push_back(reverse_index[idx]);

			return result;
		}

		void dump()
		{
			const u64 n = reverse_index.size();

			dbg::print("    ");
			for (u64 j = 0; j < n; ++j)
			{
				dbg::print("{}", reverse_index[j]);
				if (j + 1 < n)
					dbg::print(" ");
			}
			dbg::println();

			for (u64 i = 0; i < n; ++i)
			{
				std::vector<bool> present(n, false);
				const auto&       neighbors = adjacent_list[i];
				for (u64 k = 0; k < neighbors.size(); ++k)
				{
					u64 nbr = neighbors[k];
					if (nbr < n)
						present[nbr] = true;
				}

				dbg::print("{} :", reverse_index[i]);

				for (u64 j = 0; j < n; ++j)
					dbg::print(" {}", present[j] ? "1" : "0");

				dbg::println();
			}
		}
	};

} // namespace deckard::graph
