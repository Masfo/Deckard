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

		 void dfsVisit(std::size_t u, std::unordered_set<std::size_t>& visited, std::vector<T>& traversal) const
		{
			visited.insert(u);
			traversal.push_back(reverse_index[u]);

			for (auto v : adjacent_list[u])
			{
				if (!visited.contains(v))
				{
					dfsVisit(v, visited, traversal);
				}
			}
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


		std::vector<T> bfs(const T& start)
		{
			std::vector<T>        traversal;
			std::queue<T>         q;
			std::unordered_set<T> visited;

			q.push(start);
			visited.insert(start);

			while (!q.empty())
			{
				T current = q.front();
				q.pop();

				traversal.push_back(current);

				for (const auto& neighbor : neighbors(current))
				{
					if (!visited.contains(neighbor))
					{
						visited.insert(neighbor);
						q.push(neighbor);
					}
				}
			}

			return traversal;
		}


		   std::vector<T> dfs(const T& start) const
		{
			auto it = index_map.find(start);
			if (it == index_map.end())
			{
				dbg::println("Start node not found");
				return {};
			}

			std::vector<T>               traversal;
			std::unordered_set<std::size_t> visited;

			dfsVisit(it->second, visited, traversal);
			return traversal;
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
