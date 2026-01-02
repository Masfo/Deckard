export module deckard.graph;


export import :binarytree;
export import :avltree;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.arrays;

namespace deckard::graph
{


	export template<typename T, typename Weight = f32>
	struct weighted_edge_t;

	export template<typename T, typename Weight, bool Directed = false>
	class graph
	{
	private:
		using WeightedEdge = weighted_edge_t<T, Weight>;

		std::vector<std::unordered_set<u64>>         adjacent_list{};
		std::unordered_map<T, u64>                   index_map{};
		std::vector<T>                               reverse_index{};
		std::vector<std::unordered_map<u64, Weight>> edge_weights{};

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
		graph() = default;

		u64 degree(const T& node) const
		{
			if (not index_map.contains(node))
				return 0;
			return adjacent_list[index_map.at(node)].size();
		}

		void clear() noexcept
		{
			adjacent_list.clear();
			edge_weights.clear();
			index_map.clear();
			reverse_index.clear();
		}

		void remove_node(const T& node)
		{
			auto it = index_map.find(node);
			if (it == index_map.end())
				return;

			u64 node_index = it->second;
			u64 old_last   = static_cast<u64>(reverse_index.size() - 1);

			for (u64 i = 0; i < adjacent_list.size(); ++i)
			{
				auto& nbrs    = adjacent_list[i];
				auto& weights = edge_weights[i];

				if (nbrs.erase(node_index) > 0)
					weights.erase(node_index);
			}

			adjacent_list[node_index].clear();
			edge_weights[node_index].clear();

			if (node_index != old_last)
			{
				T moved                   = std::move(reverse_index[old_last]);
				adjacent_list[node_index] = std::move(adjacent_list[old_last]);
				edge_weights[node_index]  = std::move(edge_weights[old_last]);

				index_map[moved]          = node_index;
				reverse_index[node_index] = std::move(moved);

				for (u64 i = 0; i < adjacent_list.size(); ++i)
				{
					auto& nbrs    = adjacent_list[i];
					auto& weights = edge_weights[i];

					if (nbrs.erase(old_last) > 0)
					{
						nbrs.insert(node_index);

						auto itw = weights.find(old_last);
						if (itw != weights.end())
						{
							Weight w = itw->second;
							weights.erase(itw);
							weights.emplace(node_index, w);
						}
					}
				}
			}

			reverse_index.pop_back();
			adjacent_list.pop_back();
			edge_weights.pop_back();
			index_map.erase(it);
		}

		u64 size() const { return reverse_index.size(); }

		void remove_nodes(const std::vector<T>& nodes)
		{
			for (const auto& node : nodes)
				remove_node(node);
		}

		void add(const T& node)
		{
			if (not index_map.contains(node))
			{
				index_map[node] = adjacent_list.size();
				reverse_index.push_back(node);
				adjacent_list.emplace_back();
				edge_weights.emplace_back();
			}
		}

		bool has_edge(const T& u, const T& v) const
		{

			auto iu = index_map.find(u);
			auto iv = index_map.find(v);
			if (iu == index_map.end() or iv == index_map.end())
				return false;

			u64 first  = iu->second;
			u64 second = iv->second;

			return adjacent_list[first].contains(second);
		}

		std::optional<Weight> weight(const T& a, const T& b) const
		{
			auto ia = index_map.find(a);
			auto ib = index_map.find(b);
			if (ia == index_map.end() or ib == index_map.end())
				return {};

			u64 first  = ia->second;
			u64 second = ib->second;

			auto itw = edge_weights[first].find(second);
			if (itw != edge_weights[first].end())
				return itw->second;

			return {};
		}

		void connect(const T& a, const T& b) { connect(a, b, Weight{1}); }

		void connect(const T& a, const T& b, Weight weight)
		{

			if (a == b)
				return;

			add(a);
			add(b);

			u64 first  = index_map[a];
			u64 second = index_map[b];

			adjacent_list[first].insert(second);
			edge_weights[first][second] = weight;

			if constexpr (not Directed)
			{
				adjacent_list[second].insert(first);
				edge_weights[second][first] = weight;
			}
		}

