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
	}
}
