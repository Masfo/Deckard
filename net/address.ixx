export module deckard.net:address;

import std;
import deckard.types;
import deckard.enums;

namespace deckard::net
{
	export enum class flags : u8 {
		ipv4 = BIT(0),
		ipv6 = BIT(1),

		pad2 = BIT(2),
		pad3 = BIT(3),
		pad4 = BIT(4),
		pad5 = BIT(5),
		pad6 = BIT(6),
		pad7 = BIT(7),
	};
	consteval void enable_bitmask_operations(flags);

	export struct alignas(64) address
	{

		std::string ip_to_string() const
		{
			if (flags == flags::ipv4)
			{
				return {};
			}
			else if (flags == flags::ipv6)
			{
				return {};
			}

			return {};
		}

		std::string        hostname;
		std::array<u16, 8> ip{0};
		u16                port{0};
		flags              flags{flags::ipv4};

		// Recommendation for IPv6 Address Text Representation: https://www.rfc-editor.org/rfc/rfc5952
	};

	static_assert(sizeof(address) == 64, "IpAddress is not 64-bytes");

} // namespace deckard::net
