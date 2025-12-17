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
		std::vector<std::unordered_set<u64>> adjacent_list{};
		std::unordered_map<T, u64>           index_map{};
		std::vector<T>                       reverse_index{};

		bool has_edge(u64 u, u64 v) const { return adjacent_list[u].find(v) != adjacent_list[u].end(); }

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

		bool has_edge(const T& u, const T& v) const
		{

			auto iu = index_map.find(u);
			auto iv = index_map.find(v);
			if (iu == index_map.end() or iv == index_map.end())
				return false;

			u64 a = iu->second;
			u64 b = iv->second;

			return adjacent_list[a].contains(b);
		}

		void connect(const T& a, const T& b)
		{
			if (a == b)
				return;

			add(a);
			add(b);

			u64 ia = index_map[a];
			u64 ib = index_map[b];

			adjacent_list[ia].insert(ib);
			adjacent_list[ib].insert(ia);
		}

		std::vector<T> neighbors(const T& node) const
		{
			auto it = index_map.find(node);
			if (it == index_map.end())
				return {};

			const auto&    adjacent_indices = adjacent_list[it->second];
			std::vector<T> result;
			result.reserve(adjacent_indices.size());

			for (const auto& idx : adjacent_indices)
				result.push_back(reverse_index[idx]);

			return result;
		}

		// count connected nodes (islands)



		 std::vector<T> bfs(const T& start_node)
		{
			std::vector<T>                  traversal_result;
			std::unordered_set<std::size_t> visited;

			// Get the starting node index
			auto start_it = index_map.find(start_node);

			if (start_it == index_map.end())
			{
				dbg::println("Start node not found");
				return traversal_result;
			}

			std::size_t start_index = start_it->second;
			visited.insert(start_index);

			std::queue<std::size_t> queue;
			queue.push(start_index);

			while (!queue.empty())
			{
				std::size_t current_index = queue.front();
				queue.pop();

				// Retrieve the current node and add it to the result
				traversal_result.push_back(reverse_index[current_index]);

				// Get neighbors and add them to the queue
				for (const auto& neighbor_index : adjacent_list[current_index])
				{
					if (visited.find(neighbor_index) == visited.end())
					{
						visited.insert(neighbor_index);
						queue.push(neighbor_index);
					}
				}
			}

			return traversal_result;
		}

		void dump() const
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
				for (const auto& nbr : neighbors)
				{
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
