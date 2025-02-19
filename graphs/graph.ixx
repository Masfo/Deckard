export module deckard.graph;


export import :binarytree;
export import :avltree;
export import :undirected;
import std;

namespace deckard::graph
{
	export template<typename T>
	class graph
	{
	public:
		static constexpr T INF = std::numeric_limits<T>::max();
	private:
		std::vector<T>                                                                      adjacency_matrix;
		std::mdspan<T, std::extents<std::size_t, std::dynamic_extent, std::dynamic_extent>> matrix_view;

		size_t num_vertices;

	public:
		graph()
			: graph(0)
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

} // namespace deckard::graph
