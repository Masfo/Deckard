module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net;

export import :socket;
import std;

namespace deckard::net
{

	export void initialize()
	{
		WSADATA wsadata{};
		WSAStartup(0x0202, &wsadata);
	}

	export void deinitialize() { WSACleanup(); }


} // namespace deckard::net
