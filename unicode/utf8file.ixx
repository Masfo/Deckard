export module deckard.utf8:file;
import :codepoints;

import deckard.file;

import std;

namespace fs = std::filesystem;

namespace deckard::utf8
{

	export class utf8file
	{
		struct Iterator
		{
			using iterator_category = std::forward_iterator_tag;
			using value_type        = char32_t;

			Iterator(codepoints* ptr, i32 i)
				: p(ptr)
				, index(i)
			{
				if (i == 0 and p->has_data())
					current = p->next();
			}

			value_type operator*() const { return current; }

			// Prefix increment
			Iterator& operator++()
			{
				if (p->has_next())
				{
					current = p->next();
					index += 1;
					return *this;
				}
				index = -1;
				return *this;
			}

			// Postfix increment
			Iterator operator++(int)
			{
				if (p->has_next())
				{
					current = p->next();
					index += 1;
					return *this;
				}
				index = -1;
				return *this;
			}

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.index == b.index; };

			codepoints* p{nullptr};
			value_type  current{REPLACEMENT_CHARACTER};
			i32         index{0};
		};

	public:
		utf8file(fs::path filename)
			: file(filename)
		{
			points = file.data();
		}

		Iterator begin() { return Iterator(&points, 0); }

		Iterator end() { return Iterator(&points, -1); }

	private:
		file       file;
		codepoints points;
	};

} // namespace deckard::utf8
