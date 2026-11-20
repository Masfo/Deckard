#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.graph;

using namespace deckard;
using namespace deckard::graph;

// █    █   █    █   █████    ██████   █████    ██████    ████    ██████   ██████   █████    
// █    █   ██   █   █    █     ██     █    █   █        █          ██     █        █    █   
// █    █   █ █  █   █    █     ██     █████    ████     █          ██     ████     █    █   
// █    █   █  █ █   █    █     ██     █   █    █        █          ██     █        █    █   
//  ████    █   ██   █████    ██████   █    █   ██████    ████      ██     ██████   █████

  TEST_CASE("Graph/Undirected", "[graph][undirected]")
{
	SECTION("empty")
	{
		undirected<std::string> g;

		CHECK(g.empty() == true);
	}

	SECTION("add nodes")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.add("C");
		CHECK(g.size() == 3);
		CHECK(g.empty() == false);

		CHECK(g.node_count() == 3);
		CHECK(g.edge_count() == 0);
	}

	SECTION("connect nodes")
	{
		undirected<std::string> g;

		g.add("A");
		g.add("B");
		g.add("C");

		g.connect("A", "B");
		g.connect("B", "C");

		CHECK(g.size() == 3);
		CHECK(g.degree("A") == 1);
		CHECK(g.degree("B") == 2);
		CHECK(g.degree("C") == 1);

		CHECK(g.has_edge("A", "B") == true);
		CHECK(g.has_edge("B", "C") == true);
		CHECK(g.has_edge("A", "C") == false);

		CHECK(g.edges_as_vector().size() == 2);
		CHECK(g.edges_as_vector()[0] == weighted_edge<std::string>{"A", "B", 1});
		CHECK(g.edges_as_vector()[1] == weighted_edge<std::string>{"B", "C", 1});
	}

	SECTION("clear")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.connect("A", "B");
		CHECK(g.size() == 2);
		CHECK(g.empty() == false);
		CHECK(g.has_edge("A", "B") == true);

		g.clear();
		CHECK(g.size() == 0);
		CHECK(g.empty() == true);
	}

	SECTION("contains")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.connect("A", "B");
		CHECK(g.size() == 2);
		CHECK(g.empty() == false);
		CHECK(g.has_edge("A", "B") == true);

		CHECK(g.contains("A") == true);
		CHECK(g.contains("B") == true);
		CHECK(g.contains("Z") == false);

		CHECK(g.contains("A", "B") == true);
		CHECK(g.contains("B", "A") == true);
		CHECK(g.contains("Q", "C") == false);
	}

	SECTION("clone")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.connect("A", "B");
		auto g2 = g;
		CHECK(g.size() == 2);
		CHECK(g2.size() == 2);
		CHECK(g.has_edge("A", "B") == true);
		CHECK(g2.has_edge("A", "B") == true);
	}

	SECTION("remove node")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.add("C");
		g.connect("A", "B");
		g.connect("B", "C");
		CHECK(g.size() == 3);
		CHECK(g.edge_count() == 2);

		CHECK(g.has_edge("A", "B") == true);
		CHECK(g.has_edge("B", "C") == true);
		g.remove_node("B");
		CHECK(g.size() == 2);
		CHECK(g.edge_count() == 0);
		CHECK(g.has_edge("A", "B") == false);
		CHECK(g.has_edge("B", "C") == false);

		g.connect("A", "C");
		CHECK(g.size() == 2);
		CHECK(g.edge_count() == 1);
		CHECK(g.has_edge("A", "C") == true);
	}

	SECTION("remove first node")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.add("C");
		g.connect("A", "B");
		g.connect("B", "C");
		CHECK(g.size() == 3);

		g.remove_node("A");
		CHECK(g.size() == 2);
		CHECK(g.contains("A") == false);
		CHECK(g.contains("B") == true);
		CHECK(g.contains("C") == true);
		CHECK(g.has_edge("A", "B") == false);
		CHECK(g.has_edge("B", "C") == true);
	}

	SECTION("remove non-existent node")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.connect("A", "B");
		CHECK(g.size() == 2);

		g.remove_node("Z");
		CHECK(g.size() == 2);
		CHECK(g.has_edge("A", "B") == true);
	}

	SECTION("remove last node")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.add("C");
		g.connect("A", "B");
		g.connect("B", "C");
		CHECK(g.size() == 3);

		g.remove_node("C");
		CHECK(g.size() == 2);
		CHECK(g.contains("A") == true);
		CHECK(g.contains("B") == true);
		CHECK(g.contains("C") == false);
		CHECK(g.has_edge("A", "B") == true);
		CHECK(g.has_edge("B", "C") == false);
	}

	SECTION("remove from single-node graph")
	{
		undirected<std::string> g;
		g.add("A");
		CHECK(g.size() == 1);
		CHECK(g.empty() == false);

		g.remove_node("A");
		CHECK(g.size() == 0);
		CHECK(g.empty() == true);
	}

	SECTION("remove all nodes one by one")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.add("C");
		g.add("D");
		g.connect("A", "B");
		g.connect("B", "C");
		g.connect("C", "D");
		g.connect("D", "A");
		CHECK(g.size() == 4);
		CHECK(g.edge_count() == 4);

		g.remove_node("A");
		CHECK(g.size() == 3);
		CHECK(g.contains("A") == false);

		g.remove_node("B");
		CHECK(g.size() == 2);
		CHECK(g.contains("B") == false);

		g.remove_node("C");
		CHECK(g.size() == 1);
		CHECK(g.contains("C") == false);

		g.remove_node("D");
		CHECK(g.size() == 0);
		CHECK(g.empty() == true);
	}

	SECTION("remove node preserves other edges")
	{
		undirected<std::string> g;
		g.connect("A", "B");
		g.connect("A", "C");
		g.connect("B", "C");
		g.connect("C", "D");
		CHECK(g.size() == 4);
		CHECK(g.edge_count() == 4);

		g.remove_node("A");
		CHECK(g.size() == 3);
		CHECK(g.edge_count() == 2);
		CHECK(g.has_edge("B", "C") == true);
		CHECK(g.has_edge("C", "D") == true);
		CHECK(g.has_edge("A", "B") == false);
		CHECK(g.has_edge("A", "C") == false);
	}


	SECTION("disconnect edge")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.connect("A", "B");
		CHECK(g.size() == 2);
		CHECK(g.edge_count() == 1);
		CHECK(g.has_edge("A", "B") == true);

		g.disconnect("A", "B");

		CHECK(g.size() == 2);
		CHECK(g.edge_count() == 0);
		CHECK(g.has_edge("A", "B") == false);
	}

	SECTION("neighbors")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B
		 *        |      |
		 *        E ---- F
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		CHECK(gr.size() == 6);
		CHECK(gr.edge_count() == 6);

		auto n = gr.neighbors("A"); // B, C, D, E
		CHECK(n.size() == 4);
		CHECK(n[0] == "B");
		CHECK(n[1] == "C");
		CHECK(n[2] == "D");
		CHECK(n[3] == "E");
	}

	SECTION("edges")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B
		 *        |      |
		 *        E ---- F
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		CHECK(gr.size() == 6);
		CHECK(gr.edge_count() == 6);

		auto edges = gr.edges_as_vector();
		CHECK(edges.size() == 6);
		CHECK(edges[0] == weighted_edge<std::string>{"A", "B", 8});
		CHECK(edges[1] == weighted_edge<std::string>{"A", "C", 1});
		CHECK(edges[2] == weighted_edge<std::string>{"A", "D", 3});
		CHECK(edges[3] == weighted_edge<std::string>{"A", "E", 1});
		CHECK(edges[4] == weighted_edge<std::string>{"B", "F", 1});
		CHECK(edges[5] == weighted_edge<std::string>{"E", "F", 1});
	}

	SECTION("shortest path")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B
		 *        |      |
		 *        E ---- F
		 */

		// D-B shortest path: D-A-E-F-B

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);
		//

		CHECK(gr.size() == 6);
		CHECK(gr.edge_count() == 6);

		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "C") == true);
		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "B") == true);

		auto path = gr.shortest_path("D", "B");

		CHECK(path.size() == 4);
		CHECK(path[0] == weighted_edge<std::string>{"D", "A", 3});
		CHECK(path[1] == weighted_edge<std::string>{"A", "E", 1});
		CHECK(path[2] == weighted_edge<std::string>{"E", "F", 1});
		CHECK(path[3] == weighted_edge<std::string>{"F", "B", 1});
		const std::array<std::string, 5> waypoints{"D", "A", "E", "F", "B"};
		CHECK(shortest_path_via_waypoints(gr, waypoints) == path);


		gr.connect("A", "B", 1);
		// D-B shortest path: D-A-B
		path = gr.shortest_path("D", "B");

		CHECK(path.size() == 2);
		CHECK(path[0] == weighted_edge<std::string>{"D", "A", 3});
		CHECK(path[1] == weighted_edge<std::string>{"A", "B", 1});

		gr.add("Z");
		const std::array<std::string, 3> unreachable_waypoint{"D", "Z", "B"};
		CHECK(shortest_path_via_waypoints(gr, unreachable_waypoint).empty());

		const std::array<std::string, 3> missing_waypoint{"D", "missing", "B"};
		CHECK(shortest_path_via_waypoints(gr, missing_waypoint).empty());

		undirected<std::string> waypoint_graph;
		waypoint_graph.connect("S", "A", 1);
		waypoint_graph.connect("S", "B", 2);
		waypoint_graph.connect("A", "B", 1);
		waypoint_graph.connect("A", "G", 5);
		waypoint_graph.connect("B", "G", 1);
		const std::array<std::string, 4> unordered_waypoints{"S", "B", "A", "G"};
		CHECK(shortest_path_via_waypoints(waypoint_graph, unordered_waypoints)
		  == std::vector<weighted_edge<std::string>>{{"S", "A", 1}, {"A", "B", 1}, {"B", "G", 1}});
	}

	SECTION("all pairs")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B
		 *        |      |
		 *        E ---- F
		 */


		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);
		//

		CHECK(gr.size() == 6);
		CHECK(gr.edge_count() == 6);

		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "C") == true);
		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "B") == true);

		auto allpairs = gr.all_pairs_shortest_paths();

		CHECK(allpairs.size() == 6);
		CHECK(allpairs["A"]["A"] == 0);
		CHECK(allpairs["A"]["B"] == 3);
		CHECK(allpairs["F"]["C"] == 3);
	}

	SECTION("max matching")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B    Q -- Z
		 *        |      |    |    |
		 *        E ---- F    Y ---|
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		gr.connect("Q", "Z", 1);
		gr.connect("Q", "Y", 1);
		gr.connect("Y", "Z", 1);

		CHECK(gr.size() == 9);
		CHECK(gr.edge_count() == 9);

		auto matching = gr.max_matching();

		/* A-B, E-F, Q-Z */
		CHECK(matching.size() == 3);
		CHECK(matching[0] == std::pair{"A", "B"});
		CHECK(matching[1] == std::pair{"E", "F"});
		CHECK(matching[2] == std::pair{"Q", "Z"});
	}

	SECTION("connected components")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B    Q -- Z
		 *        |      |    |    |
		 *        E ---- F    Y ---|
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		gr.connect("Q", "Z", 1);
		gr.connect("Q", "Y", 1);
		gr.connect("Y", "Z", 1);

		CHECK(gr.size() == 9);
		CHECK(gr.edge_count() == 9);

		auto connected = gr.connected_components();

		CHECK(connected.size() == 2);
		CHECK(connected[0].size() == 6); // AEFDCB
		CHECK(connected[1].size() == 3); // QYZ
	}

	SECTION("Independent set")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B    Q -- Z
		 *        |      |    |
		 *        E ---- F    Y
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		gr.connect("Q", "Z", 1);
		gr.connect("Q", "Y", 1);

		CHECK(gr.size() == 9);
		CHECK(gr.edge_count() == 8);

		auto iset = gr.independent_set();

		/* A, F, Q
				  C
		 *
		 *  D            B         Z
		 *
		 *        E           Y
		 */

		CHECK(iset.size() == 3);
		CHECK(iset[0] == "A");
		CHECK(iset[1] == "F");
		CHECK(iset[2] == "Q");

		gr.remove_nodes(iset);
		CHECK(gr.size() == 6);
		CHECK(gr.edge_count() == 0);

		CHECK(gr.connected_components(0).size() == 6);
		CHECK(gr.unconnected_components_as_vector().size() == 6);
		CHECK(gr.connected_components(1).size() == 0); // filter size > 1


		for (const auto& node : gr.unconnected_components())
		{
			CHECK(node.size() == 1);
		}
	}

	SECTION("minimum spanning tree")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B    Q -- Z
		 *        |      |    |
		 *        E ---- F    Y
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		gr.connect("Q", "Z", 1);
		gr.connect("Q", "Y", 1);

		CHECK(gr.size() == 9);
		CHECK(gr.edge_count() == 8);

		auto mst = gr.minimum_spanning_tree();

		/*		  C
		 *		  |
		 *  D --- A      B    Q -- Z
		 *        |      |    |
		 *        E ---- F    Y
		 */

		CHECK(mst.size() == 7);
		CHECK(mst[0] == weighted_edge<std::string>{"A", "C", 1});
		CHECK(mst[1] == weighted_edge<std::string>{"A", "E", 1});
		CHECK(mst[2] == weighted_edge<std::string>{"B", "F", 1});
		CHECK(mst[3] == weighted_edge<std::string>{"E", "F", 1});
		CHECK(mst[4] == weighted_edge<std::string>{"Q", "Z", 1});
		CHECK(mst[5] == weighted_edge<std::string>{"Q", "Y", 1});
		CHECK(mst[6] == weighted_edge<std::string>{"A", "D", 3});
	}

	SECTION("minimum cut")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B -- Q -- Z
		 *        |      |    |
		 *        E ---- F    Y
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		gr.connect("Q", "Z", 1);
		gr.connect("Q", "Y", 1);

		gr.connect("Q", "B", 9);

		CHECK(gr.size() == 9);
		CHECK(gr.edge_count() == 9);


		auto [cut_weight, partition] = gr.minimum_cut();

		/*		  C
		 *		  |
		 *  D --- A ---- B -- Q -- Z
		 *        |      |
		 *        E ---- F    Y
		 */

		CHECK(cut_weight == 1);
		CHECK(not partition.empty());
		CHECK(partition.size() < gr.size());

		std::unordered_set<std::string> partition_nodes(partition.begin(), partition.end());
		f32                             cut_weight_from_edges{};
		for (const auto& edge : gr.edges_as_vector())
		{
			if (partition_nodes.contains(edge.from) != partition_nodes.contains(edge.to))
				cut_weight_from_edges += edge.weight;
		}
		CHECK(cut_weight_from_edges == cut_weight);
	}

	SECTION("bridges")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B -- Q -- Z
		 *        |      |    |
		 *        E ---- F    Y
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		gr.connect("Q", "Z", 1);
		gr.connect("Q", "Y", 1);

		gr.connect("Q", "B", 7);

		CHECK(gr.size() == 9);
		CHECK(gr.edge_count() == 9);

		auto bridges = gr.bridges();

		/*		  C
		 *
		 *  D     A ---- B    Q    Z
		 *        |      |
		 *        E ---- F    Y
		 */

		CHECK(bridges.size() == 5);
		auto has_bridge = [&](std::string_view from, std::string_view to, f32 weight)
		{
			return std::ranges::any_of(
			  bridges,
			  [&](const auto& bridge)
			  {
				  return ((bridge.from == from and bridge.to == to) or (bridge.from == to and bridge.to == from))
						 and bridge.weight == weight;
			  });
		};
		CHECK(has_bridge("A", "C", 1));
		CHECK(has_bridge("A", "D", 3));
		CHECK(has_bridge("Q", "B", 7));
		CHECK(has_bridge("Q", "Z", 1));
		CHECK(has_bridge("Q", "Y", 1));
	}

	SECTION("articulation points")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B    Q -- Z
		 *        |      |    |
		 *        E ---- F    Y
		 */

		undirected<std::string> gr;
		gr.connect("A", "B", 8);
		gr.connect("A", "C", 1);
		gr.connect("A", "D", 3);
		gr.connect("A", "E", 1);

		gr.connect("B", "F", 1);
		gr.connect("E", "F", 1);

		gr.connect("Q", "Z", 1);
		gr.connect("Q", "Y", 1);

		CHECK(gr.size() == 9);
		CHECK(gr.edge_count() == 8);

		auto ap = gr.articulation_points();

		CHECK(ap.size() == 2);
		CHECK(ap[0] == "A");
		CHECK(ap[1] == "Q");

		/* A, Q
		 *        C
		 *
		 *  D            B         Z
		 *               |
		 *        E ---- F    Y
		 */

		gr.remove_nodes(ap);
		auto cp = gr.connected_components(0);
		CHECK(gr.size() == 7);
		CHECK(gr.connected_components(0).size() == 5); // Sets: D, C, [B,F,E], Z, Y
		CHECK(gr.connected_components(1).size() == 1); // Sets [B,F,E]
	}


	SECTION("compress")
	{
		undirected<std::string> gr;

		/*

			A -- B --- C --- D


				 Z

		*/

		gr.add("Z");
		gr.connect("A", "B", 2);
		gr.connect("B", "C", 2);
		gr.connect("C", "D", 2);
		CHECK(gr.size() == 5);
		CHECK(gr.edge_count() == 3);

		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.weight("A", "B") == 2);

		CHECK(gr.has_edge("B", "C") == true);
		CHECK(gr.weight("B", "C") == 2);

		CHECK(gr.has_edge("C", "D") == true);
		CHECK(gr.weight("C", "D") == 2);

		CHECK(gr.has_edge("A", "Z") == false);
		CHECK(gr.weight("A", "Z") == std::nullopt);

		gr.compress();

		/*

			A -- B --- C --- D

		*/
		CHECK(gr.size() == 4);
		CHECK(gr.edge_count() == 3);
		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("B", "C") == true);
		CHECK(gr.has_edge("C", "D") == true);
	}

	SECTION("suppress degree-2 vertices")
	{
		undirected<std::string> gr;
		gr.add("Z");
		gr.connect("A", "B", 2);
		gr.connect("B", "C", 3);
		gr.connect("C", "D", 4);

		gr.suppress_degree_2_vertices();

		CHECK(gr.size() == 3);
		CHECK(gr.edge_count() == 1);
		CHECK(gr.has_node("B") == false);
		CHECK(gr.has_node("C") == false);
		CHECK(gr.has_node("Z") == true);
		CHECK(gr.has_edge("A", "D") == true);
		CHECK(gr.weight("A", "D") == 9);
	}

	SECTION("difference")
	{
		/*		  C
		 *		  |
		 *  D --- A ---- B    Q -- Z
		 *        |      |    |
		 *        E ---- F    Y
		 */

		undirected<std::string> gr1;
		gr1.connect("A", "B", 8);
		gr1.connect("A", "C", 1);
		gr1.connect("A", "D", 3);
		gr1.connect("A", "E", 1);

		gr1.connect("B", "F", 1);
		gr1.connect("E", "F", 1);

		gr1.connect("Q", "Z", 1);
		gr1.connect("Q", "Y", 1);

		CHECK(gr1.size() == 9);
		CHECK(gr1.edge_count() == 8);


		/*		  C
		 *		  |
		 *  D --- A ---- B
		 *        |      |
		 *        E ---- F
		 */
		undirected<std::string> gr2;
		gr2.connect("A", "B", 8);
		gr2.connect("A", "C", 1);
		gr2.connect("A", "D", 3);
		gr2.connect("A", "E", 1);

		gr2.connect("B", "F", 1);
		gr2.connect("E", "F", 1);

		CHECK(gr2.size() == 6);
		CHECK(gr2.edge_count() == 6);

		auto diff = gr1.difference(gr2);
		CHECK_FALSE(diff.empty());
		CHECK(diff.size() == 3);
		CHECK(std::ranges::contains(diff, "Q"));
		CHECK(std::ranges::contains(diff, "Y"));
		CHECK(std::ranges::contains(diff, "Z"));
	}

	SECTION("difference_graph")
	{
		/*
				A -- B -- C
				|         |
				|         |      E
				D---------/
		*/
		undirected<std::string> gr1;
		gr1.connect("A", "B", 2);
		gr1.connect("A", "D", 7);
		gr1.connect("B", "C", 3);
		gr1.connect("C", "D", 4);
		gr1.add("E");

		undirected<std::string> gr2;
		gr2.add("B");
		gr2.add("C");

		/*
				B      C

		*/

		auto diff = gr1.difference_graph(gr2);
		CHECK(diff.size() == 3);
		CHECK(diff.contains("A") == true);
		CHECK(diff.contains("D") == true);
		CHECK(diff.contains("E") == true);
		CHECK(diff.contains("B") == false);
		CHECK(diff.contains("C") == false);
		CHECK(diff.edge_count() == 1);
		CHECK(diff.has_edge("A", "D") == true);
		CHECK(diff.weight("A", "D") == 7);
	}

	SECTION("leafs and nodes at rank")
	{
		/*
		
			A --- B     E
			|        
			|
			C --- D
		*/

		undirected<std::string> g;
		g.connect("A", "B", 8);
		g.connect("A", "C", 1);
		g.connect("C", "D", 1);
		g.add("E");

		CHECK(g.collect_leafs() == std::vector<std::string>{"B", "D"});
		CHECK(g.collect_leafs<std::list<char>>([](const auto& node) { return node.front(); }) == std::list<char>{'B', 'D'});

		std::vector<std::string> generated_leafs;
		for (const auto& leaf : g.leafs())
			generated_leafs.push_back(*leaf);
		CHECK(generated_leafs == std::vector<std::string>{"B", "D"});

		CHECK(g.collect_at_rank("A", 1) == std::vector<std::string>{"A"});
		CHECK(g.collect_at_rank("A", 2) == std::vector<std::string>{"B", "C"});
		CHECK(g.collect_at_rank("A", 3) == std::vector<std::string>{"D"});
		std::vector<std::string> generated_rank;
		for (const auto& value : g.at_rank("A", 2))
			generated_rank.push_back(*value);
		CHECK(generated_rank == std::vector<std::string>{"B", "C"});
		CHECK(g.collect_at_rank("missing", 1).empty());
		CHECK(g.collect_at_rank("A", 0).empty());
	}

	SECTION("print")
	{
		undirected<std::string> g;
		g.connect("A", "B");
		g.add("C");

		CHECK(g.to_string() == "A : B\nB : A\nC :");
	}
}
 
