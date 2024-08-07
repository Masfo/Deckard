module;
// #include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net;

import std;
import deckard.scope_exit;
import deckard.types;
import deckard.debug;
import deckard.win32;

namespace deckard::net
{
	using namespace deckard::system;

	export enum class protocol {
		v4,
		v6,
	};
	consteval void enable_bitmask_operations(protocol);

	enum class Transport
	{
		TCP,
		UDP,
	};

	export struct address
	{
		std::string host;
		u16         port{0};
	};

	//


	export std::optional<std::string> hostname_to_ip(std::string_view hostname, protocol version)
	{

		addrinfo hints{}, *res{};

		scope_exit _(
		  [&]
		  {
			  freeaddrinfo(res);
		  });

		if (version == protocol::v4)
			hints.ai_family = AF_INET;
		else
			hints.ai_family = AF_INET6;

		hints.ai_socktype = SOCK_STREAM; // TCP

		if (getaddrinfo(hostname.data(), nullptr, &hints, &res) != 0)
		{
			dbg::println("Failed to resolve hostname: {}", hostname);
			return {};
		}

		while (res)
		{
			char host[NI_MAXHOST]{0};
			if (getnameinfo(res->ai_addr, res->ai_addrlen, host, NI_MAXHOST, nullptr, 0, 0) != 0)
			{
				dbg::println("Could not resolve host name: {}", WSAGetLastError());
				return {};
			}


			if (res->ai_family == AF_INET6 and version == protocol::v6)
				return host;

			if (res->ai_family == AF_INET and version == protocol::v4)
				return host;

			res = res->ai_next;
		}


		return {};
	}

	//
	class socket
	{
	};

	// ######################
	class NetInitializer
	{
	public:
		NetInitializer()
		{
			WSADATA wsadata{};
			WSAStartup(0x0202, &wsadata);
		}

		~NetInitializer() { WSACleanup(); }
	};

	NetInitializer dummy; // Just to automatically init winsock2

} // namespace deckard::net
