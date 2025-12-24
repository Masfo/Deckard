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

	public:
		graph() = default;
			: graph(0)
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
		{
		}

		explicit graph(size_t vertices)
			: num_vertices(vertices)
			, adjacency_matrix(vertices * vertices, {})
			, matrix_view(adjacency_matrix.data(), vertices, vertices)
		{
			for (size_t i = 0; i < num_vertices; i++)
				matrix_view[i, i] = {};
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
