module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net;
import deckard.types;
import deckard.debug;

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



	export std::string wsa_error_string(int error = WSAGetLastError())
	{
		std::string ret(256, '\0');
		const DWORD written = FormatMessageA(
		  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		  nullptr,
		  static_cast<DWORD>(error),
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  ret.data(),
		  static_cast<DWORD>(ret.size()),
		  nullptr);
		if (written == 0)
			return std::format("WSA error {}", error);
		ret.resize(written);
		while (not ret.empty() and (ret.back() == '\n' or ret.back() == '\r' or ret.back() == ' '))
			ret.pop_back();
		return ret;
	}

	export void initialize() noexcept
	{
		WSADATA wsadata{};
		if( auto result = WSAStartup(0x0202, &wsadata); result != 0 or wsadata.wVersion != 0x0202)
		{
			dbg::println("WSAStartup failed: {}", result);
			WSACleanup();
		}
	}

	export void deinitialize() { WSACleanup(); }


} // namespace deckard::net
