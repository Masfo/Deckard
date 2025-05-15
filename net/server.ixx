module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net:server;
import std;

import :address;
import :socket;

namespace deckard::net
{

	class server_tcp
	{
	public:
		server_tcp() = default;

		server_tcp(address server_address) 
		{ 
			//
			(server_address); 
		}

		bool open(int max_pending_connections = 10)
		{
			//
			(max_pending_connections);
			return true;
		}

		bool send(std::span<char> data, address client_address)
		{
			//
			(data);
			(client_address);
			return true;
		}

		bool close()
		{
			//
			return true;
		}

		bool is_open() const { return m_open; };

	private:
		address m_addr;
		socket  m_socket;

		std::atomic<bool> m_open;
	};

} // namespace deckard::net
