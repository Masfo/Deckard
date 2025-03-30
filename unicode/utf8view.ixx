export module deckard.utf8:view;

import: codepoints;
import :decode;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;


namespace deckard::utf8
{
	// TODO: non-owning view to utf8 data in memory (span)

	class view
	{
	private:
		std::span<u8> m_data;
	public:

		view(std::span<u8> data)
			: m_data(data)
		{
		}

		view(std::string_view data)
			: m_data(as<u8*>(data.data()), as<u32>(data.size()))
		{
		}

		view(const char* data, u32 len)
			: m_data(as<u8*>(data), len)
		{
		}

	};

}
