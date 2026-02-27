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
