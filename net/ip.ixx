module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net:ip;

import deckard.types;
import deckard.as;
import deckard.assert;
import deckard.debug;
import deckard.utf8;
import std;

namespace deckard::net
{
	static constexpr u8 MAX_IPV4_ADDRESS_STR_LEN = 15;
	static constexpr u8 MAX_IPV6_ADDRESS_STR_LEN = 39;

	export enum class IPVersion : u32
	{
		IPV4 = 4,
		IPV6 = 6,
	};



	export struct IPAddressResult
	{
		std::string ip;
		IPVersion   version{IPVersion::IPV4};
	};

	export std::expected<std::vector<IPAddressResult>, std::string> get_ip_addresses(const std::string_view domain) noexcept
	{
		struct addrinfo  hints{};
		struct addrinfo* result = nullptr;

		hints.ai_family   = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags    = 0;
		hints.ai_protocol = 0;

		int status = getaddrinfo(domain.data(), nullptr, &hints, &result);
		if (status != 0)
			return std::unexpected(std::format("getaddrinfo: {} ({})", domain, WSAGetLastError()));

		std::vector<IPAddressResult> addresses;

		for (struct addrinfo* p = result; p != nullptr; p = p->ai_next)
		{
			char ip_str[INET6_ADDRSTRLEN] = {0};

			if (p->ai_family == AF_INET)
			{
				// IPv4
				struct sockaddr_in* addr_in = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
				if (inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str)))
				{
					addresses.push_back({std::string(ip_str), IPVersion::IPV4});
				}
			}
			else if (p->ai_family == AF_INET6)
			{
				// IPv6
				struct sockaddr_in6* addr_in6 = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
				if (inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ip_str, sizeof(ip_str)))
				{
					addresses.push_back({std::string(ip_str), IPVersion::IPV6});
				}
			}
		}

		freeaddrinfo(result);
		return addresses;
	}



	// api.ipify.org
	// 
	// https://api64.ipify.org
	// https://api.ipify.org
	// https://api6.ipify.org

	export class ip
	{
	private:
		std::array<u8, 16> address;

		void read_address(std::string_view input)
		{
			address.fill(0u);
			if (input.empty())
				return;

			// https://www.rfc-editor.org/rfc/rfc4291
			// https://www.rfc-editor.org/rfc/rfc8200
			// https://www.rfc-editor.org/rfc/rfc5952
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
				// assume ipv4 address
				address[10] = address[11] = 0xFF;
				pos                       = 12;

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
						address[pos + 0] = as<u8>((accumulator >> 8) & 0xFF);
						address[pos + 1] = as<u8>((accumulator >> 0) & 0xFF);
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

				address[pos + 0] = as<u8>((accumulator >> 8) & 0xFF);
				address[pos + 1] = as<u8>((accumulator >> 0) & 0xFF);
			}
		}


	public:
		ip() { address.fill(0); }

		ip(std::string_view input) { read_address(input); }

		bool is_ipv4() const
		{
			for (size_t i = 0; i < 10; ++i)
			{
				if (address[i] != 0)
					return false;
			}
			return address[10] == 0xFF && address[11] == 0xFF;
		}

		bool is_ipv6() const { return not is_ipv4(); }

		std::string to_string() const
		{
			if (is_ipv4())
				return std::format("{}.{}.{}.{}", address[12], address[13], address[14], address[15]);


			std::array<u16, 8> groups;
			for (int i = 0; i < 8; ++i)
				groups[i] = as<u16>((address[2 * i] << 8) | address[2 * i + 1]);


			int best_start = -1, best_len = 0;
			for (int i = 0; i < groups.size();)
			{
				if (groups[i] == 0)
				{
					int j = i;
					while (j < groups.size() && groups[j] == 0)
						++j;
					int len = j - i;
					if (len > best_len)
					{
						best_start = i;
						best_len   = len;
					}
					i = j;
				}
				else
				{
					++i;
				}
			}

			if (best_len < 2)
				best_start = -1;

			std::string result;
			result.reserve(MAX_IPV6_ADDRESS_STR_LEN);

			for (int i = 0; i < 8;)
			{
				if (i == best_start)
				{
					result += (i == 0) ? ":" : ":";
					i += best_len;

					if (i >= 8)
						break;
				}
				else
				{
					if (i > 0)
						result += ":";
					result += std::format("{:x}", groups[i]);

					++i;
				}
			}
			result.shrink_to_fit();
			return result;
		}
	};

} // namespace deckard::net

namespace std
{
	using namespace deckard::net;

	template<>
	struct formatter<ip>
	{
		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto pos = ctx.begin();
			//	while (pos != ctx.end() && *pos != '}')
			//	{
			//		uppercase_hex = *pos == 'X';
			//		++pos;
			//	}
			return pos;
		}

		auto format(const ip& address, std::format_context& ctx) const { return std::format_to(ctx.out(), "{}", address.to_string()); }

		// bool uppercase_hex{false};
	};

} // namespace std
