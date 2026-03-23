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
	using namespace std::string_literals;

	static constexpr u8          MAX_IPV4_ADDRESS_STR_LEN = 15;
	static constexpr u8          MAX_IPV6_ADDRESS_STR_LEN = 39;
	constexpr std::array<u8, 12> ipv4_mapped_prefix       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};

	// api.ipify.org
	//
	// https://api64.ipify.org
	// https://api.ipify.org
	// https://api6.ipify.org


	export class ip
	{
	private:
		std::array<u8, 16> address{0};

		std::expected<void, std::string> validate_plain_ipv4(std::string_view v) const
		{
			u8     dot_count     = 0;
			size_t segment_start = 0;

			for (size_t i = 0; i <= v.size(); ++i)
			{
				if (i != v.size() and v[i] != '.')
					continue;

				const auto segment_len = i - segment_start;
				if (segment_len == 0)
					return std::unexpected(std::format("Empty octet in ipv4 address: {}", v));

				if (segment_len > 1 and v[segment_start] == '0')
					return std::unexpected(std::format("Leading zero in ipv4 address: {}", v));

				u16 value = 0;
				for (size_t j = segment_start; j < i; ++j)
				{
					if (not utf8::is_ascii_digit(v[j]))
						return std::unexpected(std::format("Invalid character in ipv4 address: {}", v));

					value *= 10;
					value += as<u16>(v[j] - '0');
					if (value > 255)
						return std::unexpected(std::format("IP octet too large: {}", v));
				}

				if (i != v.size())
					dot_count += 1;

				segment_start = i + 1;
			}

			if (dot_count != 3)
				return std::unexpected(std::format("Malformed ipv4 address: {}", v));

			return {};
		}

		std::expected<void, std::string> read_address(std::string_view input)
		{
			address.fill(0u);
			if (input.empty())
				return std::unexpected("Empty input");


			if (input.find(':') == std::string_view::npos)
			{
				auto check = validate_plain_ipv4(input);
				if (not check)
					return std::unexpected(check.error());

				in_addr addr4{};
				if (::inet_pton(AF_INET, input.data(), &addr4) != 1)
					return std::unexpected(std::format("Invalid ipv4 address: {}", input));

				address[10] = 0xFF;
				address[11] = 0xFF;
				std::memcpy(address.data() + 12, &addr4, 4);
				return {};
			}

			in6_addr addr6{};
			if (::inet_pton(AF_INET6, input.data(), &addr6) != 1)
				return std::unexpected(std::format("Invalid ipv6 address: {}", input));

			std::memcpy(address.data(), &addr6, 16);
			return {};
		}

		int classify_address() const
		{

			if (std::ranges::all_of(address, [](auto b) { return b == 0; }))
				return -1;

			const bool ipv4_mapped = std::ranges::equal(std::span{address}.first<12>(), ipv4_mapped_prefix);

			if (ipv4_mapped)
			{
				const auto last_4_bytes = std::span{address}.last<4>();
				return std::ranges::any_of(last_4_bytes, [](auto b) { return b != 0; }) ? 4 : -1;
			}

			return 6;
		}

	public:
		ip() { address.fill(0); }

		ip(std::string_view input)
		{
			if (const auto v = read_address(input); not v)
			{
				dbg::eprintln("Invalid IP address '{}': {}", input, v.error());
				address.fill(0);
			}
		}

		bool is_ipv4() const { return classify_address() == 4; }

		bool is_ipv6() const { return classify_address() == 6; }

		bool valid() const { return classify_address() > 0; }

		std::string to_string() const
		{
			if (not valid())
					return "<invalid IP>"s;


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
					result += "::";
					i += best_len;
				}
				else
				{
					if (not result.empty() and not result.ends_with(':'))
						result += ':';
					result += std::format("{:x}", groups[i]);
					++i;
				}
			}
			result.shrink_to_fit();
			return result;
		}

		int version() const
		{
			if (is_ipv4())
				return 4;
			if (is_ipv6())
				return 6;

			return -1;
		}

		bool operator==(const ip& other) const { return address == other.address; }
	};

	static_assert(sizeof(ip) == 16,
				  "IP class should be exactly 16 bytes to fit both IPv4 and IPv6 addresses without extra padding");

	export std::expected<std::vector<net::ip>, std::string> resolve_ips(const std::string_view domain) noexcept
	{

		struct addrinfo  hints{};
		struct addrinfo* result = nullptr;

		hints.ai_family   = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags    = 0;
		hints.ai_protocol = 0;

		int status = getaddrinfo(domain.data(), nullptr, &hints, &result);
		if (status != 0)
			return std::unexpected(std::format("no such address: \"{}\" (error: {})", domain, WSAGetLastError()));

		std::vector<net::ip> addresses;

		for (struct addrinfo* p = result; p != nullptr; p = p->ai_next)
		{
			char ip_str[INET6_ADDRSTRLEN] = {0};

			if (p->ai_family == AF_INET)
			{
				// IPv4
				struct sockaddr_in* addr_in = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
				if (inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str)))
				{
					net::ip parsed(ip_str);
					if (parsed.valid())
						addresses.push_back(parsed);
				}
			}
			else if (p->ai_family == AF_INET6)
			{
				// IPv6
				struct sockaddr_in6* addr_in6 = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
				if (inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ip_str, sizeof(ip_str)))
				{
					net::ip parsed(ip_str);
					if (parsed.valid())
						addresses.push_back(parsed);
				}
			}
		}

		freeaddrinfo(result);
		return addresses;
	}

	export struct endpoint
	{
		ip          address{};
		u16         port{};
		std::string hostname;

		endpoint() = default;

		endpoint(const ip& address, u16 port)
			: address(address)
			, port(port)
		{
		}

		endpoint(std::string_view host_str, u16 port)
		{
			this->port = port;

			// Strip URL scheme (e.g. "https://", "http://")
			auto host = host_str;
			if (const auto scheme_end = host.find("://"); scheme_end != std::string_view::npos)
				host = host.substr(scheme_end + 3);

			// Strip path component
			if (const auto slash = host.find('/'); slash != std::string_view::npos)
				host = host.substr(0, slash);

			hostname = host;

			ip parsed(hostname);
			if (parsed.valid())
			{
				this->address = std::move(parsed);
				return;
			}

			// Not an IP — try domain resolution
			if (auto result = resolve_ips(host); result and not result->empty())
			{
				this->address = result->front();
			}
			else
			{
				dbg::eprintln("Cannot resolve host '{}'", host);
				this->address = ip{};
			}
		}

		bool valid() const { return address.valid(); }

		std::string to_string() const
		{
			// if (not hostname.empty())
			//	return std::format("{}:{}", hostname, port);
			if (address.is_ipv6())
				return std::format("[{}]:{}", address.to_string(), port);

			return std::format("{}:{}", address.to_string(), port);
		}

		bool operator==(const endpoint& other) const { return address == other.address and port == other.port; }
	};


} // namespace deckard::net

namespace std
{
	using namespace deckard::net;

	template<>
	struct formatter<ip>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const ip& address, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "{}", address.to_string());
		}
	};

} // namespace std