// █████    ██████   █████    ██████    ████    ██████   ██████   █████    
// █    █     ██     █    █   █        █          ██     █        █    █   
// █    █     ██     █████    ████     █          ██     ████     █    █   
// █    █     ██     █   █    █        █          ██     █        █    █   
// █████    ██████   █    █   ██████    ████      ██     ██████   █████    
 


TEST_CASE("Graph/Directed", "[graph][directed]")
{
	SECTION("empty")
	{
		directed<std::string> g;
		CHECK(g.size() == 0);
		CHECK(g.node_count() == 0);
		CHECK(g.edge_count() == 0);
		CHECK(g.empty() == true);
	}

	SECTION("add nodes and connect directed edges")
	{
		directed<std::string> g;
		g.add("A");
		g.add("B");
		g.add("C");
		g.connect("A", "B", 4);
		g.connect("C", "A");

		CHECK(g.size() == 3);
		CHECK(g.node_count() == 3);
		CHECK(g.edge_count() == 2);
		CHECK(g.degree("A") == 1);
		CHECK(g.degree("B") == 0);
		CHECK(g.degree("C") == 1);
		CHECK(g.has_edge("A", "B") == true);
		CHECK(g.has_edge("B", "A") == false);
		CHECK(g.has_edge("C", "A") == true);
		CHECK(g.weight("A", "B") == 4);
		CHECK(g.weight("B", "A").has_value() == false);
	}

	SECTION("edges preserve direction")
	{
		directed<std::string> g;
		g.add("A");
		g.add("B");
		g.add("C");
		g.connect("C", "A", 3);
		g.connect("A", "B", 2);

		auto edges = g.edges_as_vector();
		CHECK(edges.size() == 2);
		CHECK(edges[0] == weighted_edge<std::string>{"A", "B", 2});
		CHECK(edges[1] == weighted_edge<std::string>{"C", "A", 3});

		std::vector<weighted_edge<std::string>> generated_edges;
		for (const auto& edge : g.edges())
			generated_edges.push_back(edge);
		CHECK(generated_edges == edges);
	}

	SECTION("difference graph preserves directed edges")
	{
		/*
			A --2--> B --4--> C
			A <--3-- B
			A --7--> D
			A <--9-- D

			Remove B and C; keep A and D with both directed edges.
		*/
		directed<std::string> g;
		g.connect("A", "B", 2);
		g.connect("B", "A", 3);
		g.connect("A", "D", 7);
		g.connect("D", "A", 9);
		g.connect("B", "C", 4);

		directed<std::string> other;
		other.add("B");
		other.add("C");

		auto diff = g.difference_graph(other);
		CHECK(diff.size() == 2);
		CHECK(diff.edge_count() == 2);
		CHECK(diff.has_edge("A", "D") == true);
		CHECK(diff.has_edge("D", "A") == true);
		CHECK(diff.weight("A", "D") == 7);
		CHECK(diff.weight("D", "A") == 9);
		CHECK(diff.has_edge("A", "B") == false);
	}

	SECTION("copy and clear")
	{
		directed<std::string> g;
		g.connect("A", "B", 2);
		auto copy = g;

		CHECK(copy.edge_count() == 1);
		CHECK(copy.has_edge("A", "B") == true);
		CHECK(copy.has_edge("B", "A") == false);

		g.clear();
		CHECK(g.empty() == true);
		CHECK(g.edge_count() == 0);
		CHECK(copy.has_edge("A", "B") == true);
	}

	SECTION("disconnect edge")
	{
		directed<std::string> g;
		g.connect("A", "B");
		g.connect("B", "A");

		g.disconnect("A", "B");

		CHECK(g.edge_count() == 1);
		CHECK(g.has_edge("A", "B") == false);
		CHECK(g.has_edge("B", "A") == true);
	}

	SECTION("remove node removes incoming and outgoing edges")
	{
		directed<std::string> g;
		g.connect("A", "B");
		g.connect("B", "C");
		g.connect("C", "A");

		g.remove_node("B");

		CHECK(g.contains("B") == false);
		CHECK(g.size() == 2);
		CHECK(g.edge_count() == 1);
		CHECK(g.has_edge("A", "B") == false);
		CHECK(g.has_edge("B", "C") == false);
		CHECK(g.has_edge("C", "A") == true);
	}

	SECTION("shortest path follows edge direction")
	{
		directed<std::string> g;
		g.connect("A", "B", 2);
		g.connect("B", "C", 3);
		g.connect("A", "C", 10);

		auto path = g.shortest_path("A", "C");
		CHECK(path.size() == 2);
		CHECK(path[0] == weighted_edge<std::string>{"A", "B", 2});
		CHECK(path[1] == weighted_edge<std::string>{"B", "C", 3});
		CHECK(g.shortest_path("C", "A").empty());
	}

	SECTION("leafs and nodes at rank")
	{
		directed<std::string> g;
		g.connect("A", "B");
		g.connect("A", "C");
		g.connect("C", "D");
		g.connect("D", "C");
		g.add("E");

		CHECK(g.collect_leafs() == std::vector<std::string>{"B", "E"});
		CHECK(g.collect_at_rank("A", 2) == std::vector<std::string>{"B", "C"});
		CHECK(g.collect_at_rank("A", 3) == std::vector<std::string>{"D"});
		CHECK(g.collect_at_rank("B", 2).empty());
	}

	SECTION("Bellman-Ford shortest path")
	{
		directed<std::string> g;
		g.connect("S", "A", 4);
		g.connect("S", "B", 5);
		g.connect("A", "B", -2);
		g.connect("B", "C", 3);
		g.connect("A", "C", 10);
		g.add("D");

		auto path = g.shortest_path_bellman_ford("S", "C");
		CHECK(path.size() == 3);
		CHECK(path[0] == weighted_edge<std::string>{"S", "A", 4});
		CHECK(path[1] == weighted_edge<std::string>{"A", "B", -2});
		CHECK(path[2] == weighted_edge<std::string>{"B", "C", 3});
		CHECK(g.shortest_path_bellman_ford("S", "D").empty());

		directed<std::string> negative_cycle;
		negative_cycle.connect("S", "A", 1);
		negative_cycle.connect("A", "B", -2);
		negative_cycle.connect("B", "A", -2);
		CHECK(negative_cycle.shortest_path_bellman_ford("S", "B").empty());

		directed<std::string> unrelated_negative_cycle;
		unrelated_negative_cycle.connect("S", "T", 5);
		unrelated_negative_cycle.connect("S", "A", 1);
		unrelated_negative_cycle.connect("A", "B", -2);
		unrelated_negative_cycle.connect("B", "A", 1);
		auto unaffected_path = unrelated_negative_cycle.shortest_path_bellman_ford("S", "T");
		CHECK(unaffected_path.size() == 1);
		CHECK(unaffected_path[0] == weighted_edge<std::string>{"S", "T", 5});
	}
}

