export module deckard.net:ip;

import deckard.types;
import deckard.as;
import deckard.assert;
import deckard.debug;
import deckard.utf8;
import std;

namespace deckard::net
{
	export class ip
	{
	private:
		std::array<u8, 16> address;

		void read_address(std::string_view input)
		{
			address.fill(0u);

			static constexpr u8 MAX_IPV6_ADDRESS_STR_LEN = 39;

			// https://www.rfc-editor.org/rfc/rfc4291
			//
			//  0:0:0:0:0:0:13.1.68.3
			//  0:0:0:0:0:FFFF:129.144.52.38
			//
			//	or in compressed form:
			//
			//  ::13.1.68.3
			//  ::FFFF:129.144.52.38
			//
			// 0123 if dot in 1|2|3 then ipv4
			// 1.1.1.1
			// 12.1.1.1
			// 123.1.1.1
			// 192.168.1.9

			u16 accumulator = 0;
			u8  colon_count = 0;
			u8  dot_count   = 0;
			u8  pos         = 0;


			if (input.size() >= 4 and (input[1] == '.' or input[2] == '.' or input[3] == '.'))
			{
				// ipv4 address
				address[10] = address[11] = 0xFF;
				pos         = 12;

				for (u8 i = 0; i < input.size(); i++)
				{
					if (input[i] == '.')
					{
						dot_count += 1;
						assert::check(dot_count <= 3, "Too many dots in ipv4 address");
						// TODO: actual check instead of assert

						address[pos] = as<u8>(accumulator);
						accumulator  = 0;
						pos += 1;
					}
					else
					{
						accumulator *= 10;
						accumulator += as<u16>(utf8::ascii_hex_to_int(input[i]));
						assert::check(accumulator <= 255, "IP octet too large");
						// TODO: return expected

					}
				}
				address[pos] += as<u8>(accumulator);
				return;
			}
			else
			{
				// Assume ipv6 address

				for (u8 i = 1; i < input.size(); i++)
				{
					if (input[i] == ':')
					{
						if (input[i - 1] == ':')
							colon_count = 14;
						else if (colon_count)
							colon_count -= 2;
					}
				}

				for (u8 i = 0; i < input.size() && pos < 16; i++)
				{

					if (input[i] == ':')
					{
						address[pos + 0] = as<u8>(accumulator >> 8);
						address[pos + 1] = as<u8>(accumulator);
						accumulator      = 0;

						if (colon_count && i && input[i - 1] == ':')
							pos = colon_count;
						else
							pos += 2;
					}
					else
					{
						accumulator <<= 4;
						accumulator |= utf8::ascii_hex_to_int(input[i]);
					}
				}

				address[pos + 0] = as<u8>(accumulator >> 8);
				address[pos + 1] = as<u8>(accumulator);
			}
		}


	public:
		ip() { address.fill(0); }

		ip(std::string_view address) { read_address(address); }

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