		void disconnect(const T& a, const T& b)
		{
			if (not index_map.contains(a) || not index_map.contains(b))
				return;
			u64 first  = index_map.at(a);
			u64 second = index_map.at(b);

			adjacent_list[first].erase(second);
			edge_weights[first].erase(second);

			if constexpr (not Directed)
			{
				adjacent_list[second].erase(first);
				edge_weights[second].erase(first);
			}
		}

		bool empty() const { return reverse_index.empty(); }

		u64 node_count() const { return reverse_index.size(); }

		u64 edge_count() const
		{
			u64 total = 0;
			for (const auto& nbrs : adjacent_list)
				total += nbrs.size();
			return total / 2;
		}

		bool contains(const T& node) const noexcept { return index_map.contains(node); }

		bool contains(const T& a, const T& b) const noexcept { return has_edge(a, b); }

		std::vector<T> neighbors(const T& node) const
		{
			if (not index_map.contains(node))
				return {};

			const auto&    adjacent_indices = adjacent_list[index_map.at(node)];
			std::vector<T> result;
			result.reserve(adjacent_indices.size());

			for (const auto& node_index : adjacent_indices)
				result.emplace_back(reverse_index[node_index]);

			return result;
		}

		// All-pairs shortest paths using Floyd-Warshall.
		// Returns an unordered_map mapping each node to a map of reachable nodes and their distances.
		auto all_pairs_shortest_paths() const -> std::unordered_map<T, std::unordered_map<T, Weight>>
		{
			u64          n   = static_cast<u64>(reverse_index.size());
			const Weight INF = std::numeric_limits<Weight>::infinity();

			std::vector<std::vector<Weight>> dist(static_cast<size_t>(n), std::vector<Weight>(static_cast<size_t>(n), INF));

			for (u64 i = 0; i < n; ++i)
			{
				dist[i][i] = 0.0;
				for (u64 j : adjacent_list[i])
				{
					auto   itw = edge_weights[i].find(j);
					Weight w   = 1.0;
					if (itw != edge_weights[i].end())
						w = itw->second;
					dist[i][j] = w;
				}
			}

			for (u64 k = 0; k < n; ++k)
			{
				for (u64 i = 0; i < n; ++i)
				{
					if (dist[i][k] == INF)
						continue;
					for (u64 j = 0; j < n; ++j)
					{
						if (dist[k][j] == INF)
							continue;
						Weight nd = dist[i][k] + dist[k][j];
						if (nd < dist[i][j])
						{
							dist[i][j] = nd;
						}
					}
				}
			}


			std::unordered_map<T, std::unordered_map<T, Weight>> result;
			result.reserve(n);
			for (u64 i = 0; i < n; ++i)
			{
				T ti = reverse_index[i];
				for (u64 j = 0; j < n; ++j)
				{
					if (dist[i][j] == INF)
						continue;
					T tj = reverse_index[j];
					result[ti].emplace(tj, dist[i][j]);
				}
			}

			return result;
		}

		// count connected nodes (islands)


