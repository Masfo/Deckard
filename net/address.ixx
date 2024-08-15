export module deckard.net:address;

import std;
import deckard.types;

namespace deckard::net
{
	export struct address
	{
		enum class version : u8
		{
			v4 = 4,
			v6 = 6
		};


		std::string        hostname;
		std::array<u16, 8> ip{0};
		u16                port{0};
		version            ver{version::v4};
		u8                 flags{0};

		// Recommendation for IPv6 Address Text Representation: https://www.rfc-editor.org/rfc/rfc5952
	};

	static_assert(sizeof(address) == 64, "IpAddress is not 64-bytes");

} // namespace deckard::net
