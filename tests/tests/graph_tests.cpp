#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.graph;

using namespace deckard;
using namespace deckard::graph;

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


		gr.connect("A", "B", 1);
		// D-B shortest path: D-A-B
		path = gr.shortest_path("D", "B");

		CHECK(path.size() == 2);
		CHECK(path[0] == weighted_edge<std::string>{"D", "A", 3});
		CHECK(path[1] == weighted_edge<std::string>{"A", "B", 1});
	}


	SECTION("all pairs")
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
}

TEST_CASE("Binary Tree", "[binarytree]")
{
	SECTION("empty")
	{
		binary::tree<i32> tree;

		CHECK(tree.size() == 0);
		CHECK(tree.empty() == true);
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
		const auto       to_vector = [&](i32 value) { v.push_back(value); };

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

		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2


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


		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2

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


		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2

		CHECK(v.size() == 4);
		CHECK(v[0] == 30);
		CHECK(v[1] == 10);
		CHECK(v[2] == 60);
		CHECK(v[3] == 80);
	}
}

// AVL

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

		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2


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


		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2

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


		//        30			0
		//      /    \
		//    10      60		1
		// 	 /  \	 /  \
		//  .    .  .    80		2

		CHECK(v.size() == 4);
		CHECK(v[0] == 30);
		CHECK(v[1] == 10);
		CHECK(v[2] == 60);
		CHECK(v[3] == 80);
	}
}