// █████    ██████   █    █    ████    █████    █    █   ██████   █████    ██████   ██████   
// █    █     ██     ██   █   █    █   █    █    █  █      ██     █    █   █        █        
// █████      ██     █ █  █   ██████   █████      ██       ██     █████    ████     ████     
// █    █     ██     █  █ █   █    █   █   █      ██       ██     █   █    █        █        
// █████    ██████   █   ██   █    █   █    █     ██       ██     █    █   ██████   ██████   

TEST_CASE("Binary Tree", "[binarytree]")
{
	SECTION("empty")
	{
		binary::tree<i32> tree;

		CHECK(tree.size() == 0);
		CHECK(tree.empty() == true);
		CHECK(tree.height() == 0);
	}

	SECTION("insert")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);
		CHECK(tree.size() == 7);
		CHECK(tree.empty() == false);
		CHECK(tree.height() == 5);
	}

	SECTION("nodes at rank")
	{
		binary::tree<std::pair<i32, char>> tree;
		tree.insert({10, 'A'});
		tree.insert({15, 'D'});
		tree.insert({12, 'F'});
		tree.insert({5, 'B'});
		tree.insert({3, 'C'});
		tree.insert({20, 'G'});
		tree.insert({4, 'E'});
		CHECK(tree.height() == 4);


		auto values = tree.collect_at_rank(3);
		CHECK(values.size() == 3);
		CHECK(values[0].second == 'C');
		CHECK(values[1].second == 'F');
		CHECK(values[2].second == 'G');
		CHECK(tree.collect_at_rank(0).empty());
		CHECK(tree.collect_at_rank(tree.size() + 1).empty());

		auto labels = tree.collect_at_rank(3, [](const auto& value) { return value.second; });
		CHECK(labels == std::vector<char>{'C', 'F', 'G'});
		auto label_list = tree.collect_at_rank<std::list<char>>(3, [](const auto& value) { return value.second; });
		CHECK(label_list == std::list<char>{'C', 'F', 'G'});
		auto label_map = tree.collect_at_rank<std::unordered_map<i32, char>>(3);
		CHECK(label_map == std::unordered_map<i32, char>{{3, 'C'}, {12, 'F'}, {20, 'G'}});

		auto label_string = tree.collect_at_rank<std::string>(3, [](const auto& value) { return value.second; });
		CHECK(label_string == std::string("CFG"));

		std::vector<char> generated_labels;
		for (const auto& value : tree.at_rank(3))
		{
			CHECK(value.has_value());
			generated_labels.push_back(value->second);
		}
		CHECK(generated_labels == std::vector<char>{'C', 'F', 'G'});

		std::vector<char> projected_labels;
		for (const auto& value : tree.at_rank(3, [](const auto& item) { return item.second; }))
		{
			CHECK(value.has_value());
			projected_labels.push_back(*value);
		}
		CHECK(projected_labels == std::vector<char>{'C', 'F', 'G'});

		auto rank_zero = tree.at_rank(0);
		CHECK(rank_zero.begin() == rank_zero.end());
		auto missing_rank = tree.at_rank(tree.size() + 1);
		CHECK(missing_rank.begin() == missing_rank.end());
	}

	SECTION("clone")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);

		auto tree2 = tree;

		CHECK(tree.size() == 7);
		CHECK(tree2.size() == 7);
	}

	SECTION("remove")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);

		auto tree2 = tree;

		tree.remove(40);

		CHECK(tree.size() == 6);
		CHECK(tree2.size() == 7);
	}

	SECTION("leafs")
	{

		/*
		└──10
		    ├──2
            │   └──4
            │       └──6
            └──30
                └──70
                    └──80
		*/ 
		binary::tree<i32> tree;
		tree.insert(10);
		tree.insert(30);
		tree.insert(2);
		tree.insert(4);
		tree.insert(70);
		tree.insert(6);
		tree.insert(80);

		auto leafs = tree.collect_leafs();
		CHECK(leafs.size() == 2);
		CHECK(std::ranges::contains(leafs, 6));
		CHECK(std::ranges::contains(leafs, 80));

		std::vector<i32> generated_leafs;
		for (const auto& leaf : tree.leafs())
		{
			CHECK(leaf.has_value());
			generated_leafs.push_back(*leaf);
		}
		CHECK(generated_leafs == std::vector<i32>{6, 80});

		auto leafs_vector = tree.collect_leafs<std::vector<i32>>();
		CHECK(leafs_vector.size() == 2);
		CHECK(leafs_vector == std::vector<i32>{6, 80});


	}

	SECTION("print") 
	{ 
		/*
		     50
		     ├── 25
		     │   ├── 10
		     │   └── ·
		     └── 75
		*/

		binary::tree<i32> tree;
		tree.insert(50, 25, 75, 10);
		CHECK(tree.size() == 4);

		auto printed = tree.to_string();
		CHECK(printed.find("50") != std::string::npos);

	}

	SECTION("has")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);

		CHECK(tree.size() == 7);
		CHECK(true == tree.contains(10));
		CHECK(true == tree.contains(80));
		CHECK(false == tree.contains(0));
	}

	SECTION("clear")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);
		CHECK(tree.size() == 7);
		CHECK(tree.empty() == false);

		tree.clear();
		CHECK(tree.size() == 0);
		CHECK(tree.empty() == true);
	}

	SECTION("get node/height")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		CHECK(tree.size() == 4);
		CHECK(tree.empty() == false);

	}

	SECTION("at")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);

		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2

		const auto n = tree.at(10);
		CHECK(n.has_value());
		CHECK((*n)->data == 10);

		CHECK(not tree.at(0).has_value());
	}

	SECTION("visit inorder")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		std::vector<i32> v;
		const auto       to_vector = [&](i32 value) { v.push_back(value); };

		tree.visit(to_vector);





		CHECK(v.size() == 4);
		CHECK(v[0] == 10);
		CHECK(v[1] == 30);
		CHECK(v[2] == 60);
		CHECK(v[3] == 80);
	}

	SECTION("visit preorder")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		std::vector<i32> v;
		auto             to_vector = [&](i32 value) { v.push_back(value); };

		tree.visit(binary::order::pre, to_vector);

		// 10 -> 30 -> 60 -> 80

		CHECK(v.size() == 4);
		CHECK(v[0] == 10);
		CHECK(v[1] == 30);
		CHECK(v[2] == 60);
		CHECK(v[3] == 80);
	}

	SECTION("visit postorder")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		std::vector<i32> v;
		auto             to_vector = [&](i32 value) { v.push_back(value); };

		tree.visit(binary::order::post, to_vector);

		// 10 -> 30 -> 60 -> 80

		CHECK(v.size() == 4);
		CHECK(v[0] == 80);
		CHECK(v[1] == 60);
		CHECK(v[2] == 30);
		CHECK(v[3] == 10);
	}

	SECTION("visit levelorder")
	{
		binary::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		std::vector<i32> v;
		auto             to_vector = [&](i32 value) { v.push_back(value); };

		tree.visit(binary::order::level, to_vector);

		// 10 -> 30 -> 60 -> 80

		CHECK(v.size() == 4);
		CHECK(v[0] == 10);
		CHECK(v[1] == 30);
		CHECK(v[2] == 60);
		CHECK(v[3] == 80);
	}

	SECTION("insert count")
	{
		binary::tree<i32> tree;
		tree.insert(10);
		CHECK(tree.size() == 1);

		tree.insert(10);
		CHECK(tree.size() == 1);

		tree.insert(5, 15, 20);
		CHECK(tree.size() == 4);
	}

	SECTION("remove count")
	{
		binary::tree<i32> tree;
		tree.insert(10, 5, 15, 20);

		tree.remove(99);
		CHECK(tree.size() == 4);

		tree.remove(10);
		CHECK(tree.size() == 3);
		CHECK(tree.contains(10) == false);

		tree.remove(5, 15, 20, 99);
		CHECK(tree.size() == 0);
		CHECK(tree.empty() == true);
	}
}

