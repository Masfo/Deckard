export module deckard.net:address;

import std;
import :types;
import deckard.types;
import deckard.enums;

namespace deckard::net
{

	export struct alignas(64) address
	{

		std::string ip_to_string() const
		{
			if (proto == protocol::v4)
			{
				return {};
			}
			else if (proto == protocol::v6)
			{
				return {};
			}

			return {};
		}

		std::string        hostname;
		std::array<u16, 8> ip{0};
		u16                port{0};
		protocol           proto{protocol::v4};
		transport          transport{transport::tcp};

		// Recommendation for IPv6 Address Text Representation: https://www.rfc-editor.org/rfc/rfc5952
	};

	static_assert(sizeof(address) == 64, "IpAddress is not 64-bytes");

} // namespace deckard::net
