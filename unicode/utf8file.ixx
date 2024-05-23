export module deckard.utf8:file;
import :codepoints;

import deckard.file;

import std;

namespace fs = std::filesystem;

namespace deckard::utf8
{

	export class utf8file
	{
		struct iterator
		{
			using iterator_category = std::forward_iterator_tag;
			using value_type        = char32_t;

			iterator(codepoints* ptr, i32 i)
				: p(ptr)
				, index(i)
			{
				if (i == 0 and p and p->has_data())
					current = p->next();
			}

			const value_type operator*() const { return current; }

			// Prefix increment
			iterator& operator++()
			{
				if (index >= 0 and p and p->has_next())
				{
					current = p->next();
					index += 1;
					return *this;
				}
				index = -1;
				return *this;
			}

			// Postfix increment
			iterator operator++(int)
			{
				if (index >= 0 and p and p->has_next())
				{
					current = p->next();
					index += 1;
					return *this;
				}
				index = -1;
				return *this;
			}

			friend bool operator==(const iterator& a, const iterator& b) { return a.index == b.index; };

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

		iterator begin() { return iterator(&points, 0); }

		iterator end() { return iterator(&points, -1); }

	private:
		file       file;
		codepoints points;
	};

} // namespace deckard::utf8
