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
		CHECK(g.has_edge("A", "B") == true);
		CHECK(g.has_edge("B", "C") == true);
		g.remove_node("B");
		CHECK(g.size() == 2);
		CHECK(g.has_edge("A", "B") == false);
		CHECK(g.has_edge("B", "C") == false);
	}

	SECTION("disconnect edge")
	{
		undirected<std::string> g;
		g.add("A");
		g.add("B");
		g.connect("A", "B");
		CHECK(g.size() == 2);
		CHECK(g.has_edge("A", "B") == true);

		g.disconnect("A", "B");

		CHECK(g.size() == 2);
		CHECK(g.has_edge("A", "B") == false);
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
		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "C") == true);
		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "B") == true);
		CHECK(gr.has_edge("A", "B") == true);

		auto path = gr.shortest_path("D", "B");
		CHECK(path.size() == 4);

		CHECK(path[0].from == "D");
		CHECK(path[0].to == "A");
		CHECK(path[0].weight == 3);

		CHECK(path[1].from == "A");
		CHECK(path[1].to == "E");
		CHECK(path[1].weight == 1);

		CHECK(path[2].from == "E");
		CHECK(path[2].to == "F");
		CHECK(path[2].weight == 1);

		CHECK(path[3].from == "F");
		CHECK(path[3].to == "B");
		CHECK(path[3].weight == 1);

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
