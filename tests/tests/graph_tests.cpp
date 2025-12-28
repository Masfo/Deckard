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
