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

		scope_exit _([&] { freeaddrinfo(res); });

		if (version == protocol::v4)
			hints.ai_family = AF_INET;
		else
			hints.ai_family = AF_INET6;

		hints.ai_socktype = SOCK_STREAM; // TCP
		hints.ai_flags    = AI_PASSIVE;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(hostname.data(), nullptr, &hints, &res) != 0)
		{
			dbg::println("Failed to resolve hostname: {}", hostname);
			return {};
		}


		for (addrinfo* ptr = res; ptr != nullptr; ptr = ptr->ai_next)
		{
			char       buffer[46];
			DWORD      buffer_len  = 46;
			LPSOCKADDR sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
			auto       ret         = WSAAddressToStringA(sockaddr_ip, (DWORD)ptr->ai_addrlen, 0, buffer, &buffer_len);

			if (ret == 0 and ptr->ai_family == AF_INET and version == protocol::v4)
				return buffer;

			if (ret == 0 and ptr->ai_family == AF_INET6 and version == protocol::v6)
				return buffer;
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
	} dummy; // Just to automatically init winsock2

} // namespace deckard::net