		std::vector<T> bfs(const T& start)
		{
			auto it = index_map.find(start);
			if (it == index_map.end())
				return {};

			u64 s = it->second;
			u64 n = static_cast<u64>(reverse_index.size());

			const Weight        INF = std::numeric_limits<Weight>::infinity();
			std::vector<Weight> dist(n, INF);
			using PQItem = std::pair<Weight, u64>;
			std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> pq;

			dist[s] = Weight{0};
			pq.push({Weight{0}, s});

			std::vector<T> traversal;

			while (!pq.empty())
			{
				auto [d, u] = pq.top();
				pq.pop();
				if (d > dist[u])
					continue;
				traversal.push_back(reverse_index[u]);

				for (u64 v : adjacent_list[u])
				{
					Weight w   = 1.0;
					auto   itw = edge_weights[u].find(v);
					if (itw != edge_weights[u].end())
						w = itw->second;
					Weight nd = d + w;
					if (nd < dist[v])
					{
						dist[v] = nd;
						pq.push({nd, v});
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

			std::vector<T>                  traversal;
			std::unordered_set<std::size_t> visited;

			dfsVisit(it->second, visited, traversal);
			return traversal;
		}

		std::generator<const T&> nodes() const
		{
			for (const auto& node : reverse_index)
				co_yield node;
		}

		std::generator<const WeightedEdge&> edges() const
		{
			for (u64 u = 0; u < adjacent_list.size(); ++u)
			{
				for (u64 v : adjacent_list[u])
				{
					if (u < v)
					{
						WeightedEdge e;
						e.from   = reverse_index[u];
						e.to     = reverse_index[v];
						auto itw = edge_weights[u].find(v);
						if (itw != edge_weights[u].end())
							e.weight = itw->second;
						co_yield e;
					}
				}
			}
			co_return;
		}

		std::vector<WeightedEdge> edges_as_vector() const
		{
			std::vector<WeightedEdge> ret;
			for (u64 u = 0; u < adjacent_list.size(); ++u)
			{
				for (u64 v : adjacent_list[u])
				{
					if (u < v)
					{
						WeightedEdge e;
						e.from   = reverse_index[u];
						e.to     = reverse_index[v];
						auto itw = edge_weights[u].find(v);
						if (itw != edge_weights[u].end())
							e.weight = itw->second;
						ret.push_back(e);
					}
				}
			}
			return ret;
		}

		std::generator<const T&> unconnected_components() const
		{
			assert::check(reverse_index.size() == adjacent_list.size(), "Reverse index and adjacent list should match");


			for (u64 i = 0; i < adjacent_list.size(); ++i)
			{
				if (adjacent_list[i].empty())
					co_yield reverse_index[i];
			}

			co_return;
		}

		std::vector<T> unconnected_components_as_vector() const
		{
			assert::check(reverse_index.size() == adjacent_list.size(), "Reverse index and adjacent list should match");

			std::vector<T> ret;
			ret.reserve(adjacent_list.size());

			for (u64 i = 0; i < adjacent_list.size(); ++i)
			{
				if (adjacent_list[i].empty())
					ret.emplace_back(reverse_index[i]);
			}
			return ret;
		}

		// Return a maximum matching for bipartite graphs using Hopcroft-Karp.
		// If the graph is not bipartite, fall back to a greedy maximal matching.
		auto max_matching() const -> std::vector<std::pair<T, T>>
		{
			u64 n = static_cast<u64>(reverse_index.size());

			// Attempt to 2-color the graph to see if it's bipartite
			std::vector<i64> color(n, -1);
			bool             is_bipartite = true;
			for (u64 i = 0; i < n && is_bipartite; ++i)
			{
				if (color[i] != -1)
					continue;
				std::deque<u64> q;
				color[i] = 0;
				q.push_back(i);
				while (!q.empty() && is_bipartite)
				{
					u64 u = q.front();
					q.pop_front();
					for (u64 v : adjacent_list[u])
					{
						if (color[v] == -1)
						{
							color[v] = color[u] ^ 1;
							q.push_back(v);
						}
						else if (color[v] == color[u])
						{
							is_bipartite = false;
							break;
						}
					}
				}
			}

			if (not is_bipartite)
			{
				std::vector<char>            matched(n, 0);
				std::vector<std::pair<T, T>> result;
				for (u64 u = 0; u < n; ++u)
				{
					if (matched[u])
						continue;
					for (u64 v : adjacent_list[u])
					{
						if (!matched[v])
						{
							matched[u] = matched[v] = 1;
							result.emplace_back(reverse_index[u], reverse_index[v]);
							break;
						}
					}
				}
				return result;
			}

			// Build left/right partitions
			std::vector<i64> left_id(n, -1), right_id(n, -1);
			i64              nl = 0, nr = 0;
			for (u64 i = 0; i < n; ++i)
			{
				if (color[i] == 0)
					left_id[i] = nl++;
				else if (color[i] == 1)
					right_id[i] = nr++;
			}

			// adjacency from left indices to right indices
			std::vector<std::vector<i64>> adjL(static_cast<size_t>(nl));
			for (u64 u = 0; u < n; ++u)
			{
				if (color[u] != 0)
					continue;
				i64 lu = left_id[u];
				for (u64 v : adjacent_list[u])
				{
					if (color[v] != 1)
						continue;
					adjL[lu].push_back(right_id[v]);
				}
			}

			// Hopcroft-Karp
			const int        INF = std::numeric_limits<int>::max();
			std::vector<i64> pairU(static_cast<i64>(nl), -1);
			std::vector<i64> pairV(static_cast<i64>(nr), -1);
			std::vector<i64> dist(static_cast<i64>(nl), 0);

			auto bfs = [&]() -> bool
			{
				std::deque<i64> q;
				for (i64 u = 0; u < nl; ++u)
				{
					if (pairU[u] == -1)
					{
						dist[u] = 0;
						q.push_back(u);
					}
					else
						dist[u] = INF;
				}
				i64 dist_nil = INF;
				while (!q.empty())
				{
					i64 u = q.front();
					q.pop_front();
					if (dist[u] >= dist_nil)
						continue;
					for (i64 v : adjL[u])
					{
						i64 pu = pairV[v];
						if (pu == -1)
						{
							dist_nil = dist[u] + 1;
						}
						else if (dist[pu] == INF)
						{
							dist[pu] = dist[u] + 1;
							q.push_back(pu);
						}
					}
				}
				return dist_nil != INF;
			};

			std::function<bool(i64)> dfs = [&](i64 u) -> bool
			{
				for (i64 v : adjL[u])
				{
					i64 pu = pairV[v];
					if (pu == -1 || (dist[pu] == dist[u] + 1 && dfs(pu)))
					{
						pairU[u] = v;
						pairV[v] = u;
						return true;
					}
				}
				dist[u] = INF;
				return false;
			};

			i64 matching = 0;
			while (bfs())
			{
				for (i64 u = 0; u < nl; ++u)
				{
					if (pairU[u] == -1 && dfs(u))
						++matching;
				}
			}

			std::vector<std::pair<T, T>> result;
			result.reserve(static_cast<size_t>(matching));
			for (i64 u = 0; u < nl; ++u)
			{
				if (pairU[u] != -1)
				{
					// find original node ids
					// left_id: node -> u, need inverse
					// build inverse lazily
				}
			}

			// build inverse maps
			std::vector<u64> inv_left(static_cast<size_t>(nl));
			std::vector<u64> inv_right(static_cast<size_t>(nr));
			for (u64 i = 0; i < n; ++i)
			{
				if (left_id[i] != -1)
					inv_left[left_id[i]] = i;
				if (right_id[i] != -1)
					inv_right[right_id[i]] = i;
			}

			for (i64 u = 0; u < nl; ++u)
			{
				i64 v = pairU[u];
				if (v != -1)
				{
					u64 orig_u = inv_left[u];
					u64 orig_v = inv_right[v];
					result.emplace_back(reverse_index[orig_u], reverse_index[orig_v]);
				}
			}

			return result;
		}

		// return vector of nodes to remove to get an unconnected nodes
		auto independent_set() const -> std::vector<T>
		{
			assert::check(reverse_index.size() == adjacent_list.size(), "Reverse index and adjacent list should match");

			u64               n = static_cast<u64>(reverse_index.size());
			std::vector<char> blocked(n, 0);
			std::vector<T>    result;

			for (u64 u = 0; u < n; ++u)
			{
				if (blocked[u])
					continue;
				result.push_back(reverse_index[u]);
				blocked[u] = 1;
				for (u64 v : adjacent_list[u])
					blocked[v] = 1;
			}

			return result;
		}

		std::vector<std::vector<T>> connected_components(u64 include_groups_larger_than = 1) const
		{
			assert::check(reverse_index.size() == adjacent_list.size(), "Reverse index and adjacent list should match");


			std::vector<std::vector<T>> comps;
			std::vector<char>           seen(reverse_index.size(), 0);

			for (u64 i = 0; i < reverse_index.size(); ++i)
			{
				if (seen[i])
					continue;
				std::vector<T>  comp;
				std::stack<u64> st;
				st.push(i);
				seen[i] = 1;
				while (!st.empty())
				{
					u64 u = st.top();
					st.pop();
					comp.push_back(reverse_index[u]);
					for (u64 v : adjacent_list[u])
					{
						if (!seen[v])
						{
							seen[v] = 1;
							st.push(v);
						}
					}
				}
				if (comp.size() > include_groups_larger_than)
					comps.push_back(std::move(comp));
			}
			return comps;
		}

		std::vector<WeightedEdge> shortest_path(const T& src, const T& dst) const
		{
			auto isrc = index_map.find(src);
			auto idst = index_map.find(dst);
			if (isrc == index_map.end() || idst == index_map.end())
				return {};

			u64 s = isrc->second;
			u64 t = idst->second;
			u64 n = static_cast<u64>(reverse_index.size());

			const Weight        INF = std::numeric_limits<Weight>::infinity();
			std::vector<Weight> dist(n, INF);
			std::vector<u64>    prev(n, static_cast<u64>(-1));

			using PQItem = std::pair<Weight, u64>;
			std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> pq;

			dist[s] = 0.0;
			prev[s] = s;
			pq.push({Weight{0}, s});

			while (!pq.empty())
			{
				auto [d, u] = pq.top();
				pq.pop();
				if (d > dist[u])
					continue;
				if (u == t)
					break;

				for (u64 v : adjacent_list[u])
				{
					Weight w   = 1.0;
					auto   itw = edge_weights[u].find(v);
					if (itw != edge_weights[u].end())
						w = itw->second;
					Weight nd = d + w;
					if (nd < dist[v])
					{
						dist[v] = nd;
						prev[v] = u;
						pq.push({nd, v});
					}
				}
			}

			if (dist[t] == INF)
				return {};

			std::vector<WeightedEdge> edges;
			for (u64 cur = t;;)
			{
				if (prev[cur] == cur)
					break;
				u64    p   = prev[cur];
				Weight w   = 1.0;
				auto   itw = edge_weights[p].find(cur);
				if (itw != edge_weights[p].end())
					w = itw->second;
				edges.push_back({reverse_index[p], reverse_index[cur], w});
				cur = p;
			}

			std::ranges::reverse(edges);
			return edges;
		}

		// Bellman-Ford single-source shortest path (supports negative weights).
		std::vector<WeightedEdge> bellman_ford(const T& src, const T& dst) const
		{
			auto isrc = index_map.find(src);
			auto idst = index_map.find(dst);
			if (isrc == index_map.end() || idst == index_map.end())
				return {};

			u64 s = isrc->second;
			u64 t = idst->second;
			u64 n = static_cast<u64>(reverse_index.size());

			std::vector<std::tuple<u64, u64, Weight>> edges;
			edges.reserve(static_cast<size_t>(edge_count() * 2));
			for (u64 u = 0; u < n; ++u)
			{
				for (u64 v : adjacent_list[u])
				{
					auto   itw = edge_weights[u].find(v);
					Weight w   = 1.0;
					if (itw != edge_weights[u].end())
						w = itw->second;
					edges.emplace_back(u, v, w);
				}
			}

			const Weight        INF = std::numeric_limits<Weight>::infinity();
			std::vector<Weight> dist(n, INF);
			std::vector<u64>    prev(n, static_cast<u64>(-1));

			dist[s] = 0.0;
			prev[s] = s;

			for (u64 i = 0; i + 1 < n; ++i)
			{
				bool changed = false;
				for (const auto& e : edges)
				{
					u64    u;
					u64    v;
					Weight w;
					std::tie(u, v, w) = e;
					if (dist[u] != INF && dist[u] + w < dist[v])
					{
						dist[v] = dist[u] + w;
						prev[v] = u;
						changed = true;
					}
				}
				if (!changed)
					break;
			}

			for (const auto& e : edges)
			{
				u64    u;
				u64    v;
				Weight w;
				std::tie(u, v, w) = e;
				if (dist[u] != INF && dist[u] + w < dist[v])
					return {};
			}

			if (dist[t] == INF)
				return std::vector<WeightedEdge>{};

			std::vector<WeightedEdge> path;
			for (u64 cur = t;;)
			{
				if (prev[cur] == cur)
					break;
				u64    p   = prev[cur];
				Weight w   = 1.0;
				auto   itw = edge_weights[p].find(cur);
				if (itw != edge_weights[p].end())
					w = itw->second;
				path.push_back({reverse_index[p], reverse_index[cur], w});
				cur = p;
			}
			std::ranges::reverse(path);
			return path;
		}

		// Minimum spanning tree (Kruskal) using stored edge weights. Returns forest as list of weighted edges.
		std::vector<WeightedEdge> minimum_spanning_tree() const
		{
			u64                                       n = static_cast<u64>(reverse_index.size());
			std::vector<std::tuple<Weight, u64, u64>> edges;
			edges.reserve(static_cast<size_t>(edge_count()));

			for (u64 u = 0; u < n; ++u)
			{
				for (u64 v : adjacent_list[u])
				{
					if (v > u)
					{
						auto   itw = edge_weights[u].find(v);
						Weight w   = 1.0;
						if (itw != edge_weights[u].end())
							w = itw->second;
						edges.emplace_back(w, u, v);
					}
				}
			}

			std::sort(edges.begin(), edges.end(), [](auto const& a, auto const& b) { return std::get<0>(a) < std::get<0>(b); });

			// union-find
			std::vector<u64> parent(n);
			std::vector<u64> rankv(n, 0);
			for (u64 i = 0; i < n; ++i)
				parent[i] = i;

			auto findp = [&](u64 x)
			{
				u64 r = x;
				while (parent[r] != r)
					r = parent[r];
				// path compression
				u64 cur = x;
				while (parent[cur] != r)
				{
					u64 next    = parent[cur];
					parent[cur] = r;
					cur         = next;
				}
				return r;
			};

			auto unite = [&](u64 a, u64 b) -> bool
			{
				u64 pa = findp(a);
				u64 pb = findp(b);
				if (pa == pb)
					return false;
				if (rankv[pa] < rankv[pb])
					parent[pa] = pb;
				else if (rankv[pb] < rankv[pa])
					parent[pb] = pa;
				else
				{
					parent[pb] = pa;
					rankv[pa]++;
				}
				return true;
			};

			std::vector<WeightedEdge> result;
			result.reserve(n ? static_cast<size_t>(n - 1) : 0);

			for (const auto& [w, u, v] : edges)
			{
				if (unite(u, v))
					result.push_back({reverse_index[u], reverse_index[v], w});
			}

			return result;
		}

		auto bridges() const -> std::vector<std::pair<T, T>>
		{
			u64                          n = static_cast<u64>(reverse_index.size());
			std::vector<int>             disc(n, -1);
			std::vector<int>             low(n, -1);
			std::vector<int>             parent(n, -1);
			std::vector<std::pair<T, T>> result;

			int timer = 0;

			std::function<void(u64)> dfs = [&](u64 u)
			{
				disc[u] = low[u] = timer++;
				for (u64 v : adjacent_list[u])
				{
					if (disc[v] == -1)
					{
						parent[v] = static_cast<int>(u);
						dfs(v);
						low[u] = std::min(low[u], low[v]);
						if (low[v] > disc[u])
						{
							result.emplace_back(reverse_index[u], reverse_index[v]);
						}
					}
					else if (static_cast<int>(v) != parent[u])
					{
						low[u] = std::min(low[u], disc[v]);
					}
				}
			};

			for (u64 i = 0; i < n; ++i)
			{
				if (disc[i] == -1)
					dfs(i);
			}

			return result;
		}

		std::vector<T> articulation_points() const
		{
			u64               n = static_cast<u64>(reverse_index.size());
			std::vector<int>  disc(n, -1);
			std::vector<int>  low(n, -1);
			std::vector<int>  parent(n, -1);
			std::vector<char> is_artic(n, 0);

			int timer = 0;

			std::function<void(u64)> dfs = [&](u64 u)
			{
				disc[u] = low[u] = timer++;
				int children     = 0;
				for (u64 v : adjacent_list[u])
				{
					if (disc[v] == -1)
					{
						children++;
						parent[v] = static_cast<int>(u);
						dfs(v);
						low[u] = std::min(low[u], low[v]);
						if (parent[u] != -1 && low[v] >= disc[u])
							is_artic[u] = 1;
					}
					else if (static_cast<int>(v) != parent[u])
					{
						low[u] = std::min(low[u], disc[v]);
					}
				}
				if (parent[u] == -1 && children > 1)
					is_artic[u] = 1;
			};

			for (u64 i = 0; i < n; ++i)
			{
				if (disc[i] == -1)
					dfs(i);
			}

			std::vector<T> ret;
			for (u64 i = 0; i < n; ++i)
			{
				if (is_artic[i])
					ret.push_back(reverse_index[i]);
			}
			return ret;
		}

		// Return true if graph contains any cycle (undirected)
		auto has_cycle() const -> bool
		{
			u64               n = static_cast<u64>(reverse_index.size());
			std::vector<char> visited(n, 0);
			std::vector<int>  parent(n, -1);

			std::function<bool(u64)> dfs = [&](u64 u) -> bool
			{
				visited[u] = 1;
				for (u64 v : adjacent_list[u])
				{
					if (!visited[v])
					{
						parent[v] = static_cast<int>(u);
						if (dfs(v))
							return true;
					}
					else if (static_cast<int>(v) != parent[u])
					{
						// found a back edge -> cycle
						return true;
					}
				}
				return false;
			};

			for (u64 i = 0; i < n; ++i)
			{
				if (!visited[i])
				{
					if (dfs(i))
						return true;
				}
			}
			return false;
		}

		// Return true if the graph is a tree: connected and acyclic.
		auto is_tree() const -> bool
		{
			u64 n = static_cast<u64>(reverse_index.size());
			if (n == 0)
				return false;

			// check connected
			std::vector<char> seen(n, 0);
			std::stack<u64>   st;
			st.push(0);
			seen[0]   = 1;
			u64 count = 0;
			while (!st.empty())
			{
				u64 u = st.top();
				st.pop();
				++count;
				for (u64 v : adjacent_list[u])
				{
					if (!seen[v])
					{
						seen[v] = 1;
						st.push(v);
					}
				}
			}
			if (count != n)
				return false;

			// no cycles
			return !has_cycle();
		}

		// return cycles in the graph
		auto cycles() const -> std::vector<std::vector<T>>
		{
			u64                             n = static_cast<u64>(reverse_index.size());
			std::vector<char>               visited(n, 0);
			std::vector<int>                parent(n, -1);
			std::unordered_set<std::string> seen_cycles;
			std::vector<std::vector<T>>     cycles;

			auto make_key = [&](const std::vector<u64>& cyc)
			{
				if (cyc.empty())
					return std::string();
				u64              m    = cyc.size();
				std::vector<u64> best = cyc;
				for (u64 r = 0; r < m; ++r)
				{
					std::vector<u64> rot;
					rot.reserve(m);
					for (u64 i = 0; i < m; ++i)
						rot.push_back(cyc[(r + i) % m]);
					if (rot < best)
						best = rot;
				}
				auto rev = best;
				std::reverse(rev.begin(), rev.end());
				if (rev < best)
					best = rev;
				std::string s;
				for (size_t i = 0; i < best.size(); ++i)
				{
					if (i)
						s.push_back(',');
					s += std::to_string(best[i]);
				}
				return s;
			};

			std::function<void(u64)> dfs = [&](u64 u)
			{
				visited[u] = 1;
				for (u64 v : adjacent_list[u])
				{
					if (!visited[v])
					{
						parent[v] = static_cast<int>(u);
						dfs(v);
					}
					else if (static_cast<int>(v) != parent[u])
					{
						// reconstruct cycle from u back to v
						std::vector<u64> path;
						u64              cur = u;
						path.push_back(v);
						while (cur != static_cast<u64>(-1) && cur != v)
						{
							path.push_back(cur);
							cur = static_cast<u64>(parent[cur]);
						}
						if (path.size() >= 3 && path.back() == v)
						{
							std::reverse(path.begin(), path.end());
							std::string key = make_key(path);
							if (!seen_cycles.contains(key))
							{
								seen_cycles.insert(key);
								std::vector<T> cycle_nodes;
								cycle_nodes.reserve(path.size());
								for (u64 idx : path)
									cycle_nodes.push_back(reverse_index[idx]);
								cycles.push_back(std::move(cycle_nodes));
							}
						}
					}
				}
			};

			for (u64 i = 0; i < n; ++i)
			{
				if (!visited[i])
					dfs(i);
			}

			return cycles;
		}

		// Graph radius: minimum eccentricity across all nodes.
		auto radius() const -> std::optional<Weight>
		{
			if (reverse_index.empty())
				return std::nullopt;

			auto                  aps = all_pairs_shortest_paths();
			std::optional<Weight> best;
			for (const auto& v : reverse_index)
			{
				auto   it  = aps.find(v);
				Weight ecc = 0.0;
				if (it != aps.end())
				{
					for (const auto& p : it->second)
					{
						Weight d = p.second;
						if (std::isnan(d))
							continue;
						if (!best.has_value() || d > ecc)
							ecc = std::max(ecc, d);
					}
				}
				if (!best.has_value() || ecc < *best)
					best = ecc;
			}
			return best;
		}

		// Graph diameter: maximum eccentricity across all nodes.
		auto diameter() const -> std::optional<Weight>
		{
			if (reverse_index.empty())
				return std::nullopt;

			auto                  aps = all_pairs_shortest_paths();
			std::optional<Weight> best;
			for (const auto& v : reverse_index)
			{
				auto   it  = aps.find(v);
				Weight ecc = 0.0;
				if (it != aps.end())
				{
					for (const auto& p : it->second)
					{
						Weight d = p.second;
						if (std::isnan(d))
							continue;
						ecc = std::max(ecc, d);
					}
				}
				if (!best.has_value() || ecc > *best)
					best = ecc;
			}
			return best;
		}

		void dump_as_matrix() const
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

		void dump_list() const
		{
			assert::check(reverse_index.size() == adjacent_list.size(), "Reverse index and adjacent list should match");

			for (u64 i = 0; i < adjacent_list.size(); ++i)
			{
				dbg::print("{} :", reverse_index[i]);

				for (const auto& nbr : adjacent_list[i])
				{
					dbg::print(" {}", reverse_index[nbr]);
				}

				dbg::println();
			}
		}
	};

	export template<typename T, typename Weight>
	struct weighted_edge_t
	{
		T      from;
		T      to;
		Weight weight{Weight{0}};

		bool operator==(const weighted_edge_t& rhs) const { return from == rhs.from and to == rhs.to and weight == rhs.weight; }
	};

	export template<typename T>
	using undirected = graph<T, f32, false>;

	export template<typename T>
	using weighted_edge = weighted_edge_t<T, f32>;

	export template<typename T>
	using directed = graph<T, f32, true>;


} // namespace deckard::graph
