module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net;

export import :socket;
export import :client;
export import :server;
export import :address;
export import :ip;

import deckard.types;

import std;

namespace deckard::net
{

	// Clumsy, lag simulator


	export void initialize()
	{
		WSADATA wsadata{};
		WSAStartup(0x0202, &wsadata);
	}

	export void deinitialize() { WSACleanup(); }


} // namespace deckard::net
