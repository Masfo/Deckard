module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net;

export import :socket;
export import :server;
export import :client;
export import :ip;

import deckard.types;

import std;

// Retransmission timer: https://www.rfc-editor.org/rfc/rfc2988
// For example, assuming an RTO of 500 ms, requests would be sent 
// at times 0 ms, 500 ms, 1500 ms, 3500 ms, 7500 ms, 15500 ms, 
// and 31500 ms.  If the client has not received a response after 
// 39500 ms, the client will consider the transaction to have timed 
// out.
// 
// STUN: https://www.rfc-editor.org/rfc/rfc5389
//       https://www.rfc-editor.org/rfc/rfc5780.txt


namespace deckard::net
{

	// Clumsy, lag simulator


	export void initialize()
	{
		WSADATA wsadata{};
		WSAStartup(0x0202, &wsadata);
	}

	export void deinitialize() { WSACleanup(); }


	export std::string hostname()
	{
		std::array<char, 256> hostname { 0};
		if (gethostname(hostname.data(), as<i32>(hostname.size())) == SOCKET_ERROR)
			return "<unknown>";
		return std::string{hostname.data()};
	}


} // namespace deckard::net
