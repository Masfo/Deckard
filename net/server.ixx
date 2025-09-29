module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net:server;
import std;

import :address;
import :socket;

namespace deckard::net
{
	template<typename T>
	constexpr bool socket_type = requires(T a) {
		{ a };
	};

	template<typename S>
	class server
	{
	};

} // namespace deckard::net
