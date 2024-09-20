#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.graph;

using namespace deckard;

TEST_CASE("Binary Tree", "[binarytree]")
{
	SECTION("")
	{
		//
		REQUIRE(1 == 1);
	}
}

TEST_CASE("AVL", "[avl]")
{
	SECTION("empty")
	{
		graph::avl<i32> tree;

		REQUIRE(tree.size() == 0);
		REQUIRE(tree.empty() == true);
	}

	SECTION("insert")
	{
		graph::avl<i32> tree;

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

	{
		SECTION("clone")
		{
			graph::avl<i32> tree;

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
			graph::avl<i32> tree;

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

		SECTION("search")
		{
			graph::avl<i32> tree;

			tree.insert(10);
			tree.insert(30);
			tree.insert(20);
			tree.insert(40);
			tree.insert(70);
			tree.insert(60);
			tree.insert(80);

			REQUIRE(tree.size() == 7);
			REQUIRE(true == tree.search(10));
			REQUIRE(true == tree.search(80));
			REQUIRE(false == tree.search(0));
		}

		SECTION("clear")
		{
			graph::avl<i32> tree;

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
			graph::avl<i32> tree;

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


		SECTION("visit inorder")
		{
			graph::avl<i32> tree;

			tree.insert(10);
			tree.insert(30);
			tree.insert(60);
			tree.insert(80);
			std::vector<i32> v;
			auto             to_vector = [&](i32 value) { v.push_back(value); };

			tree.visit(to_vector);


			//        30			0
			//      /   \
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
			graph::avl<i32> tree;

			tree.insert(10);
			tree.insert(30);
			tree.insert(60);
			tree.insert(80);
			std::vector<i32> v;
			auto             to_vector = [&](i32 value) { v.push_back(value); };

			tree.visit(graph::order::pre, to_vector);

			//        30			0
			//      /   \
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
			graph::avl<i32> tree;

			tree.insert(10);
			tree.insert(30);
			tree.insert(60);
			tree.insert(80);
			std::vector<i32> v;
			auto             to_vector = [&](i32 value) { v.push_back(value); };

			tree.visit(graph::order::post, to_vector);


			//        30			0
			//      /   \
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
			graph::avl<i32> tree;

			tree.insert(10);
			tree.insert(30);
			tree.insert(60);
			tree.insert(80);
			std::vector<i32> v;
			auto             to_vector = [&](i32 value) { v.push_back(value); };

			tree.visit(graph::order::level, to_vector);


			//        30			0
			//      /   \
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
}