//  ████    █    █   █        
// █    █   █    █   █        
// ██████   █    █   █        
// █    █    █  █    █        
// █    █     ██     ██████   


TEST_CASE("AVL", "[avl]")
{
	SECTION("empty")
	{
		avl::tree<i32> tree;

		CHECK(tree.size() == 0);
		CHECK(tree.empty() == true);
	}

	SECTION("insert")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);
		CHECK(tree.size() == 7);
		CHECK(tree.empty() == false);
	}

	SECTION("clone")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);

		auto tree2 = tree;

		CHECK(tree.size() == 7);
		CHECK(tree2.size() == 7);
	}

	SECTION("remove")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);

		auto tree2 = tree;

		tree.remove(40);

		CHECK(tree.size() == 6);
		CHECK(tree2.size() == 7);
	}

	SECTION("has")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);

		CHECK(tree.size() == 7);
		CHECK(true == tree.contains(10));
		CHECK(true == tree.contains(80));
		CHECK(false == tree.contains(0));
	}

	SECTION("clear")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(20);
		tree.insert(40);
		tree.insert(70);
		tree.insert(60);
		tree.insert(80);
		CHECK(tree.size() == 7);
		CHECK(tree.empty() == false);

		tree.clear();
		CHECK(tree.size() == 0);
		CHECK(tree.empty() == true);
	}


	SECTION("get node/height")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		CHECK(tree.size() == 4);
		CHECK(tree.empty() == false);

		CHECK(tree.get(10)->height == 0);
		CHECK(tree.get(30)->height == 2);
		CHECK(tree.get(60)->height == 1);
		CHECK(tree.get(80)->height == 0);

		tree.insert(100);
		CHECK(tree.size() == 5);
		CHECK(tree.get(10)->height == 0);
		CHECK(tree.get(30)->height == 2);
		CHECK(tree.get(60)->height == 0);
		CHECK(tree.get(80)->height == 1);
		CHECK(tree.get(100)->height == 0);

		tree.insert(5);
		CHECK(tree.size() == 6);
		CHECK(tree.get(5)->height == 0);
		CHECK(tree.get(10)->height == 1);
		CHECK(tree.get(30)->height == 2);
		CHECK(tree.get(60)->height == 0);
		CHECK(tree.get(80)->height == 1);
		CHECK(tree.get(100)->height == 0);
	}

	SECTION("at")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);

		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2

		const auto n = tree.at(10);
		CHECK(n.has_value());
		CHECK((*n)->data == 10);

		CHECK(not tree.at(0).has_value());
	}

	SECTION("visit inorder")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		std::vector<i32> v;
		auto             to_vector = [&](i32 value) { v.push_back(value); };

		tree.visit(to_vector);


		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2


		CHECK(v.size() == 4);
		CHECK(v[0] == 10);
		CHECK(v[1] == 30);
		CHECK(v[2] == 60);
		CHECK(v[3] == 80);
	}

	SECTION("visit preorder")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		std::vector<i32> v;
		auto             to_vector = [&](i32 value) { v.push_back(value); };

		tree.visit(avl::order::pre, to_vector);

		// 30 -> 10, 60 -> 80

		CHECK(v.size() == 4);
		CHECK(v[0] == 30);
		CHECK(v[1] == 10);
		CHECK(v[2] == 60);
		CHECK(v[3] == 80);
	}

	SECTION("visit postorder")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		std::vector<i32> v;
		auto             to_vector = [&](i32 value) { v.push_back(value); };

		tree.visit(avl::order::post, to_vector);

		// 30 -> 10, 60 -> 80

		CHECK(v.size() == 4);
		CHECK(v[0] == 10);
		CHECK(v[1] == 80);
		CHECK(v[2] == 60);
		CHECK(v[3] == 30);
	}

	SECTION("visit levelorder")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		std::vector<i32> v;
		auto             to_vector = [&](i32 value) { v.push_back(value); };

		tree.visit(avl::order::level, to_vector);

		// 30 -> 10, 60 -> 80

		CHECK(v.size() == 4);
		CHECK(v[0] == 30);
		CHECK(v[1] == 10);
		CHECK(v[2] == 60);
		CHECK(v[3] == 80);
	}

	
	SECTION("leafs")
	{
		avl::tree<i32> tree;
		tree.insert(10, 20, 30, 40);

		CHECK(tree.collect_leafs() == std::vector<i32>{10, 40});
		auto projected = tree.collect_leafs<std::list<i32>>([](i32 value) { return value * 2; });
		CHECK(projected == std::list<i32>{20, 80});

		std::vector<i32> generated_leafs;
		for (const auto& leaf : tree.leafs())
		{
			CHECK(leaf.has_value());
			generated_leafs.push_back(*leaf);
		}
		CHECK(generated_leafs == std::vector<i32>{10, 40});

		std::vector<i32> projected_leafs;
		for (const auto& leaf : tree.leafs([](i32 value) { return value * 2; }))
		{
			CHECK(leaf.has_value());
			projected_leafs.push_back(*leaf);
		}
		CHECK(projected_leafs == std::vector<i32>{20, 80});
	}

	SECTION("at rank")
	{
		avl::tree<std::pair<i32, char>> tree;
		tree.insert({10, 'A'});
		tree.insert({15, 'D'});
		tree.insert({12, 'F'});
		tree.insert({5, 'B'});
		tree.insert({3, 'C'});
		tree.insert({20, 'G'});
		tree.insert({4, 'E'});

		auto values = tree.collect_at_rank(3);
		CHECK(values.size() == 3);
		CHECK(values[0].second == 'C');
		CHECK(values[1].second == 'A');
		CHECK(values[2].second == 'G');
		CHECK(tree.collect_at_rank(0).empty());
		CHECK(tree.collect_at_rank(tree.size() + 1).empty());

		auto labels = tree.collect_at_rank(3, [](const auto& value) { return value.second; });
		CHECK(labels == std::vector<char>{'C', 'A', 'G'});
		auto label_list = tree.collect_at_rank<std::list<char>>(3, [](const auto& value) { return value.second; });
		CHECK(label_list == std::list<char>{'C', 'A', 'G'});
		auto label_map = tree.collect_at_rank<std::unordered_map<i32, char>>(3);
		CHECK(label_map == std::unordered_map<i32, char>{{3, 'C'}, {10, 'A'}, {20, 'G'}});

		std::vector<char> generated_labels;
		for (const auto& value : tree.at_rank(3))
		{
			CHECK(value.has_value());
			generated_labels.push_back(value->second);
		}
		CHECK(generated_labels == std::vector<char>{'C', 'A', 'G'});

		std::vector<char> projected_labels;
		for (const auto& value : tree.at_rank(3, [](const auto& item) { return item.second; }))
		{
			CHECK(value.has_value());
			projected_labels.push_back(*value);
		}
		CHECK(projected_labels == std::vector<char>{'C', 'A', 'G'});

		auto rank_zero = tree.at_rank(0);
		CHECK(rank_zero.begin() == rank_zero.end());
		auto missing_rank = tree.at_rank(tree.size() + 1);
		CHECK(missing_rank.begin() == missing_rank.end());
	}
}

TEST_CASE("flat_tree", "[flat_tree][graph]")
{
	using namespace deckard::graph::binary;
	SECTION("empty")
	{
		flat_tree<int> f(16);
		CHECK(f.size() == 0);
		CHECK(f.empty() == true);
		CHECK(f.capacity() == 16);
	}
	SECTION("insert")
	{
		flat_tree<int> f(16);
		CHECK(f.size() == 0);
		CHECK(f.empty() == true);
		CHECK(f.capacity() == 16);

		(void)f.insert(10);

		CHECK(f.size() == 1);
		CHECK(f.empty() == false);
		CHECK(f.capacity() == 16);
	}
}
