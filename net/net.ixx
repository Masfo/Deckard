module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net;

export import :socket;

namespace deckard::net
{

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

	// ######################


} // namespace deckard::net
