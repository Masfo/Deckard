#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.graph;

using namespace deckard;
using namespace deckard::graph;

TEST_CASE("Binary Tree", "[binarytree]")
{
	SECTION("empty")
	{
		binary::tree<i32> tree;

		REQUIRE(tree.size() == 0);
		REQUIRE(tree.empty() == true);
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
		REQUIRE(tree.size() == 7);
		REQUIRE(tree.empty() == false);
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

		REQUIRE(tree.size() == 7);
		REQUIRE(tree2.size() == 7);
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

		REQUIRE(tree.size() == 6);
		REQUIRE(tree2.size() == 7);
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

		REQUIRE(tree.size() == 7);
		REQUIRE(true == tree.has(10));
		REQUIRE(true == tree.has(80));
		REQUIRE(false == tree.has(0));
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
		REQUIRE(tree.size() == 7);
		REQUIRE(tree.empty() == false);

		tree.clear();
		REQUIRE(tree.size() == 0);
		REQUIRE(tree.empty() == true);
	}

	SECTION("get node/height")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		REQUIRE(tree.size() == 4);
		REQUIRE(tree.empty() == false);

		REQUIRE(tree.get(10)->height == 0);
		REQUIRE(tree.get(30)->height == 2);
		REQUIRE(tree.get(60)->height == 1);
		REQUIRE(tree.get(80)->height == 0);

		tree.insert(100);
		REQUIRE(tree.size() == 5);
		REQUIRE(tree.get(10)->height == 0);
		REQUIRE(tree.get(30)->height == 2);
		REQUIRE(tree.get(60)->height == 0);
		REQUIRE(tree.get(80)->height == 1);
		REQUIRE(tree.get(100)->height == 0);

		tree.insert(5);
		REQUIRE(tree.size() == 6);
		REQUIRE(tree.get(5)->height == 0);
		REQUIRE(tree.get(10)->height == 1);
		REQUIRE(tree.get(30)->height == 2);
		REQUIRE(tree.get(60)->height == 0);
		REQUIRE(tree.get(80)->height == 1);
		REQUIRE(tree.get(100)->height == 0);
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
		REQUIRE(n.has_value());
		REQUIRE((*n)->data == 10);

		REQUIRE(not tree.at(0).has_value());
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


		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 10);
		REQUIRE(v[1] == 30);
		REQUIRE(v[2] == 60);
		REQUIRE(v[3] == 80);
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


		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 30);
		REQUIRE(v[1] == 10);
		REQUIRE(v[2] == 60);
		REQUIRE(v[3] == 80);
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

		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 10);
		REQUIRE(v[1] == 80);
		REQUIRE(v[2] == 60);
		REQUIRE(v[3] == 30);
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

		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 30);
		REQUIRE(v[1] == 10);
		REQUIRE(v[2] == 60);
		REQUIRE(v[3] == 80);
	}
}

// AVL

TEST_CASE("AVL", "[avl]")
{
	SECTION("empty")
	{
		avl::tree<i32> tree;

		REQUIRE(tree.size() == 0);
		REQUIRE(tree.empty() == true);
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
		REQUIRE(tree.size() == 7);
		REQUIRE(tree.empty() == false);
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

		REQUIRE(tree.size() == 7);
		REQUIRE(tree2.size() == 7);
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

		REQUIRE(tree.size() == 6);
		REQUIRE(tree2.size() == 7);
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

		REQUIRE(tree.size() == 7);
		REQUIRE(true == tree.has(10));
		REQUIRE(true == tree.has(80));
		REQUIRE(false == tree.has(0));
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
		REQUIRE(tree.size() == 7);
		REQUIRE(tree.empty() == false);

		tree.clear();
		REQUIRE(tree.size() == 0);
		REQUIRE(tree.empty() == true);
	}


	SECTION("get node/height")
	{
		avl::tree<i32> tree;

		tree.insert(10);
		tree.insert(30);
		tree.insert(60);
		tree.insert(80);
		REQUIRE(tree.size() == 4);
		REQUIRE(tree.empty() == false);

		REQUIRE(tree.get(10)->height == 0);
		REQUIRE(tree.get(30)->height == 2);
		REQUIRE(tree.get(60)->height == 1);
		REQUIRE(tree.get(80)->height == 0);

		tree.insert(100);
		REQUIRE(tree.size() == 5);
		REQUIRE(tree.get(10)->height == 0);
		REQUIRE(tree.get(30)->height == 2);
		REQUIRE(tree.get(60)->height == 0);
		REQUIRE(tree.get(80)->height == 1);
		REQUIRE(tree.get(100)->height == 0);

		tree.insert(5);
		REQUIRE(tree.size() == 6);
		REQUIRE(tree.get(5)->height == 0);
		REQUIRE(tree.get(10)->height == 1);
		REQUIRE(tree.get(30)->height == 2);
		REQUIRE(tree.get(60)->height == 0);
		REQUIRE(tree.get(80)->height == 1);
		REQUIRE(tree.get(100)->height == 0);
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
		REQUIRE(n.has_value());
		REQUIRE((*n)->data == 10);

		REQUIRE(not tree.at(0).has_value());
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


		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 10);
		REQUIRE(v[1] == 30);
		REQUIRE(v[2] == 60);
		REQUIRE(v[3] == 80);
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


		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 30);
		REQUIRE(v[1] == 10);
		REQUIRE(v[2] == 60);
		REQUIRE(v[3] == 80);
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

		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 10);
		REQUIRE(v[1] == 80);
		REQUIRE(v[2] == 60);
		REQUIRE(v[3] == 30);
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

		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == 30);
		REQUIRE(v[1] == 10);
		REQUIRE(v[2] == 60);
		REQUIRE(v[3] == 80);
	}
}
