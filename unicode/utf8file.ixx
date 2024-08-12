export module deckard.utf8:file;
import :codepoints;

import deckard.file;

import std;

namespace fs = std::filesystem;

namespace deckard::utf8
{

	export class utf8file
	{
	private:
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

			const iterator& operator++()
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
		utf8file() = default;

		utf8file(fs::path filename)
			: m_file(filename)
		{
			open(filename);
		}

		void open(fs::path f)
		{
			m_file.open(f);
			points = m_file.data();
		}

		iterator begin() { return iterator(&points, 0); }

		iterator end() { return iterator(&points, -1); }

		std::span<u8> data() const
		{
			if (m_file.data())
				return m_file.data().value();
			return {};
		}

		bool is_open() const { return m_file.is_open(); }

		fs::path name() const { return m_file.name(); }

		codepoints codepoint() const { return points; };

	private:
		file       m_file;
		codepoints points;
	};

	export utf8file operator""_utf8file(const char* filename, size_t) noexcept { return utf8file{filename}; }
} // namespace deckard::utf8
