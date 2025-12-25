export module deckard.graph;


export import :binarytree;
export import :avltree;
export import :undirected;
import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.arrays;

namespace deckard::graph
{

	export template<typename T, bool Directed = false>
	class graph
	{
	public:
		static constexpr T INF = std::numeric_limits<T>::max();
	private:
		struct weighted_edge
		{
			T   from;
			T   to;
			f64 weight{1.0};
		};

		std::vector<std::unordered_set<u64>>      adjacent_list{};
		std::unordered_map<T, u64>                index_map{};
		std::vector<T>                            reverse_index{};
		std::vector<std::unordered_map<u64, f64>> edge_weights{};

		bool has_edge(u64 u, u64 v) const { return adjacent_list[u].find(v) != adjacent_list[u].end(); }
	public:
		graph() = default;
			: graph(0)
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
			if (not index_map.contains(node))
				return;

			u64 node_index = index_map.at(node);
			u64 last       = static_cast<u64>(reverse_index.size() - 1);

			for (u64 nbr : adjacent_list[node_index])
			{
				adjacent_list[nbr].erase(node_index);
				edge_weights[nbr].erase(node_index);
			}

			if (node_index != last)
			{
				T moved                   = std::move(reverse_index[last]);
				adjacent_list[node_index] = std::move(adjacent_list[last]);
				reverse_index[node_index] = std::move(reverse_index[last]);
				edge_weights[node_index]  = std::move(edge_weights[last]);

				for (u64 nbr : adjacent_list[node_index])
				{
					adjacent_list[nbr].erase(last);
					adjacent_list[nbr].insert(node_index);
					auto itw = edge_weights[nbr].find(last);
					if (itw != edge_weights[nbr].end())
					{
						f64 w = itw->second;
						edge_weights[nbr].erase(itw);
						edge_weights[nbr].emplace(node_index, w);
					}
				}

				index_map[moved] = node_index;
			}

			reverse_index.pop_back();
			adjacent_list.pop_back();
			edge_weights.pop_back();
			index_map.erase(node);
		}

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

			u64 first = iu->second;
			u64 second = iv->second;

			return adjacent_list[first].contains(second);
		}

		void connect(const T& a, const T& b) { connect(a, b, 1.0); }

		void connect(const T& a, const T& b, f64 weight)
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
			auto it = index_map.find(node);
			if (it == index_map.end())
				return {};

			const auto&    adjacent_indices = adjacent_list[it->second];
			std::vector<T> result;
			result.reserve(adjacent_indices.size());

			for (const auto& node_index : adjacent_indices)
				result.push_back(reverse_index[node_index]);

			return result;
		}

		// Copy constructor
		graph(const graph& other)
			: num_vertices(other.num_vertices)
			, adjacency_matrix(other.adjacency_matrix)
			, matrix_view(adjacency_matrix.data(), num_vertices, num_vertices)
		{
		}

		// Copy assignment operator
		graph& operator=(const graph& other)
		{
			if (this == &other)
				return *this;
			num_vertices     = other.num_vertices;
			adjacency_matrix = other.adjacency_matrix;
			matrix_view       = std::mdspan<T, std::extents<std::size_t, std::dynamic_extent, std::dynamic_extent>>(
              adjacency_matrix.data(), num_vertices, num_vertices);
			return *this;
		}

		// Move constructor
		graph(graph&& other) noexcept
			: num_vertices(other.num_vertices)
			, adjacency_matrix(std::move(other.adjacency_matrix))
			, matrix_view(adjacency_matrix.data(), num_vertices, num_vertices)
		{
			other.num_vertices = 0;
		std::generator<const T&> nodes() const
		{
			for (const auto& node : reverse_index)
				co_yield node;
		}

		std::generator<const weighted_edge&> edges() const
		{
			for (u64 u = 0; u < adjacent_list.size(); ++u)
			{
				for (u64 v : adjacent_list[u])
				{
					if (u < v)
					{
						weighted_edge e;
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

		std::vector<weighted_edge> edges_as_vector() const
		{
			std::vector<weighted_edge> ret;
			for (u64 u = 0; u < adjacent_list.size(); ++u)
			{
				for (u64 v : adjacent_list[u])
				{
					if (u < v)
					{
						weighted_edge e;
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
		}

		// Move assignment operator
		graph& operator=(graph&& other) noexcept
		{
			if (this == &other)
				return *this;
			num_vertices       = other.num_vertices;
			adjacency_matrix   = std::move(other.adjacency_matrix);
			matrix_view        = std::mdspan<T, std::extents<std::size_t, std::dynamic_extent, std::dynamic_extent>>(
              adjacency_matrix.data(), num_vertices, num_vertices);
			other.num_vertices = 0;
			return *this;
		}

		// Destructor
		~graph() = default;

		void add(size_t u, size_t v, T weight)
		{
			matrix_view[u, v] = weight;
			matrix_view[v, u] = weight;
		}

		[[nodiscard]] auto getAdjacencyMatrix() const { return matrix_view; }

		[[nodiscard]] size_t getNumVertices() const noexcept { return num_vertices; }
	};

	export template<typename T>
	void print(const graph<T>& g)
	{
		size_t n      = g.getNumVertices();
		auto   matrix = g.getAdjacencyMatrix();

		std::print("Adjacency Matrix:\n");
		for (size_t i = 0; i < n; i++)
		{
			for (size_t j = 0; j < n; j++)
			{
				T weight = matrix[i, j];
				if (weight == graph<T>::INF)
					dbg::print("   ∞ ");
				else
					dbg::print(" {:3} ", weight);
			}
			dbg::println();
		}
	}
	};

	export template<typename T>
	using undirected_graph = graph<T, false>;

	export template<typename T>
	using directed_graph = graph<T, true>;


} // namespace deckard::graph
