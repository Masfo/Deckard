#include <catch2/catch_all.hpp>


import std;
import deckard.json;

TEST_CASE("json", "[json]")
{
	using namespace deckard;
	using namespace Catch;
	using namespace std::string_view_literals;

	SECTION("line and column tracking in errors")
	{
		std::string  json_str = R"({
"key1": "value1",
"key2": tru,
"key3": "value3"
		})";
		json::parser j;
		auto         result = j.parse(utf8::view{json_str});
		CHECK(result.error() == "Error at line 3, column 9: Invalid literal, expected 'true'"sv);
	}


	SECTION("empty value")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{""});
		CHECK_FALSE(result);
		CHECK(result.error() == "Error at line 1, column 1: Unexpected end of input"sv);
	}

	SECTION("empty array")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"[]"});
		CHECK(result);
		CHECK(result->is_array());
		auto arr = result->as_array();
		CHECK(arr.empty());
	}

	SECTION("empty object")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{}"});
		CHECK(result);
		CHECK(result->is_object());
		auto obj = result->as_object();
		CHECK(obj.empty());
	}

	SECTION("find")
	{ 
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"key\": \"value\"}"});
		CHECK(result);
		
		CHECK(result->find("key").value()->is_string());
		CHECK(result->find("key").value()->as_string() == "value"sv);

		CHECK_FALSE(result->find("nonexistent"));

	}
	SECTION("string key and value")
	{
		json::parser j;

		auto result = j.parse(utf8::view{"{\"key\": \"value\"}"});
		CHECK(result);
		CHECK(result->is_object());
		auto obj = result->as_object();
		CHECK(obj.size() == 1);
		CHECK(obj.contains("key"sv));
		CHECK(result->at("key").value()->as_string() == "value"sv);


		auto it = obj.find("key"sv);
		CHECK(it != obj.end());
		CHECK(it->second.is_string());
		CHECK(it->second.as_string() == "value"sv);
	}

	SECTION("string quoted key and string value")
	{
		json::parser j;

		auto result = j.parse(utf8::view{"{\"key\\\"quoted\": \"value\"}"});
		CHECK(result);
		CHECK(result->is_object());
		auto obj = result->as_object();
		CHECK(obj.size() == 1);
		CHECK(obj.contains("key\"quoted"sv));

		auto it = obj.find("key\"quoted"sv);
		CHECK(it != obj.end());
		CHECK(it->second.is_string());
		CHECK(it->second.as_string() == "value"sv);
	}

	SECTION("utf8 keys and values")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"ключ\": \"значение\"}"sv});
		CHECK(result);
		CHECK(result->is_object());
		auto obj = result->as_object();
		CHECK(obj.size() == 1);
		CHECK(obj.contains("ключ"sv));
		auto it = obj.find("ключ"sv);
		CHECK(it != obj.end());
		CHECK(it->second.is_string());
		CHECK(it->second.as_string() == "значение"sv);
	}

	SECTION("numbers")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"int\": -42, \"float\": 3.14, \"s\": 666}"sv});
		REQUIRE(result);
		CHECK(result->is_object());
		CHECK(result->at("int").value()->is_number());
		CHECK(result->at("int").value()->as_number() == Approx(-42.0).margin(0.0001));
		CHECK(result->at("float").value()->is_number());
		CHECK(result->at("float").value()->as_number() == Approx(3.14).margin(0.0001));
		CHECK(result->at("s").value()->is_number());
		CHECK(result->at("s").value()->as_number() == Approx(666.0).margin(0.0001));
	}

	SECTION("invalid number")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"key\": 1.2.3}"sv});
		CHECK_FALSE(result);
		CHECK(result.error() == "Error at line 1, column 14: Invalid number: 1.2.3"sv);

		result = j.parse(utf8::view{"{\"key\": -}"sv});
		CHECK_FALSE(result);
		CHECK(result.error() == "Error at line 1, column 9: Expected digit after '-' in number"sv);
	}

	SECTION("float value")
	{ 
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"pi\": 3.141}"sv});
		REQUIRE(result);
		CHECK(result->is_object());
		CHECK(result->at("pi").value()->is_number());
		CHECK(result->at("pi").value()->as_number() == Approx(3.141).margin(0.0001));
	}

	SECTION("boolean values")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"true\": true, \"false\": false}"sv});
		CHECK(result);
		CHECK(result->is_object());
		auto obj = result->as_object();
		CHECK(obj.size() == 2);
		CHECK(obj.contains("true"sv));
		CHECK(obj.contains("false"sv));
		auto it_true = obj.find("true"sv);
		CHECK(it_true != obj.end());
		CHECK(it_true->second.is_bool());
		CHECK(it_true->second.as_bool() == true);
		auto it_false = obj.find("false"sv);
		CHECK(it_false != obj.end());
		CHECK(it_false->second.is_bool());
		CHECK(it_false->second.as_bool() == false);
	}

	SECTION("invalid boolean value")
	{
		{
			json::parser j;
			auto         result = j.parse(utf8::view{"{\"key\": tru}"sv});
			CHECK_FALSE(result);
			CHECK(result.error() == "Error at line 1, column 9: Invalid literal, expected 'true'"sv);
		}
		{
			json::parser j;
			auto         result = j.parse(utf8::view{"{\"key\": fals}"sv});
			CHECK_FALSE(result);
			CHECK(result.error() == "Error at line 1, column 9: Invalid literal, expected 'false'"sv);
		}
	}

	SECTION("invalid escape characters in string")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"key\": \"value\\x\"}"sv});
		CHECK_FALSE(result);
		CHECK(result.error() == "Error at line 1, column 16: Invalid escape sequence in string"sv);
	}


	SECTION("null value")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"key\": null}"sv});
		CHECK(result);
		CHECK(result->is_object());
		auto obj = result->as_object();
		CHECK(obj.size() == 1);
		CHECK(obj.contains("key"sv));
		auto it = obj.find("key"sv);
		CHECK(it != obj.end());
		CHECK(it->second.is_null());
	}

	SECTION("invalid null value")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"key\": nul}"sv});
		CHECK_FALSE(result);
		CHECK(result.error() == "Error at line 1, column 12: Invalid literal, expected 'null'"sv);
	}

	SECTION("array of numbers")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"[1, 2, 3]"sv});
		CHECK(result);
		CHECK(result->is_array());
		auto arr = result->as_array();
		CHECK(arr.size() == 3);
		CHECK(arr[0].is_number());
		CHECK(arr[0].as_number() == 1.0);
		CHECK(arr[1].is_number());
		CHECK(arr[1].as_number() == 2.0);
		CHECK(arr[2].is_number());
		CHECK(arr[2].as_number() == 3.0);
	}

	SECTION("trailing comma in array")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"[1, 2, 3,]"sv});
		CHECK_FALSE(result);
		CHECK(result.error() == "Error at line 1, column 10: Trailing comma in array is not allowed"sv);
	}

	SECTION("trailing comma in object")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"key\": \"value\",}"sv});
		CHECK_FALSE(result);
		CHECK(result.error() == "Error at line 1, column 17: Trailing comma in object is not allowed"sv);
	}

	SECTION("nested object and array")
	{
		json::parser j;
		auto         result = j.parse(utf8::view{"{\"array\": [1, 2, {\"nested\": true}]}"sv});
		CHECK(result);
		CHECK(result->is_object());
		auto obj = result->as_object();
		CHECK(obj.size() == 1);
		CHECK(obj.contains("array"sv));
		auto it = obj.find("array"sv);
		CHECK(it != obj.end());
		CHECK(it->second.is_array());
		auto arr = it->second.as_array();
		CHECK(arr.size() == 3);
		CHECK(arr[0].is_number());
		CHECK(arr[0].as_number() == 1.0);
		CHECK(arr[1].is_number());
		CHECK(arr[1].as_number() == 2.0);
		CHECK(arr[2].is_object());
		auto nested_obj = arr[2].as_object();
		CHECK(nested_obj.size() == 1);
		CHECK(nested_obj.contains("nested"sv));
		auto nested_it = nested_obj.find("nested"sv);
		CHECK(nested_it != nested_obj.end());
		CHECK(nested_it->second.is_bool());
		CHECK(nested_it->second.as_bool() == true);
	}

	SECTION("at")
	{
		std::string  json_str = R"({"headers": {"dnt": "1"}})";
		json::parser j;
		auto         result = j.parse(utf8::view{json_str});
		CHECK(result);
		CHECK(result->at("headers.dnt").value()->as_string() == "1"sv);

		CHECK(result->at("unknonwn").error() == "key not found: 'unknonwn'"sv);
	}



	SECTION("invalid json")
	{
		json::parser j;
		CHECK(j.parse(utf8::view{"{\"key\": \"value\""sv}).error()
			  == R"(Error at line 1, column 16: Expected ',' or '}' after value in object)");
		CHECK(j.parse(utf8::view{"{\"key\": \"value\",}"sv}).error()
			  == R"(Error at line 1, column 17: Trailing comma in object is not allowed)");
		CHECK(j.parse(utf8::view{"[1, 2, 3"sv}).error()
			  == R"(Error at line 1, column 9: Expected ',' or ']' after value in array)");
	}

	SECTION("nested objects")
	{
		std::string json_str = R"(
{
  "headers": {
    "dnt": "1"
    }
})";

		json::parser j;
		auto         result = j.parse(utf8::view{json_str});
		REQUIRE(result);

		const auto& doc = *result;
		CHECK(doc["headers"].contains("dnt"sv));
		CHECK(doc["headers"]["dnt"].as_string() == "1"sv);
	}
}
