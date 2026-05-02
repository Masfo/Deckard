#include <catch2/catch_test_macros.hpp>


import std;
import deckard.lru;

using namespace deckard;

TEST_CASE("lru", "[lru]")
{
	SECTION("ctor")
	{
		lru_cache<int, std::string> cache(3);

		cache.put(1, "one");
		cache.put(2, "two");
		cache.put(3, "three");

		CHECK(cache.size() == 3);
		CHECK(cache.get(1) == "one");
		CHECK(cache.get(2) == "two");
		CHECK(cache.get(3) == "three");

		cache.put(4, "four");
		CHECK(cache.size() == 3);
		CHECK(cache.get(1) == std::nullopt);
	}

	SECTION("empty cache")
	{
		lru_cache<int, std::string> cache(3);

		CHECK(cache.size() == 0);
		CHECK(cache.get(1) == std::nullopt);
		CHECK(cache.exists(1) == false);
		CHECK(cache.exists(999) == false);

		std::vector<int> keys;
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys.empty());
	}

	SECTION("single item cache (max_size = 1)")
	{
		lru_cache<int, std::string> cache(1);

		cache.put(1, "one");
		CHECK(cache.size() == 1);
		CHECK(cache.get(1) == "one");
		CHECK(cache.exists(1) == true);

		cache.put(2, "two");
		CHECK(cache.size() == 1);
		CHECK(cache.get(1) == std::nullopt);
		CHECK(cache.get(2) == "two");
		CHECK(cache.exists(2) == true);
		CHECK(cache.exists(1) == false);

		cache.put(3, "three");
		CHECK(cache.size() == 1);
		CHECK(cache.get(2) == std::nullopt);
		CHECK(cache.get(3) == "three");
	}

	SECTION("repeated access keeps item at front")
	{
		lru_cache<int, std::string> cache(3);

		cache.put(1, "one");
		cache.put(2, "two");
		cache.put(3, "three");

		std::vector<int> keys;
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{3, 2, 1});

		cache.get(1);

		keys.clear();
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{1, 3, 2});
	}

	SECTION("iterate")
	{
		lru_cache<int, std::string> cache(3);

		cache.put(1, "one");
		cache.put(2, "two");
		cache.put(3, "three");

		std::vector<int> keys;
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{3, 2, 1});

		cache.get(1);

		keys.clear();
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{1, 3, 2});

		cache.put(4, "four");

		keys.clear();
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{4, 1, 3});


		std::vector<std::pair<int, std::string>> items;
		for(const auto& [k, v] : cache)
			items.emplace_back(k, v);

		CHECK(items == std::vector<std::pair<int, std::string>>{{4, "four"}, {1, "one"}, {3, "three"}});
	}

	SECTION("update existing key")
	{
		lru_cache<int, std::string> cache(2);
		cache.put(1, "one");
		cache.put(2, "two");
		CHECK(cache.get(1) == "one");
		CHECK(cache.get(2) == "two");

		cache.put(1, "ONE");
		CHECK(cache.get(1) == "ONE");
		CHECK(cache.get(2) == "two");
		CHECK(cache.get(3) == std::nullopt);
	}

	SECTION("exists")
	{ 
		lru_cache<int, std::string> cache(2);
		cache.put(1, "one");
		cache.put(2, "two");

		CHECK(cache.exists(1) == true);
		CHECK(cache.exists(2) == true);
		CHECK(cache.exists(3) == false);

		cache.put(3, "three");

		CHECK(cache.exists(1) == false);
		CHECK(cache.exists(2) == true);
		CHECK(cache.exists(3) == true);
	}

	SECTION("rapid evictions")
	{
		lru_cache<int, int> cache(3);

		cache.put(1, 10);
		cache.put(2, 20);
		cache.put(3, 30);

		std::vector<int> keys;
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{3, 2, 1});

		cache.put(4, 40);
		cache.put(5, 50);
		cache.put(6, 60);

		keys.clear();
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{6, 5, 4});
		CHECK(cache.exists(1) == false);
		CHECK(cache.exists(2) == false);
		CHECK(cache.exists(3) == false);
	}

	SECTION("interleaved access pattern")
	{
		lru_cache<int, std::string> cache(4);

		cache.put(1, "one");
		cache.put(2, "two");
		cache.put(3, "three");
		cache.put(4, "four");

		cache.get(2);
		cache.get(4);
		cache.get(1);

		std::vector<int> keys;
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{1, 4, 2, 3});

		cache.put(5, "five");

		keys.clear();
		for (const auto& [k, v] : cache)
			keys.push_back(k);
		CHECK(keys == std::vector<int>{5, 1, 4, 2});
		CHECK(cache.exists(3) == false);
	}

	SECTION("different value types (int)")
	{
		lru_cache<std::string, int> cache(3);

		cache.put("a", 100);
		cache.put("b", 200);
		cache.put("c", 300);

		CHECK(cache.get("a") == 100);
		CHECK(cache.get("b") == 200);
		CHECK(cache.get("c") == 300);

		cache.put("d", 400);
		CHECK(cache.get("a") == std::nullopt);
		CHECK(cache.get("d") == 400);
	}

	SECTION("format")
	{
		//
		lru_cache<int, std::string> cache(2);
		cache.put(1, "one");
		cache.put(2, "two");

		CHECK(std::format("{}", cache) == "{2: two, 1: one}");

		cache.get(1);

		CHECK(std::format("{}", cache) == "{1: one, 2: two}");


	}

}
