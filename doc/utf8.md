## Utf-8 string:

UTF8 string implementation with 32 bytes of inline storage (SSO).



```cpp
import deckard;
using namespace deckard


static_assert(sizeof(utf8::string) == 32);

utf8::string str("hello 🌍");


assert::check(str.size() == 7);
assert::check(str.capacity() == 31);
assert::check(str.length() == 7);
assert::check(str.size_in_bytes() == 10);
assert::check(str.valid() == true);



// concat
utf8::string another = str + " world!";
assert::check(another == "hello 🌍 world!"_utf8);

// insert
utf8::string insert = "hello 🌍 world!";
insert.insert(5, " beautiful"_utf8);
assert::check(insert == "hello 🌍 beautiful world!"_utf8);

// substring
utf8::string sub = insert.substr(0, 7);
assert::check(sub == "hello 🌍"_utf8);

// erase
insert.erase(5, 10); // remove " beautiful"

// find
assert::check(insert.find("beautiful"_utf8) == 8);

// contains
assert::check(insert.contains("beautiful"_utf8) == true);

// iterators
for (auto it = insert.begin(); it != insert.end(); ++it) {
	// do something with *it
}

// index operator
assert::check(insert[0] == 'h');

// hash
std::hash<utf8::string> hasher{}(str);



```