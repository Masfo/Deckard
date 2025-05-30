export module deckard.net:client;

import deckard.types;
import std;

namespace deckard::net
{
	export class ip
	{
	private:
		std::array<u8, 16> address;

		bool is_ipv4(std::string_view address) const
		{
			return address[
		}

		void read_address(std::string_view address_str)
		{
			// Parse the address string and fill the address array
			// This is a placeholder for actual parsing logic
			// You would typically split the string by '.' or ':' and convert each part to an integer
		}

	public:
		ip() { address.fill(0); }

		ip(std::string_view address)
		{
		}

		bool is_ipv4() const
		{
			// IPv4 Address : 192.0.2.1 16 - Byte Representation
			//   0  1  2  3  4  5  6  7  8  9 10 11 
			// [00 00 00 00 00 00 00 00 00 00 FF FF C0 00 02 01]

			for (size_t i = 0; i < 10; ++i)
			{
				if (address[i] != 0)
					return false;
			}
			// Check if bytes 10 and 11 are 0xFF
			return address[10] == 0xFF && address[11] == 0xFF;	
		}

		bool is_ipv6() const { return not is_ipv4(); }
	};

} // namespace deckard::net
