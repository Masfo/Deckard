#include <Windows.h>
#include <intrin.h>
#include <pdh.h>
#include <winsock2.h>

#include <iphlpapi.h>
#include <ws2tcpip.h>


import std;
import deckard;

import deckard.helpers;
import deckard.net;
import deckard.file;
using namespace deckard;

using namespace std::chrono_literals;
namespace fs = std::filesystem;

/* Deckard Net
 *
 *		- TCP/UDP socket abstraction with policy-based design
 *		- Commands for sending/receiving data, opening/closing connections
 *		- Handshakes and connection management
 *		- Handle clients and realtime send/receive data
 *		- Based on UDP with TCP-like reliability features (retries, acknowledgments, sequencing)
 *		- STUN for clients
 *		- Synch with NTP
 *		- cloudflare workers for db, and coordination
 *
 *
 *
 *
 *
 *
 */


struct DeckardNetHeader
{
	u8  version;   // Protocol version
	u8  flags;     // Flags for future use (e.g., reliability, compression)
	u16 sequence;  // Sequence number for ordering
	u32 timestamp; // Timestamp for latency measurement
};


enum class NetCommand : u8
{
	Heartbeat = 0x00,

	OpenConnection  = 0x01,
	CloseConnection = 0x02,
	SendData        = 0x03,
	ReceiveData     = 0x04,
	Handshake       = 0x05,

	ServerHello = 0x10,
	ClientHello = 0x11,


	chat_message = 0x30,
	chat_join    = 0x31,
	chat_leave   = 0x32,


	Ping = 0xFE,
	Pong = 0xFF,
};


using namespace std::string_view_literals;

template<typename T>
concept TransportConcept = requires(T t, std::string_view data, u64 n) {
	{ t.open(u16{}) } -> std::same_as<bool>;
	{ t.close() } -> std::same_as<void>;
	{ t.send(data) } -> std::same_as<u64>;
	{ t.receive(n) } -> std::convertible_to<std::string>;
	//{ t.is_connected() } -> std::same_as<bool>;
	{ t.protocol() } -> std::convertible_to<std::string_view>;
};

struct AsyncService
{
};

struct NTPService : AsyncService
{
	std::chrono::system_clock::time_point query_time{};

	std::chrono::milliseconds roundtrip_delay{};
	std::chrono::milliseconds local_clock_offset{};

	u32 unix_epoch{};
	u32 local_epoch{};
};

enum class Address : u32
{
	V4 = AF_INET,
	V6 = AF_INET6,
};

// Bitfield ACK - index and bit represent ACK for every packet after sequence number - 32 packets can be ACKed with each ACK
// bitfield// u64
//	- u32 for sequence number
// - u32 for ACK bitfield (32 bits for ACKs of the last

// 100 then 0b11111...1111101 ACKS packets 100-132, but not 101)

struct BitfieldACK
{
	u64 sequence_and_bitfield{0};

	std::generator<u32> acked_packets() const
	{
		u32 seq_num      = static_cast<u32>(sequence_and_bitfield >> 32);
		u32 ack_bitfield = static_cast<u32>(sequence_and_bitfield & 0xFFFF'FFFF);
		for (u32 i = 0; i < 32; ++i)
		{
			if (ack_bitfield & (1 << i))
				co_yield seq_num - i;
		}
	}

	void set_ack(u32 num)
	{
		u32 seq_num      = static_cast<u32>(sequence_and_bitfield >> 32);
		u32 ack_bitfield = static_cast<u32>(sequence_and_bitfield & 0xFFFF'FFFF);
		if (num > seq_num || num <= seq_num - 32)
			return;

		u32 bit_index = seq_num - num;
		ack_bitfield |= (1 << bit_index);
		sequence_and_bitfield = (static_cast<u64>(seq_num) << 32) | ack_bitfield;
	}
};

static_assert(sizeof(BitfieldACK) == 8, "BitfieldACK should be exactly 8 bytes to fit into a single u64");

struct TCP
{
	bool open(u16 port)
	{
		info("TCP open port {}", port);
		return true;
	}

	void close() { info("TCP close"); }

	u64 send(std::string_view input)
	{
		info("TCP::send = '{}'", input);
		return input.size();
	}

	std::string receive(u64 n)
	{
		info("TCP::receive {} bytes", n);
		return "TCP receive";
	}

	std::string_view protocol() const { return "TCP"; }
};

struct UDP
{
	bool open([[maybe_unused]] u16 port, Address inet = Address::V6)
	{
		u32 type = (inet == Address::V6) ? AF_INET6 : AF_INET;

		socket = ::socket(type, SOCK_DGRAM, IPPROTO_UDP);
		if (socket == INVALID_SOCKET)
		{
			dbg::println("Failed to create UDP socket: {}", WSAGetLastError());
			return false;
		}
		return true;
	}

	void close() { ::closesocket(socket); }

	u64 send(std::string_view input)
	{
		info("UDP::send = '{}'", input);
		return input.size();
	}

	std::string receive(u64 n)
	{
		info("UDP::receive {} bytes", n);
		return "UDP receive";
	}

	std::string_view protocol() const { return "UDP"; }

	SOCKET socket{INVALID_SOCKET};
};

template<TransportConcept TransportPolicy>
struct Socket : TransportPolicy
{

	bool open(u16 port) { return TransportPolicy::open(port); }

	void close() { TransportPolicy::close(); }

	u64 send(std::string_view input) { return TransportPolicy::send(input); }

	std::string receive(u64 n)
	{
		dbg::println("Policy: {}", TransportPolicy::receive(n));

		return "TCP recv";
	}

	std::string_view protocol() const { return TransportPolicy::protocol(); }

	Socket()  = default;
	~Socket() = default;

	Socket(const Socket&)            = delete;
	Socket& operator=(const Socket&) = delete;
};

using UDPSocket = Socket<UDP>;
using TCPSocket = Socket<TCP>;

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))


constexpr size_t MAX_DNS_BUFFER_SIZE = 512;
constexpr size_t MAX_DNS_RECV_SIZE   = 4096;
constexpr size_t DNS_HEADER_SIZE     = 12;

// DNS header structure
struct DNSHeader
{
	u16 id;         // Identification
	u8  rd : 1;     // Recursion Desired
	u8  tc : 1;     // Truncated
	u8  aa : 1;     // Authoritative Answer
	u8  opcode : 4; // Opcode
	u8  qr : 1;     // Query/Response Flag
	u8  rcode : 4;  // Response Code
	u8  cd : 1;     // Checking Disabled
	u8  ad : 1;     // Authenticated Data
	u8  z : 1;      // Reserved for future use
	u8  ra : 1;     // Recursion Available
	u16 qcount;     // Number of Questions
	u16 ancount;    // Number of Answers
	u16 nscount;    // Number of Authority Resource Records
	u16 arcount;    // Number of Additional Resource Records
};

// DNS question structure
struct DNSQuestion
{
	u16 qtype;  // Query Type
	u16 qclass; // Query Class
};

// DNS answer structure
struct DNSAnswer
{
	std::string     name;     // Name
	u16             type;     // Type
	u16             class_;   // Class
	u32             ttl;      // Time to Live
	u16             data_len; // Data Length
	std::vector<u8> data;     // Data
};

class DNSQuery
{
public:
	explicit DNSQuery(const std::string& domain)
		: domain(domain)
	{
		set_dns_server("1.1.1.1", 4);
		set_dns_server("2606:4700:4700::1111", 6);
	}

	std::expected<void, std::string> send_query()
	{
		SOCKET sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (sockfd == INVALID_SOCKET)
			return std::unexpected(std::format("Socket creation failed: {}", net::wsa_error_string()));

		DWORD timeout_ms = 2000;
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof timeout_ms) ==
			SOCKET_ERROR)
			dbg::println("setsockopt failed: {}", net::wsa_error_string());

		sockaddr_in6 dest{};
		dest.sin6_family = AF_INET6;
		dest.sin6_port   = htons(53);

		if (inet_pton(AF_INET6, dns_server_ipv6.c_str(), &dest.sin6_addr) != 1)
		{
			in_addr ipv4{};
			if (inet_pton(AF_INET, dns_server_ipv4.c_str(), &ipv4) != 1)
			{
				closesocket(sockfd);
				return std::unexpected("Invalid DNS server address");
			}

			dest.sin6_addr = {};
			auto* bytes    = reinterpret_cast<u8*>(&dest.sin6_addr);
			bytes[10]      = 0xFF;
			bytes[11]      = 0xFF;
			std::memcpy(bytes + 12, &ipv4, sizeof(ipv4));
		}

		for (const u16 record_type : {u16{1}, u16{28}})
		{
			if (auto r = build_query(record_type); not r)
			{
				closesocket(sockfd);
				return r;
			}

			const int send_len = sendto(
			  sockfd,
			  reinterpret_cast<const char*>(query_buffer.data()),
			  static_cast<int>(query_length),
			  0,
			  reinterpret_cast<struct sockaddr*>(&dest),
			  sizeof(dest));
			if (send_len == SOCKET_ERROR)
			{
				closesocket(sockfd);
				return std::unexpected(std::format("Send failed: {}", net::wsa_error_string()));
			}

			sockaddr_in6 src{};
			socklen_t    len = sizeof(src);
			buffer.resize(MAX_DNS_RECV_SIZE);
			const int recv_len = recvfrom(
			  sockfd,
			  reinterpret_cast<char*>(buffer.data()),
			  static_cast<int>(buffer.size()),
			  0,
			  reinterpret_cast<struct sockaddr*>(&src),
			  &len);

			if (recv_len == SOCKET_ERROR)
			{
				const auto err = net::wsa_error_string();
				closesocket(sockfd);
				return std::unexpected(std::format("Receive failed: {}", err));
			}
			buffer.resize(recv_len);

			if (auto r = parse_response(recv_len); not r)
			{
				closesocket(sockfd);
				if (r.error() == "TC")
				{
					dbg::println("Response truncated, retrying over TCP...");
					return send_query_tcp(record_type, dest);
				}
				return r;
			}
		}

		closesocket(sockfd);
		return {};
	}

	std::expected<void, std::string> send_query_tcp(u16 record_type, const sockaddr_in6& dest)
	{
		SOCKET sockfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (sockfd == INVALID_SOCKET)
			return std::unexpected(std::format("TCP socket creation failed: {}", net::wsa_error_string()));

		DWORD timeout_ms = 4000;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof timeout_ms);
		setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof timeout_ms);

		if (connect(sockfd, reinterpret_cast<const sockaddr*>(&dest), sizeof(dest)) == SOCKET_ERROR)
		{
			closesocket(sockfd);
			return std::unexpected(std::format("TCP connect failed: {}", net::wsa_error_string()));
		}

		if (auto r = build_query(record_type); not r)
		{
			closesocket(sockfd);
			return r;
		}

		// TCP DNS: 2-byte length prefix (RFC 1035 §4.2.2)
		const u16 msg_len     = htons(static_cast<u16>(query_length));
		const int prefix_sent = send(sockfd, reinterpret_cast<const char*>(&msg_len), sizeof(msg_len), 0);
		const int body_sent =
		  send(sockfd, reinterpret_cast<const char*>(query_buffer.data()), static_cast<int>(query_length), 0);
		if (prefix_sent == SOCKET_ERROR or body_sent == SOCKET_ERROR)
		{
			closesocket(sockfd);
			return std::unexpected(std::format("TCP send failed: {}", net::wsa_error_string()));
		}

		// Read 2-byte length prefix
		u16 resp_len_net{};
		if (recv(sockfd, reinterpret_cast<char*>(&resp_len_net), sizeof(resp_len_net), MSG_WAITALL) != sizeof(resp_len_net))
		{
			closesocket(sockfd);
			return std::unexpected("TCP recv length prefix failed");
		}
		const u16 resp_len = ntohs(resp_len_net);

		buffer.resize(resp_len);
		int received = 0;
		while (received < resp_len)
		{

			const int n = recv(sockfd, reinterpret_cast<char*>(buffer.data()) + received, resp_len - received, 0);

			dbg::println("TCP recv {} bytes ({} total, {})", n, received + n, resp_len - received);

			if (n <= 0)
			{
				closesocket(sockfd);
				return std::unexpected("TCP recv body failed");
			}
			received += n;
		}

		closesocket(sockfd);
		return parse_response(resp_len);
	}

	void set_dns_server(const std::string_view server, int version)
	{
		if (version == 6)
			dns_server_ipv6 = server;
		else if (version == 4)
			dns_server_ipv4 = server;
	}

private:
	std::string                         domain;
	std::string                         dns_server_ipv4, dns_server_ipv6;
	std::array<u8, MAX_DNS_BUFFER_SIZE> query_buffer = {};
	std::vector<u8>                     buffer;
	size_t                              query_length{0};

	u16 transaction_id{0};

	std::expected<void, std::string> build_query(u16 record_type)
	{
		transaction_id = random::randu16();
		DNSHeader* dns = reinterpret_cast<DNSHeader*>(query_buffer.data());
		dns->id        = htons(transaction_id);
		dns->qr        = 0;        // Query
		dns->opcode    = 0;        // Standard query
		dns->aa        = 0;        // Not Authoritative
		dns->tc        = 0;        // Not Truncated
		dns->rd        = 1;        // Recursion Desired
		dns->ra        = 0;        // Recursion Not Available
		dns->z         = 0;        // Reserved
		dns->rcode     = 0;        // No error
		dns->qcount    = htons(1); // One question
		dns->ancount   = 0;        // No answer
		dns->nscount   = 0;        // No authority
		dns->arcount   = 0;        // No additional

		dbg::println("transaction id: {:x}", transaction_id);

		size_t           offset = DNS_HEADER_SIZE;
		std::string_view rest{domain};
		constexpr size_t max_label_length = 63;

		while (not rest.empty())
		{
			const auto dot   = rest.find('.');
			auto       label = (dot == std::string_view::npos) ? rest : rest.substr(0, dot);

			if (label.empty() or label.size() > max_label_length)
				return std::unexpected("Invalid domain label");

			if (offset + 1 + label.size() + sizeof(DNSQuestion) >= query_buffer.size())
				return std::unexpected("Domain too long for DNS query");

			query_buffer[offset++] = static_cast<u8>(label.size());
			std::memcpy(query_buffer.data() + offset, label.data(), label.size());
			offset += label.size();

			if (dot == std::string_view::npos)
				break;

			rest.remove_prefix(dot + 1);
		}

		query_buffer[offset++] = 0;

		DNSQuestion qinfo{};
		qinfo.qtype  = htons(record_type);
		qinfo.qclass = htons(1);

		std::memcpy(query_buffer.data() + offset, &qinfo, sizeof(qinfo));
		query_length = offset + sizeof(qinfo);
		return {};
	}

	bool parse_name(size_t& offset, const size_t recv_len, std::string& out) const
	{
		out.clear();
		size_t cursor     = offset;
		bool   jumped     = false;
		u32    jump_count = 0;

		while (cursor < recv_len)
		{
			const u8 len = buffer[cursor];

			if ((len & 0xC0) == 0xC0)
			{
				if (cursor + 1 >= recv_len)
					return false;

				u16 pointer = static_cast<u16>((static_cast<u16>(len & 0x3F) << 8) | buffer[cursor + 1]);
				if (pointer >= recv_len)
					return false;

				if (not jumped)
					offset = cursor + 2;

				cursor = pointer;
				jumped = true;

				if (++jump_count > 20)
					return false;

				continue;
			}

			if (len == 0)
			{
				if (not jumped)
					offset = cursor + 1;
				return true;
			}

			if (len > 63)
				return false;

			++cursor;
			if (cursor + len > recv_len)
				return false;

			if (not out.empty())
				out += '.';

			out.append(reinterpret_cast<const char*>(buffer.data() + cursor), len);
			cursor += len;
		}

		return false;
	}

	bool read_u16(size_t& offset, const size_t recv_len, u16& out) const
	{
		if (offset + sizeof(u16) > recv_len)
			return false;

		std::memcpy(&out, buffer.data() + offset, sizeof(u16));
		offset += sizeof(u16);
		out = ntohs(out);
		return true;
	}

	bool read_u32(size_t& offset, const size_t recv_len, u32& out) const
	{
		if (offset + sizeof(u32) > recv_len)
			return false;

		std::memcpy(&out, buffer.data() + offset, sizeof(u32));
		offset += sizeof(u32);
		out = ntohl(out);
		return true;
	}

	std::expected<void, std::string> parse_response(size_t recv_len)
	{
		if (recv_len < DNS_HEADER_SIZE)
			return std::unexpected("DNS response too short");

		dbg::println("DNS response received, length: {}", recv_len);
		dbg::println("Raw response data:");
		dbg::println("{}", to_hex_string<u8>({buffer.data(), recv_len}, {.delimiter = " ", .show_hex = false}));


		auto read_be16_at = [&](size_t pos) -> std::optional<u16>
		{
			if (pos + sizeof(u16) > recv_len)
				return std::nullopt;

			u16 value{};
			std::memcpy(&value, buffer.data() + pos, sizeof(value));
			return ntohs(value);
		};

		const auto tid = read_be16_at(0);


		if (transaction_id != tid.value_or(0))
		{
			dbg::println("Expected {:x} got {:x}", transaction_id, tid.value_or(0));
			return std::unexpected("Transaction ID mismatch");
		}
		else
			dbg::println("Transaction ID: {:x}", tid.value_or(0));

		const auto flags = read_be16_at(2);

		if (not flags)
			return std::unexpected("Malformed DNS flags");

		const u16 f      = *flags;
		const u8  qr     = (f >> 15) & 0x1;
		const u8  opcode = (f >> 11) & 0xF;
		const u8  aa     = (f >> 10) & 0x1;
		const u8  tc     = (f >> 9) & 0x1;
		const u8  rd     = (f >> 8) & 0x1;
		const u8  ra     = (f >> 7) & 0x1;
		const u8  z      = (f >> 6) & 0x1;
		const u8  ad     = (f >> 5) & 0x1;
		const u8  cd     = (f >> 4) & 0x1;
		const u8  rcode  = (f >> 0) & 0xF;


		if (qr != 1)
			return std::unexpected("DNS response has QR=0 (not a response)");

		if (tc)
			return std::unexpected("TC");

		if (rcode != 0)
			return std::unexpected(std::format("DNS error rcode={}", rcode));

		dbg::println(
		  "Flags: qr={} opcode={} aa={} tc={} rd={} ra={} z={} ad={} cd={} rcode={}",
		  qr,
		  opcode,
		  aa,
		  tc,
		  rd,
		  ra,
		  z,
		  ad,
		  cd,
		  rcode);


		const auto q_count    = read_be16_at(4);
		const auto a_count    = read_be16_at(6);
		const auto auth_count = read_be16_at(8);
		const auto add_count  = read_be16_at(10);

		if (not q_count or not a_count or not auth_count or not add_count)
			return std::unexpected("Malformed DNS header");

		int question_count   = static_cast<int>(*q_count);
		int answer_count     = static_cast<int>(*a_count);
		int authority_count  = static_cast<int>(*auth_count);
		int additional_count = static_cast<int>(*add_count);

		dbg::println(
		  "Received {} bytes, Questions: {}, Answers: {}, Authority: {}, Additional: {}",
		  recv_len,
		  question_count,
		  answer_count,
		  authority_count,
		  additional_count);

		size_t offset = DNS_HEADER_SIZE;

		for (int i = 0; i < question_count; ++i)
		{
			std::string qname;
			u16         qtype{};
			u16         qclass{};

			if (not parse_name(offset, recv_len, qname) or not read_u16(offset, recv_len, qtype) or
				not read_u16(offset, recv_len, qclass))
				return std::unexpected("Malformed DNS question section");
		}

		auto parse_records = [&](std::string_view section_name, int count) -> std::expected<void, std::string>
		{
			dbg::println("{} records: {}", section_name, count);

			for (int i = 0; i < count; ++i)
			{
				std::string name;
				if (not parse_name(offset, recv_len, name))
					return std::unexpected("Malformed DNS name in answer");

				DNSAnswer answer;
				if (not read_u16(offset, recv_len, answer.type) or not read_u16(offset, recv_len, answer.class_) or
					not read_u32(offset, recv_len, answer.ttl) or not read_u16(offset, recv_len, answer.data_len))
					return std::unexpected("Malformed DNS answer header");

				if (offset + answer.data_len > recv_len)
					return std::unexpected("Malformed DNS answer payload");

				answer.data.resize(answer.data_len);
				std::memcpy(answer.data.data(), buffer.data() + offset, answer.data_len);
				const size_t rdata_start = offset;
				offset += answer.data_len;

				answer.name = name;

				dbg::println("{} Record {}:", section_name, i + 1);
				dbg::println("  Name: {}", answer.name);
				dbg::println("  Type: {}", answer.type);
				dbg::println("  Class: {}", answer.class_);
				dbg::println("  TTL: {}", answer.ttl);

				if (answer.type == 1)
				{
					char ipv4[INET_ADDRSTRLEN]{};
					if (answer.data.size() == 4 and inet_ntop(AF_INET, answer.data.data(), ipv4, sizeof(ipv4)) != nullptr)
						dbg::println("  Address (IPv4): {}", ipv4);
					else
						dbg::println("  Address (IPv4): <invalid>");
				}
				else if (answer.type == 2)
				{
					std::string ns_name;
					size_t      ns_offset = rdata_start;
					if (parse_name(ns_offset, recv_len, ns_name))
						dbg::println("  NS: {}", ns_name);
					else
						dbg::println("  NS: <invalid>");
				}
				else if (answer.type == 28)
				{
					char ipv6[INET6_ADDRSTRLEN]{};
					if (answer.data.size() == 16 and inet_ntop(AF_INET6, answer.data.data(), ipv6, sizeof(ipv6)) != nullptr)
						dbg::println("  Address (IPv6): {}", ipv6);
					else
						dbg::println("  Address (IPv6): <invalid>");
				}
				else
				{
					dbg::println(
					  "  Data (hex): {}", to_hex_string(std::span{answer.data}, {.delimiter = " ", .show_hex = false}));
				}
			}
			return {};
		};

		if (auto r = parse_records("Answer", answer_count); not r)
			return r;
		if (auto r = parse_records("Authority", authority_count); not r)
			return r;
		if (auto r = parse_records("Additional", additional_count); not r)
			return r;
		return {};
	}
};

enum BytecodeToken : u8
{
	PUSH = 0x01,
	ADD  = 0x02,
};

class Tokens
{
	std::vector<BytecodeToken> data;
	u32                        index{0};

public:
	std::optional<BytecodeToken> next()
	{
		if (index < data.size())
			return data[index++];

		return {};
	}
};

auto ip_query(std::string_view host, [[maybe_unused]] int version = 4) -> std::pair<std::string, std::chrono::milliseconds>
{
	const std::string req = std::format("GET / HTTP/1.0\r\nHost: {}\r\nConnection: close\r\n\r\n", host);

	net::socket s(net::endpoint{host, 80});
	s.write(req);
	if (not s.send())
		return {};

	auto [response, rtt] = s.recv_string();

	// Strip HTTP headers — body follows "\r\n\r\n"
	if (const auto pos = response.find("\r\n\r\n"); pos != std::string::npos)
	{
		auto ip = response.substr(pos + 4);
		// trim trailing whitespace/newline
		while (not ip.empty() and (ip.back() == '\n' or ip.back() == '\r' or ip.back() == ' '))
			ip.pop_back();
		return {ip, rtt};
	}

	return {response, rtt};
}

auto ip_query(int version = 4)
{
	const std::string host = (version == 6) ? "ipv6.icanhazip.com" : "ipv4.icanhazip.com";
	return ip_query(host, version);
}

auto ipify_query(int version = 4)
{
	const std::string host = (version == 6) ? "api6.ipify.org" : "api.ipify.org";
	return ip_query(host, version);
}

constexpr std::array<std::string_view, 128> koremutake_syllables = {
  {"BA",  "BE",  "BI",  "BO",  "BU",  "BY",  "DA",  "DE",  "DI",  "DO",  "DU",  "DY",  "FA",  "FE",  "FI",  "FO",
   "FU",  "FY",  "GA",  "GE",  "GI",  "GO",  "GU",  "GY",  "HA",  "HE",  "HI",  "HO",  "HU",  "HY",  "JA",  "JE",
   "JI",  "JO",  "JU",  "JY",  "KA",  "KE",  "KI",  "KO",  "KU",  "KY",  "LA",  "LE",  "LI",  "LO",  "LU",  "LY",
   "MA",  "ME",  "MI",  "MO",  "MU",  "MY",  "NA",  "NE",  "NI",  "NO",  "NU",  "NY",  "PA",  "PE",  "PI",  "PO",
   "PU",  "PY",  "RA",  "RE",  "RI",  "RO",  "RU",  "RY",  "SA",  "SE",  "SI",  "SO",  "SU",  "SY",  "TA",  "TE",
   "TI",  "TO",  "TU",  "TY",  "VA",  "VE",  "VI",  "VO",  "VU",  "VY",  "BRA", "BRE", "BRI", "BRO", "BRU", "BRY",
   "DRA", "DRE", "DRI", "DRO", "DRU", "DRY", "FRA", "FRE", "FRI", "FRO", "FRU", "FRY", "GRA", "GRE", "GRI", "GRO",
   "GRU", "GRY", "PRA", "PRE", "PRI", "PRO", "PRU", "PRY", "STA", "STE", "STI", "STO", "STU", "STY", "TRA", "TRE"}};

[[nodiscard]] auto koremutake_encode(uint64_t value) -> std::string
{
	if (value == 0)
		return std::string{koremutake_syllables[0]};

	std::string result;
	result.reserve(3 * 7);
	while (value--)
	{
		result = std::string{koremutake_syllables[random::randu8() & 0x7F]} + result;
	}
	return result;

	while (value > 0)
	{
		result = std::string{koremutake_syllables[value & 0x7F]} + result;
		value >>= 7;
	}
	return result;
}

[[nodiscard]] auto generate_human_name(std::mt19937& rng, std::size_t parts = 3) -> std::string
{
#if 0
	static constexpr std::array onset = {
	  "KAL"sv, "VER"sv, "TER"sv, "MAR"sv, "SOL"sv, "CEN"sv, "ARC"sv, "VAN"sv, "ORI"sv, "CAS"sv, "LEG"sv, "FOR"sv,
	  "IMP"sv, "COR"sv, "FAB"sv, "GAL"sv, "HAD"sv, "LAC"sv, "MAX"sv, "NAV"sv, "OPT"sv, "PAX"sv, "QUI"sv, "REG"sv,
	  "SEN"sv, "TIT"sv, "URB"sv, "VAL"sv, "XAN"sv, "ZEN"sv, "ALB"sv, "BEL"sv, "CAP"sv, "DEC"sv, "EQU"sv, "FEL"sv,
	  "GEN"sv, "HEL"sv, "INV"sv, "JUL"sv, "LAT"sv, "MER"sv, "NOM"sv, "OCT"sv, "POM"sv, "QUA"sv, "ROM"sv, "SAT"sv,
	  "TRO"sv, "ULP"sv, "VIC"sv, "WAR"sv, "XER"sv, "YOR"sv, "ZAC"sv, "ACQ"sv, "BRU"sv, "DOM"sv, "EXP"sv, "CLAv"sv,
	};

	static constexpr std::array nucleus = {
	  "IA"sv,  "IO"sv,  "AN"sv,  "EN"sv,  "ON"sv,  "AL"sv,  "EL"sv,  "IN"sv,  "UN"sv,  "ER"sv,  "AE"sv,  "OE"sv,
	  "AU"sv,  "EU"sv,  "UI"sv,  "IE"sv,  "UA"sv,  "UE"sv,  "OI"sv,  "ARI"sv, "ORI"sv, "ANI"sv, "ENI"sv, "ONI"sv,
	  "ALI"sv, "ELI"sv, "INI"sv, "UNI"sv, "ERI"sv, "AVA"sv, "EVE"sv, "IVI"sv, "OVO"sv, "UVU"sv, "ALA"sv, "ELE"sv,
	  "ILI"sv, "OLO"sv, "ULU"sv, "ANA"sv, "ENE"sv, "INI"sv, "ONO"sv, "UNU"sv, "ARA"sv, "ERE"sv, "IRI"sv, "ORO"sv,
	  "URU"sv, "AMA"sv, "EME"sv, "IMI"sv, "OMO"sv, "UMU"sv, "APA"sv, "EPE"sv, "IPI"sv, "OPO"sv, "UPU"sv, "OAE"sv,
	};

	static constexpr std::array coda = {
	  "US"sv,  "IX"sv,  "AN"sv,  "OR"sv,  "IS"sv,  "AR"sv,  "ON"sv,  "UM"sv,  "AX"sv,  "EN"sv,  "IUS"sv, "AUS"sv,
	  "EUS"sv, "OUS"sv, "UUS"sv, "INS"sv, "ANS"sv, "ENS"sv, "ONS"sv, "UNS"sv, "TOR"sv, "SOR"sv, "XOR"sv, "NOR"sv,
	  "COR"sv, "TUS"sv, "SUS"sv, "XUS"sv, "NUS"sv, "CUS"sv, "TIS"sv, "SIS"sv, "XIS"sv, "NIS"sv, "CIS"sv, "TUM"sv,
	  "SUM"sv, "XUM"sv, "NUM"sv, "LUM"sv, "TAX"sv, "SAX"sv, "XAX"sv, "NAX"sv, "CAX"sv, "TON"sv, "SON"sv, "XON"sv,
	  "NON"sv, "CON"sv, "TAN"sv, "SAN"sv, "XAN"sv, "NAN"sv, "CAN"sv, "TEN"sv, "SEN"sv, "XEN"sv, "NEN"sv, "CEN"sv,
	};
#endif

#if 0
	static constexpr std::array onset = {
	  "AET"sv, "CYR"sv, "DAX"sv, "ELY"sv, "EXO"sv, "GAL"sv, "HEL"sv, "HYP"sv, "ION"sv, "IXO"sv, "KAI"sv, "KYN"sv,
	  "LYR"sv, "NEX"sv, "NOV"sv, "NYX"sv, "ORB"sv, "ORN"sv, "OXY"sv, "PHO"sv, "PLX"sv, "POL"sv, "PRX"sv, "PSI"sv,
	  "PYR"sv, "QUA"sv, "RAD"sv, "REL"sv, "RHO"sv, "RYX"sv, "SOL"sv, "SYN"sv, "TAU"sv, "TEC"sv, "TEL"sv, "TER"sv,
	  "THY"sv, "TRX"sv, "VEC"sv, "VEG"sv, "VEL"sv, "VEN"sv, "VEX"sv, "VOI"sv, "VOL"sv, "VOR"sv, "XEN"sv, "XER"sv,
	  "XYL"sv, "YON"sv, "ZEL"sv, "ZEN"sv, "ZEP"sv, "ZET"sv, "ZON"sv, "ZOR"sv, "ZYG"sv, "ZYN"sv, "ZYP"sv, "ZYX"sv,
	};

	static constexpr std::array nucleus = {
	  "AE"sv,  "AI"sv,  "AO"sv,  "AU"sv,  "AX"sv,  "EA"sv,  "EI"sv,  "EO"sv,  "EU"sv,  "EX"sv,  "IA"sv,  "IE"sv,
	  "IO"sv,  "IU"sv,  "IX"sv,  "OA"sv,  "OE"sv,  "OI"sv,  "OU"sv,  "OX"sv,  "UA"sv,  "UE"sv,  "UI"sv,  "UO"sv,
	  "UX"sv,  "YA"sv,  "YE"sv,  "YI"sv,  "YO"sv,  "YU"sv,  "AEI"sv, "AEO"sv, "AEU"sv, "AIU"sv, "AOE"sv, "EAI"sv,
	  "EAO"sv, "EAU"sv, "EIO"sv, "EIU"sv, "IAE"sv, "IAO"sv, "IAU"sv, "IEA"sv, "IEO"sv, "OAE"sv, "OAI"sv, "OAU"sv,
	  "OEA"sv, "OEI"sv, "UAI"sv, "UAE"sv, "UAO"sv, "UEA"sv, "UEI"sv, "YAE"sv, "YAI"sv, "YAO"sv, "YEA"sv, "YEI"sv,
	};

	static constexpr std::array coda = {
	  "AX"sv,  "EX"sv,  "IX"sv,  "OX"sv,  "UX"sv,  "YX"sv,  "ANX"sv, "ENX"sv, "INX"sv, "ONX"sv, "ARX"sv, "ERX"sv,
	  "IRX"sv, "ORX"sv, "URX"sv, "ALX"sv, "ELX"sv, "ILX"sv, "OLX"sv, "ULX"sv, "ON"sv,  "AN"sv,  "EN"sv,  "IN"sv,
	  "UN"sv,  "YN"sv,  "AON"sv, "EON"sv, "ION"sv, "UON"sv, "OR"sv,  "AR"sv,  "ER"sv,  "IR"sv,  "UR"sv,  "YR"sv,
	  "AOR"sv, "EOR"sv, "IOR"sv, "UOR"sv, "OS"sv,  "AS"sv,  "ES"sv,  "IS"sv,  "US"sv,  "YS"sv,  "AOS"sv, "EOS"sv,
	  "IOS"sv, "UOS"sv, "OZ"sv,  "AZ"sv,  "EZ"sv,  "IZ"sv,  "UZ"sv,  "YZ"sv,  "AOZ"sv, "EOZ"sv, "IOZ"sv, "UOZ"sv,
	};
#endif

#if 1
	static constexpr std::array onset = {
	  "AEL"sv, "AER"sv, "ALT"sv, "ARA"sv, "ARD"sv, "ARG"sv, "ASH"sv, "ATH"sv, "AVA"sv, "AYR"sv, "BEL"sv, "BOR"sv,
	  "BRA"sv, "BRE"sv, "CAL"sv, "CER"sv, "DAL"sv, "DOR"sv, "DRA"sv, "DWA"sv, "ELD"sv, "ELF"sv, "ELM"sv, "ELR"sv,
	  "EMB"sv, "ERE"sv, "ETH"sv, "EVA"sv, "FAL"sv, "FEN"sv, "GAL"sv, "GAR"sv, "GLA"sv, "GOR"sv, "HAL"sv, "HEL"sv,
	  "ISH"sv, "IVA"sv, "KAL"sv, "KER"sv, "LAR"sv, "LOR"sv, "LYN"sv, "MAL"sv, "MIR"sv, "MOR"sv, "NAL"sv, "NOR"sv,
	  "OAK"sv, "OHN"sv, "ORM"sv, "RAL"sv, "RAN"sv, "RIM"sv, "ROW"sv, "RYN"sv, "SAL"sv, "SIL"sv, "TAL"sv, "THR"sv,
	};

	static constexpr std::array nucleus = {
	  "AA"sv,  "AE"sv,  "AI"sv,  "AL"sv,  "AN"sv,  "AR"sv,  "EA"sv,  "EL"sv,  "EN"sv,  "ER"sv,  "IA"sv,  "IE"sv,
	  "IL"sv,  "IN"sv,  "IR"sv,  "OA"sv,  "OE"sv,  "OI"sv,  "OL"sv,  "ON"sv,  "UA"sv,  "UE"sv,  "UI"sv,  "UL"sv,
	  "UN"sv,  "YA"sv,  "YE"sv,  "YL"sv,  "YN"sv,  "YR"sv,  "AEL"sv, "AER"sv, "AIR"sv, "ALI"sv, "ANI"sv, "EAL"sv,
	  "EAR"sv, "EIL"sv, "ELI"sv, "ENI"sv, "IAL"sv, "IAR"sv, "IEL"sv, "ILI"sv, "INI"sv, "OAL"sv, "OAR"sv, "OEL"sv,
	  "OLI"sv, "ONI"sv, "UAL"sv, "UAR"sv, "UEL"sv, "ULI"sv, "UNI"sv, "YAL"sv, "YAR"sv, "YEL"sv, "YLI"sv, "YNI"sv,
	};

	static constexpr std::array coda = {
	  "AL"sv,  "AN"sv,  "AR"sv,  "AS"sv,  "ATH"sv, "EL"sv,  "EN"sv,  "ER"sv,  "ES"sv,  "ETH"sv, "IAL"sv, "IAN"sv,
	  "IAR"sv, "IEL"sv, "IEN"sv, "IL"sv,  "IN"sv,  "IR"sv,  "IS"sv,  "ITH"sv, "OAL"sv, "OAN"sv, "OAR"sv, "OEL"sv,
	  "OEN"sv, "OL"sv,  "ON"sv,  "OR"sv,  "OS"sv,  "OTH"sv, "UAL"sv, "UAN"sv, "UAR"sv, "UEL"sv, "UEN"sv, "UL"sv,
	  "UN"sv,  "UR"sv,  "US"sv,  "UTH"sv, "AND"sv, "END"sv, "IND"sv, "OND"sv, "UND"sv, "ARD"sv, "ERD"sv, "IRD"sv,
	  "ORD"sv, "URD"sv, "ALD"sv, "ELD"sv, "ILD"sv, "OLD"sv, "ULD"sv, "ANE"sv, "ENE"sv, "INE"sv, "ONE"sv, "UNE"sv,
	};
#endif

#if 0
	static constexpr std::array onset = {
	  "AQU"sv, "ARG"sv, "AST"sv, "AUR"sv, "AXI"sv, "AZU"sv, "CAE"sv, "CAL"sv, "CAS"sv, "CER"sv, "COR"sv, "CRU"sv,
	  "CYG"sv, "DRA"sv, "ERI"sv, "EUX"sv, "FOR"sv, "GAL"sv, "GEM"sv, "GLA"sv, "HEL"sv, "HYD"sv, "HYP"sv, "IGN"sv,
	  "ION"sv, "KAL"sv, "KEP"sv, "KET"sv, "KOI"sv, "KRO"sv, "LYR"sv, "MAG"sv, "MAR"sv, "MER"sv, "MOR"sv, "NAB"sv,
	  "NEP"sv, "NOV"sv, "NUB"sv, "OBE"sv, "OPH"sv, "ORB"sv, "ORI"sv, "ORP"sv, "PAL"sv, "PER"sv, "PHO"sv, "PLU"sv,
	  "POL"sv, "PUL"sv, "RAD"sv, "RHO"sv, "RIG"sv, "SAG"sv, "SEL"sv, "SER"sv, "SOL"sv, "TAU"sv, "TER"sv, "THA"sv,
	};

	static constexpr std::array nucleus = {
	  "AA"sv,  "AE"sv,  "AI"sv,  "AO"sv,  "AU"sv,  "EA"sv,  "EI"sv,  "EO"sv,  "EU"sv,  "IA"sv,  "IE"sv,  "IO"sv,
	  "IU"sv,  "OA"sv,  "OE"sv,  "OI"sv,  "OU"sv,  "UA"sv,  "UE"sv,  "UI"sv,  "ABI"sv, "ADI"sv, "AKI"sv, "ALI"sv,
	  "AMI"sv, "ANI"sv, "API"sv, "ARI"sv, "ASI"sv, "ATI"sv, "EBI"sv, "EDI"sv, "EKI"sv, "ELI"sv, "EMI"sv, "ENI"sv,
	  "EPI"sv, "ERI"sv, "ESI"sv, "ETI"sv, "IBA"sv, "IDA"sv, "IKA"sv, "ILA"sv, "IMA"sv, "INA"sv, "IPA"sv, "IRA"sv,
	  "ISA"sv, "ITA"sv, "OBA"sv, "ODA"sv, "OKA"sv, "OLA"sv, "OMA"sv, "ONA"sv, "OPA"sv, "ORA"sv, "OSA"sv, "OTA"sv,
	};

	static constexpr std::array coda = {
	  "AX"sv,  "IX"sv,  "OX"sv,   "UX"sv,   "YX"sv,   "ION"sv,  "EON"sv,  "AON"sv,  "UON"sv,  "YON"sv,  "ARA"sv,  "ERA"sv,
	  "IRA"sv, "ORA"sv, "URA"sv,  "ALA"sv,  "ELA"sv,  "ILA"sv,  "OLA"sv,  "ULA"sv,  "ANE"sv,  "ENE"sv,  "INE"sv,  "ONE"sv,
	  "UNE"sv, "ARE"sv, "ERE"sv,  "IRE"sv,  "ORE"sv,  "URE"sv,  "AND"sv,  "END"sv,  "IND"sv,  "OND"sv,  "UND"sv,  "ANT"sv,
	  "ENT"sv, "INT"sv, "ONT"sv,  "UNT"sv,  "ARS"sv,  "ERS"sv,  "IRS"sv,  "ORS"sv,  "URS"sv,  "ALS"sv,  "ELS"sv,  "ILS"sv,
	  "OLS"sv, "ULS"sv, "ARIA"sv, "ERIA"sv, "IRIA"sv, "ORIA"sv, "URIA"sv, "ALIS"sv, "ELIS"sv, "ILIS"sv, "OLIS"sv, "ULIS"sv,
	};
#endif

#if 0
	// Japanese-inspired
	static constexpr std::array onset = {
	  "AKA"sv, "AKI"sv, "AKO"sv, "AKU"sv, "AMA"sv, "AMI"sv, "AMO"sv, "AMU"sv, "ANA"sv, "ANI"sv, "AOI"sv, "ARA"sv,
	  "ARI"sv, "ARU"sv, "ASA"sv, "ASI"sv, "ASO"sv, "ASU"sv, "ATA"sv, "ATO"sv, "EKA"sv, "EKI"sv, "EMA"sv, "EMI"sv,
	  "ENA"sv, "ENI"sv, "ERA"sv, "ERI"sv, "ESA"sv, "ETA"sv, "IKA"sv, "IKI"sv, "IKO"sv, "IKU"sv, "IMA"sv, "IMI"sv,
	  "INA"sv, "INI"sv, "IRA"sv, "IRI"sv, "OKA"sv, "OKI"sv, "OKO"sv, "OKU"sv, "OMA"sv, "OMI"sv, "ONA"sv, "ONI"sv,
	  "ORA"sv, "ORI"sv, "UKA"sv, "UKI"sv, "UMA"sv, "UMI"sv, "UNA"sv, "UNI"sv, "URA"sv, "URI"sv, "USA"sv, "UTA"sv,
	};

	static constexpr std::array nucleus = {
	  "A"sv,  "I"sv,  "U"sv,  "E"sv,  "O"sv,  "AI"sv, "AU"sv, "IE"sv, "IO"sv, "IU"sv, "OA"sv, "OE"sv, "OI"sv, "UA"sv, "UE"sv,
	  "UI"sv, "UO"sv, "AE"sv, "AO"sv, "EI"sv, "KA"sv, "KI"sv, "KU"sv, "KE"sv, "KO"sv, "MA"sv, "MI"sv, "MU"sv, "ME"sv, "MO"sv,
	  "NA"sv, "NI"sv, "NU"sv, "NE"sv, "NO"sv, "RA"sv, "RI"sv, "RU"sv, "RE"sv, "RO"sv, "SA"sv, "SI"sv, "SU"sv, "SE"sv, "SO"sv,
	  "TA"sv, "TI"sv, "TU"sv, "TE"sv, "TO"sv, "WA"sv, "WI"sv, "WU"sv, "WE"sv, "WO"sv, "YA"sv, "YI"sv, "YU"sv, "YE"sv, "YO"sv,
	};

	static constexpr std::array coda = {
	  "KA"sv,  "KI"sv,  "KO"sv,  "KU"sv,  "KE"sv,  "MA"sv,  "MI"sv,  "MO"sv,  "MU"sv,  "ME"sv,  "NA"sv,  "NI"sv,
	  "NO"sv,  "NU"sv,  "NE"sv,  "RA"sv,  "RI"sv,  "RO"sv,  "RU"sv,  "RE"sv,  "SHA"sv, "SHI"sv, "SHO"sv, "SHU"sv,
	  "SHE"sv, "CHI"sv, "CHA"sv, "CHO"sv, "CHU"sv, "CHE"sv, "TSA"sv, "TSI"sv, "TSO"sv, "TSU"sv, "TSE"sv, "ZHA"sv,
	  "ZHI"sv, "ZHO"sv, "ZHU"sv, "ZHE"sv, "RYA"sv, "RYI"sv, "RYO"sv, "RYU"sv, "RYE"sv, "MYA"sv, "MYI"sv, "MYO"sv,
	  "MYU"sv, "MYE"sv, "NYA"sv, "NYI"sv, "NYO"sv, "NYU"sv, "NYE"sv, "KYA"sv, "KYI"sv, "KYO"sv, "KYU"sv, "KYE"sv,
	};
#endif

#if 0
	// Korean-inspired
	static constexpr std::array onset = {
	  "BAE"sv, "BAK"sv, "BAL"sv, "BAM"sv, "BAN"sv, "BAP"sv, "BAR"sv, "BAS"sv, "BAT"sv, "BAU"sv, "BEO"sv, "BEU"sv,
	  "BEY"sv, "BIN"sv, "BOM"sv, "BON"sv, "BOP"sv, "BOR"sv, "BOT"sv, "BOU"sv, "BUK"sv, "BUL"sv, "BUM"sv, "BUN"sv,
	  "BUP"sv, "BUR"sv, "BUS"sv, "BUT"sv, "BUY"sv, "BYE"sv, "CHA"sv, "CHE"sv, "CHO"sv, "CHU"sv, "DAE"sv, "DAK"sv,
	  "DAL"sv, "DAM"sv, "DAN"sv, "DAP"sv, "DEO"sv, "DEU"sv, "DEY"sv, "DIN"sv, "DOM"sv, "DON"sv, "DOP"sv, "DOR"sv,
	  "DOT"sv, "DOU"sv, "DUK"sv, "DUL"sv, "DUM"sv, "DUN"sv, "DUP"sv, "DUR"sv, "DUS"sv, "DUT"sv, "GAE"sv, "GAK"sv,
	};

	static constexpr std::array nucleus = {
	  "AE"sv,  "AK"sv,  "AL"sv,  "AM"sv,  "AN"sv,  "AP"sv,  "AR"sv,  "AS"sv,  "AT"sv,  "AU"sv,  "EO"sv,  "EU"sv,
	  "EY"sv,  "IN"sv,  "OM"sv,  "ON"sv,  "OP"sv,  "OR"sv,  "OT"sv,  "OU"sv,  "UK"sv,  "UL"sv,  "UM"sv,  "UN"sv,
	  "UP"sv,  "UR"sv,  "US"sv,  "UT"sv,  "UY"sv,  "YE"sv,  "ANG"sv, "ENG"sv, "ING"sv, "ONG"sv, "UNG"sv, "ANK"sv,
	  "ENK"sv, "INK"sv, "ONK"sv, "UNK"sv, "AEO"sv, "AEU"sv, "EON"sv, "EOU"sv, "OEA"sv, "OEU"sv, "UAE"sv, "UAO"sv,
	  "UEA"sv, "UEO"sv, "YAE"sv, "YAK"sv, "YAL"sv, "YAM"sv, "YAN"sv, "YAP"sv, "YAR"sv, "YAS"sv, "YAT"sv, "YAU"sv,
	};

	static constexpr std::array coda = {
	  "AK"sv,  "AL"sv,  "AM"sv,  "AN"sv,  "AP"sv,  "AR"sv,  "AS"sv,  "AT"sv,  "ANG"sv, "ANK"sv, "EK"sv,   "EL"sv,
	  "EM"sv,  "EN"sv,  "EP"sv,  "ER"sv,  "ES"sv,  "ET"sv,  "ENG"sv, "ENK"sv, "IK"sv,  "IL"sv,  "IM"sv,   "IN"sv,
	  "IP"sv,  "IR"sv,  "IS"sv,  "IT"sv,  "ING"sv, "INK"sv, "OK"sv,  "OL"sv,  "OM"sv,  "ON"sv,  "OP"sv,   "OR"sv,
	  "OS"sv,  "OT"sv,  "ONG"sv, "ONK"sv, "UK"sv,  "UL"sv,  "UM"sv,  "UN"sv,  "UP"sv,  "UR"sv,  "US"sv,   "UT"sv,
	  "UNG"sv, "UNK"sv, "YAK"sv, "YAL"sv, "YAM"sv, "YAN"sv, "YAP"sv, "YAR"sv, "YAS"sv, "YAT"sv, "YANG"sv, "YANK"sv,
	};
#endif

#if 0
	// Mandarin-inspired
	static constexpr std::array onset = {
	  "BAI"sv, "BEI"sv, "BIN"sv, "CAI"sv, "CAN"sv, "CAO"sv, "CEI"sv, "CEN"sv, "CHA"sv, "CHE"sv, "CHI"sv, "CHO"sv,
	  "CHU"sv, "DAI"sv, "DAN"sv, "DAO"sv, "DEI"sv, "DEN"sv, "DIA"sv, "DIE"sv, "DIN"sv, "DON"sv, "DOU"sv, "DUA"sv,
	  "DUI"sv, "DUN"sv, "DUO"sv, "FAI"sv, "FAN"sv, "FAO"sv, "FEI"sv, "FEN"sv, "FIA"sv, "GAI"sv, "GAN"sv, "GAO"sv,
	  "GEI"sv, "GEN"sv, "GOU"sv, "GUA"sv, "GUI"sv, "GUN"sv, "GUO"sv, "HAI"sv, "HAN"sv, "HAO"sv, "HEI"sv, "HEN"sv,
	  "HOU"sv, "HUA"sv, "HUI"sv, "HUN"sv, "HUO"sv, "JIA"sv, "JIE"sv, "JIN"sv, "JOU"sv, "JUA"sv, "JUI"sv, "JUN"sv,
	};

	static constexpr std::array nucleus = {
	  "AI"sv,  "AN"sv,  "AO"sv,  "EI"sv,  "EN"sv,  "IA"sv,  "IE"sv,  "IN"sv,  "IU"sv,  "OU"sv,  "UA"sv,  "UE"sv,
	  "UI"sv,  "UN"sv,  "UO"sv,  "YA"sv,  "YE"sv,  "YI"sv,  "YO"sv,  "YU"sv,  "ANG"sv, "ENG"sv, "ING"sv, "ONG"sv,
	  "UNG"sv, "AIN"sv, "EIN"sv, "IAN"sv, "UAN"sv, "YAN"sv, "IAO"sv, "IOU"sv, "UAI"sv, "UEI"sv, "UAO"sv, "YAO"sv,
	  "YOU"sv, "WAI"sv, "WEI"sv, "WAN"sv, "ANG"sv, "ENG"sv, "ING"sv, "ONG"sv, "UNG"sv, "YNG"sv, "ANR"sv, "ENR"sv,
	  "INR"sv, "ONR"sv, "IAR"sv, "IER"sv, "UAR"sv, "UER"sv, "YAR"sv, "YER"sv, "AIR"sv, "EIR"sv, "UIR"sv, "UOR"sv,
	};

	static constexpr std::array coda = {
	  "AN"sv,   "EN"sv,  "IN"sv,  "ON"sv,  "UN"sv,  "ANG"sv, "ENG"sv,  "ING"sv, "ONG"sv,  "UNG"sv,  "AI"sv,   "EI"sv,
	  "UI"sv,   "AO"sv,  "OU"sv,  "IA"sv,  "IE"sv,  "UA"sv,  "UE"sv,   "UO"sv,  "LI"sv,   "LIN"sv,  "LING"sv, "LAN"sv,
	  "LANG"sv, "LEI"sv, "LOU"sv, "LUN"sv, "LUO"sv, "LYU"sv, "MEI"sv,  "MEN"sv, "MIN"sv,  "MING"sv, "MOU"sv,  "MUA"sv,
	  "MUI"sv,  "MUN"sv, "MUO"sv, "MYU"sv, "NAN"sv, "NEI"sv, "NEN"sv,  "NIN"sv, "NING"sv, "NOU"sv,  "NUA"sv,  "NUI"sv,
	  "NUN"sv,  "NUO"sv, "RAN"sv, "REI"sv, "REN"sv, "RIN"sv, "RING"sv, "ROU"sv, "RUA"sv,  "RUI"sv,  "RUN"sv,  "RUO"sv,
	};
#endif

#if 0
	// Cyber / Hacking
static constexpr std::array onset = {
    "BIN"sv, "BIT"sv, "BUF"sv, "BYT"sv, "CMD"sv, "COD"sv, "COR"sv, "CPU"sv, "CRC"sv, "CRY"sv,
    "DAT"sv, "DEC"sv, "DEF"sv, "DEX"sv, "DIF"sv, "DIG"sv, "DMA"sv, "DNS"sv, "DOM"sv, "DYN"sv,
    "ENC"sv, "ENT"sv, "EXE"sv, "EXP"sv, "EXT"sv, "FAT"sv, "FLO"sv, "FLX"sv, "FMT"sv, "FUZ"sv,
    "GDB"sv, "GLB"sv, "GPU"sv, "GRF"sv, "GRP"sv, "HAX"sv, "HEX"sv, "HOP"sv, "HTM"sv, "HUB"sv,
    "INJ"sv, "INT"sv, "INX"sv, "IOC"sv, "IPC"sv, "IRQ"sv, "ISR"sv, "JAV"sv, "JIT"sv, "JSN"sv,
    "KER"sv, "KEY"sv, "KNL"sv, "LAT"sv, "LIN"sv, "LNK"sv, "LOG"sv, "LOP"sv, "LSB"sv, "LUT"sv,
};

static constexpr std::array nucleus = {
    "0X"sv,  "1N"sv,  "2D"sv,  "3R"sv,  "4T"sv,  "5K"sv,  "6L"sv,  "7M"sv,  "8S"sv,  "9V"sv,
    "AC"sv,  "AL"sv,  "AN"sv,  "AR"sv,  "AT"sv,  "AX"sv,  "EC"sv,  "EL"sv,  "EN"sv,  "ER"sv,
    "EX"sv,  "IC"sv,  "IL"sv,  "IN"sv,  "IR"sv,  "IX"sv,  "OC"sv,  "OL"sv,  "ON"sv,  "OR"sv,
    "OX"sv,  "UC"sv,  "UL"sv,  "UN"sv,  "UR"sv,  "UX"sv,  "ACK"sv, "ALT"sv, "ANT"sv, "ARP"sv,
    "ASM"sv, "AST"sv, "ATM"sv, "AXE"sv, "ECH"sv, "EDG"sv, "EKT"sv, "ELT"sv, "EMU"sv, "ENT"sv,
    "ERR"sv, "ESC"sv, "ETH"sv, "EXT"sv, "IFK"sv, "IGN"sv, "IKT"sv, "ILT"sv, "IMT"sv, "INT"sv,
};

static constexpr std::array coda = {
    "ASM"sv, "BUS"sv, "CMD"sv, "COR"sv, "CPU"sv, "CRC"sv, "CTL"sv, "DLL"sv, "DMP"sv, "DNS"sv,
    "EXE"sv, "EXT"sv, "FAT"sv, "FMT"sv, "FSB"sv, "GPU"sv, "GRP"sv, "HAL"sv, "HEX"sv, "HTM"sv,
    "INT"sv, "IPC"sv, "IRQ"sv, "ISR"sv, "JIT"sv, "JSN"sv, "KNL"sv, "LNK"sv, "LOG"sv, "LSB"sv,
    "MMU"sv, "MOD"sv, "MSB"sv, "MTX"sv, "MUX"sv, "NET"sv, "NIC"sv, "NOP"sv, "NUL"sv, "OBJ"sv,
    "OOM"sv, "OPT"sv, "ORB"sv, "OVF"sv, "PCB"sv, "PID"sv, "PKT"sv, "PLT"sv, "POP"sv, "PTR"sv,
    "RAM"sv, "REG"sv, "ROM"sv, "RPC"sv, "RST"sv, "RTT"sv, "SIG"sv, "SMP"sv, "SQL"sv, "SYS"sv,
};
#endif


#if 0
// Nanotech / Biotech
static constexpr std::array onset = {
    "ACT"sv, "ADN"sv, "ALG"sv, "AMI"sv, "AMN"sv, "AMP"sv, "AMY"sv, "ANT"sv, "APO"sv, "AQU"sv,
    "ARG"sv, "ASP"sv, "ATP"sv, "AXN"sv, "AZT"sv, "BIO"sv, "CAT"sv, "CEL"sv, "CHR"sv, "CLA"sv,
    "CLO"sv, "COD"sv, "COR"sv, "CRY"sv, "CYT"sv, "DEN"sv, "DIF"sv, "DIG"sv, "DIN"sv, "DNA"sv,
    "DOP"sv, "DYN"sv, "ELT"sv, "EMB"sv, "ENC"sv, "ENZ"sv, "EPG"sv, "ERY"sv, "EVO"sv, "EXO"sv,
    "FAG"sv, "FER"sv, "FIB"sv, "FLU"sv, "GAM"sv, "GEN"sv, "GLU"sv, "GLY"sv, "GON"sv, "GYR"sv,
    "HAP"sv, "HEL"sv, "HEM"sv, "HEP"sv, "HET"sv, "HEX"sv, "HIB"sv, "HIS"sv, "HOL"sv, "HOM"sv,
};

static constexpr std::array nucleus = {
    "ACE"sv, "ACI"sv, "ACO"sv, "ACU"sv, "ADE"sv, "ADI"sv, "ADO"sv, "ADU"sv, "AGE"sv, "AGI"sv,
    "AGO"sv, "AGU"sv, "AKE"sv, "AKI"sv, "AKO"sv, "AKU"sv, "ALE"sv, "ALI"sv, "ALO"sv, "ALU"sv,
    "AME"sv, "AMI"sv, "AMO"sv, "AMU"sv, "ANE"sv, "ANI"sv, "ANO"sv, "ANU"sv, "APE"sv, "API"sv,
    "APO"sv, "APU"sv, "ARE"sv, "ARI"sv, "ARO"sv, "ARU"sv, "ASE"sv, "ASI"sv, "ASO"sv, "ASU"sv,
    "ATE"sv, "ATI"sv, "ATO"sv, "ATU"sv, "AVE"sv, "AVI"sv, "AVO"sv, "AVU"sv, "AXE"sv, "AXI"sv,
    "AXO"sv, "AXU"sv, "AYE"sv, "AYI"sv, "AYO"sv, "AYU"sv, "AZE"sv, "AZI"sv, "AZO"sv, "AZU"sv,
};

static constexpr std::array coda = {
    "ASE"sv, "ATE"sv, "ENE"sv, "IDE"sv, "INE"sv, "ION"sv, "IRE"sv, "ISE"sv, "ITE"sv, "IVE"sv,
    "OID"sv, "OIN"sv, "OIS"sv, "OIT"sv, "OIV"sv, "OKE"sv, "OKI"sv, "OKO"sv, "OKU"sv, "OLE"sv,
    "OLI"sv, "OLO"sv, "OLU"sv, "OME"sv, "OMI"sv, "OMO"sv, "OMU"sv, "ONE"sv, "ONI"sv, "ONO"sv,
    "ONU"sv, "OPE"sv, "OPI"sv, "OPO"sv, "OPU"sv, "ORE"sv, "ORI"sv, "ORO"sv, "ORU"sv, "OSE"sv,
    "OSI"sv, "OSO"sv, "OSU"sv, "OTE"sv, "OTI"sv, "OTO"sv, "OTU"sv, "OVE"sv, "OVI"sv, "OVO"sv,
    "OVU"sv, "OXE"sv, "OXI"sv, "OXO"sv, "OXU"sv, "OYE"sv, "OYI"sv, "OYO"sv, "OYU"sv, "OZE"sv,
};
#endif

	auto pick = [&](std::span<const std::string_view> t) -> std::string_view
	{ return t[std::uniform_int_distribution<std::size_t>{0, t.size() - 1}(rng)]; };

	std::string result;
	for (auto i : std::views::iota(0uz, parts))
	{
		const bool last = (i == parts - 1);
		result += last ? pick(coda) : (i % 2 == 0) ? pick(onset) : pick(nucleus);
	}
	return result;
}

i32 deckard_main([[maybe_unused]] utf8::view commandline)
{
	dbg::println("cmd: {}", commandline);

#if 0
	u64        lines = 0;
	ScopeTimer timer("File writing");
	timer.start();
	for (auto& out : file::writer({.filename = "newfile7.txt"_path, .limit = 16_GiB}))
	{
		out.write("Line {}\n", ++lines);

		if (lines >= 1'000'000'000)
		{
			out.stop();
			break;
		}
	}
	timer.now();
#endif



	
	std::mt19937 rng(random::seed<u32>());

	for ([[maybe_unused]] const auto i : range(0, 100))
	{
		auto pss1 = koremutake_encode(random::randu8(2, 7));

		dbg::println("Koremutake: {: >21}", pss1);
	}

	for ([[maybe_unused]] const auto i : range(0, 100))
	{
		auto pss = generate_human_name(rng, random::randu8(2, 3));

		dbg::println("Human Name: {: >16}", pss);
	}


	std::unordered_set<utf8::string> keywords;

	keywords.insert("if"sv);
	keywords.insert("fn"sv);
	keywords.insert("ident"sv);


	std::vector<utf8::string> test_strings = {"if"sv, "else"sv, "fn"sv, "while"sv};


	for (const auto& test : test_strings)
		if (keywords.contains(test))
			info("'{}' is a keyword", test);
		else
			info("'{}' is not a keyword", test);

	_ = 0;


	/* deckard::threadpool tp(0);
	tp.post([] { info("Hello from threadpool!"); });

	auto task = [](deckard::threadpool& tp, int id)
	{
		info("Task {} started", id);
		std::this_thread::sleep_for(1s);

		while(tp.is_running())
		{
			info("Task {} is running...", id);
			std::this_thread::sleep_for(1s);
		}

		tp.post([id] {
			info("Task {} completed", id);
		});
	};


	tp.post(task);


	*/


	auto [body, rtt] = ip_query();
	dbg::println("Public IPv4: {} - ({})", body, rtt);

	auto [body2, rtt2] = ip_query(6);
	dbg::println("Public IPv6: {} - ({})", body2, rtt2);

	auto [body3, rtt3] = ipify_query();
	dbg::println("Public IPv4: {} - ({})", body3, rtt3);

	auto [body4, rtt4] = ipify_query(6);
	dbg::println("Public IPv6: {} - ({})", body4, rtt4);


	_ = 0;


	info("░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░░▒▓█▓▒░");
	info("✅ ⚠️ ❌ ❗");

	auto ips1 = net::resolve_ips("1.1.1.1", 4);
	auto ips2 = net::resolve_ips("2606:4700:4700::1111", 6);

	auto ips = net::resolve_ips("api.taboobuilder.com", 6);
	for (const auto& ip : ips.value_or({}))
		dbg::println("Resolved IP: {}", ip);


	dbg::println("Hostname: {}", net::hostname());


	net::endpoint ep("api.taboobuilder.com", 80);

	dbg::println("Measused MTU: {}", net::measure_mtu_for_target({"api.taboobuilder.com", 80}));


	dbg::println("Endpoint: {}", ep.to_string());
	_ = 0;

	{
		SOCKET server = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (server == INVALID_SOCKET)
		{
			dbg::println("Failed to create server socket: {}", WSAGetLastError());
			return 1;
		}

		sockaddr_in server_addr{};
		server_addr.sin_family      = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		server_addr.sin_port        = htons(0);

		if (::bind(server, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR)
		{
			dbg::println("bind() failed: {}", WSAGetLastError());
			::closesocket(server);
			return 1;
		}

		if (::listen(server, 1) == SOCKET_ERROR)
		{
			dbg::println("listen() failed: {}", WSAGetLastError());
			::closesocket(server);
			return 1;
		}

		int server_len = static_cast<int>(sizeof(server_addr));
		if (::getsockname(server, reinterpret_cast<sockaddr*>(&server_addr), &server_len) == SOCKET_ERROR)
		{
			dbg::println("getsockname() failed: {}", WSAGetLastError());
			::closesocket(server);
			return 1;
		}


		// #################
		// #################
		// #################
		// #################
		// #################


		SOCKET client = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (client == INVALID_SOCKET)
		{
			dbg::println("Failed to create client socket: {}", WSAGetLastError());
			::closesocket(server);
			return 1;
		}

		if (::connect(client, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR)
		{
			dbg::println("connect() failed: {}", WSAGetLastError());
			::closesocket(client);
			::closesocket(server);
			return 1;
		}

		SOCKET accepted = ::accept(server, nullptr, nullptr);
		if (accepted == INVALID_SOCKET)
		{
			dbg::println("accept() failed: {}", WSAGetLastError());
			::closesocket(client);
			::closesocket(server);
			return 1;
		}

		{
			sockaddr_storage peer_addr{};
			int              peer_len = sizeof(peer_addr);
			if (::getpeername(client, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len) == 0)
			{
				char ip[INET6_ADDRSTRLEN]{};
				u16  port{};
				if (peer_addr.ss_family == AF_INET)
				{
					auto& in = reinterpret_cast<sockaddr_in&>(peer_addr);
					inet_ntop(AF_INET, &in.sin_addr, ip, sizeof(ip));
					port = ntohs(in.sin_port);
				}
				else if (peer_addr.ss_family == AF_INET6)
				{
					auto& in6 = reinterpret_cast<sockaddr_in6&>(peer_addr);
					inet_ntop(AF_INET6, &in6.sin6_addr, ip, sizeof(ip));
					port = ntohs(in6.sin6_port);
				}
				dbg::println("client peer: {}:{}", ip, port);
			}
			else
				dbg::println("getpeername() failed: {}", WSAGetLastError());
		}

		u32 sent_bytes = 0;
		u32 recv_bytes = 0;

		auto sender = std::jthread(
		  [client, &sent_bytes](std::stop_token st)
		  {
			  while (not st.stop_requested())
			  {
				  std::string msg = "hello from sender\n";

				  msg   = random::password(1024);
				  int n = ::send(client, msg.data(), static_cast<int>(msg.size()), 0);
				  if (n == SOCKET_ERROR)
				  {
					  dbg::println("send() failed: {}", WSAGetLastError());
					  break;
				  }
				  // dbg::println("Sent {} bytes", n);
				  sent_bytes++;
				  // std::this_thread::sleep_for(std::chrono::milliseconds(250));
			  }
		  });

		// #################
		// #################


		auto receiver = std::jthread(
		  [accepted, &recv_bytes](std::stop_token st)
		  {
			  std::array<char, 1024> buf{};
			  while (not st.stop_requested())
			  {
				  int n = ::recv(accepted, buf.data(), static_cast<int>(buf.size()), 0);
				  if (n <= 0)
				  {
					  dbg::println("recv() ended: {}", WSAGetLastError());
					  break;
				  }
				  // dbg::println("Received {} bytes", n);
				  recv_bytes++;
				  // dbg::println("Message: {}", std::string_view{buf.data(), static_cast<size_t>(n)});
			  }
		  });

		// #################
		// #################


		std::this_thread::sleep_for(std::chrono::seconds(1));
		sender.request_stop();
		receiver.request_stop();

		dbg::println(
		  "Sent {} messages, received {} messages - {} bytes / {} bytes",
		  sent_bytes,
		  recv_bytes,
		  sent_bytes * 1024,
		  recv_bytes * 1024);


		::shutdown(client, SD_BOTH);
		::shutdown(accepted, SD_BOTH);
		::closesocket(client);
		::closesocket(accepted);
		::closesocket(server);
	}


	DNSQuery query("api.taboobuilder.com");

	// query.set_dns_server("198.41.0.4", 4);
	// query.set_dns_server("2001:503:ba3e::2:30", 6);

	if (auto result = query.send_query(); not result)
		dbg::println("{}", result.error());


	//
	ULONG                 outsize    = 0;
	PIP_ADAPTER_ADDRESSES pHead      = nullptr; // original allocation — used for FREE
	PIP_ADAPTER_ADDRESSES pAddresses = nullptr;


	DWORD dw = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &outsize);
	dbg::println("Buffer needs to be {} bytes", outsize);

	if (dw == ERROR_BUFFER_OVERFLOW)
	{
		dbg::println("Buffer overflow, allocating {} bytes", outsize);

		pHead      = (IP_ADAPTER_ADDRESSES*)MALLOC(outsize);
		pAddresses = pHead;

		dw = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outsize);
		if (dw != ERROR_SUCCESS)
		{
			dbg::println("GetAdaptersAddresses failed with error: {}", dw);
			FREE(pHead);
			return 0;
		}
	}

	// get MTU size


	// Capture the local IP of the first active tunnel/VPN adapter found
	std::optional<sockaddr_in> tunnel_local_addr;
	std::optional<sockaddr_in> non_tunnel_local_addr;

	while (pAddresses->Next != nullptr)
	{
		auto friendly_name = platform::string_from_wide(pAddresses->FriendlyName);
		dbg::println("Adapter: {}", friendly_name);

		if (pAddresses->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
		{
			dbg::println("Software loopback, skipping...");
			pAddresses = pAddresses->Next;
			continue;
		}


		const bool mtu_unlimited = (pAddresses->Mtu == std::numeric_limits<ULONG>::max());
		if (mtu_unlimited)
			dbg::println("Mtu: unlimited");
		else
			dbg::println("Mtu: {}", pAddresses->Mtu);

		dbg::println("Adapter: {}", pAddresses->AdapterName);
		dbg::println("Description: {}", platform::string_from_wide(pAddresses->Description));


		auto address_to_string = [](auto address) -> std::string
		{
			if (address == nullptr)
				return "<none>";

			char unicast_str[INET_ADDRSTRLEN]{};
			inet_ntop(AF_INET, address, unicast_str, sizeof(unicast_str));
			return std::string{unicast_str};
		};


		//
		while (pAddresses->FirstUnicastAddress and pAddresses->FirstUnicastAddress->Address.lpSockaddr != nullptr)
		{
			dbg::println("Unicast address: {}", address_to_string(&pAddresses->FirstUnicastAddress->Address.lpSockaddr));
			pAddresses->FirstUnicastAddress = pAddresses->FirstUnicastAddress->Next;
		}

		//
		while (pAddresses->FirstAnycastAddress and pAddresses->FirstAnycastAddress->Address.lpSockaddr != nullptr)
		{
			dbg::println("Anycast address: {}", address_to_string(&pAddresses->FirstAnycastAddress->Address.lpSockaddr));
			pAddresses->FirstAnycastAddress = pAddresses->FirstAnycastAddress->Next;
		}


		//
		while (pAddresses->FirstMulticastAddress and pAddresses->FirstMulticastAddress->Address.lpSockaddr != nullptr)
		{
			dbg::println("FirstMulticastAddress address: {}",
						 address_to_string(&pAddresses->FirstMulticastAddress->Address.lpSockaddr));
			pAddresses->FirstMulticastAddress = pAddresses->FirstMulticastAddress->Next;
		}

		//
		while (pAddresses->FirstDnsServerAddress and pAddresses->FirstDnsServerAddress->Address.lpSockaddr != nullptr)
		{
			dbg::println("FirstDnsServerAddress address: {}",
						 address_to_string(&pAddresses->FirstDnsServerAddress->Address.lpSockaddr));
			pAddresses->FirstDnsServerAddress = pAddresses->FirstDnsServerAddress->Next;
		}


		bool has_mac = pAddresses->PhysicalAddressLength > 0;
		if (has_mac)
		{
			dbg::println(
			  "Physical Address: {:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}",
			  pAddresses->PhysicalAddress[0],
			  pAddresses->PhysicalAddress[1],
			  pAddresses->PhysicalAddress[2],
			  pAddresses->PhysicalAddress[3],
			  pAddresses->PhysicalAddress[4],
			  pAddresses->PhysicalAddress[5]);
		}
		else
			dbg::println("No mac");


		dbg::println("Flags: {}", pAddresses->Flags);

		dbg::println(" - DHCP enabled: {}", (pAddresses->Flags & IP_ADAPTER_DHCP_ENABLED) != 0);
		dbg::println(" - Registered: {}", (pAddresses->Flags & IP_ADAPTER_REGISTER_ADAPTER_SUFFIX) != 0);
		dbg::println(" - Dhcpv4 enabled: {}", (pAddresses->Flags & IP_ADAPTER_DHCP_ENABLED) != 0);
		dbg::println(" - Receive-only: {}", (pAddresses->Flags & IP_ADAPTER_RECEIVE_ONLY) != 0);
		dbg::println(" - NoMulticast: {}", (pAddresses->Flags & IP_ADAPTER_NO_MULTICAST) != 0);
		dbg::println(" - IPv6OthersStatefulConfig: {}", (pAddresses->Flags & IP_ADAPTER_IPV6_OTHER_STATEFUL_CONFIG) != 0);
		dbg::println(" - Netbios over Tcpip enabled: {}", (pAddresses->Flags & IP_ADAPTER_NETBIOS_OVER_TCPIP_ENABLED) != 0);
		dbg::println(" - Ipv4 enabled: {}", (pAddresses->Flags & IP_ADAPTER_IPV4_ENABLED) != 0);
		dbg::println(" - Ipv6 enabled: {}", (pAddresses->Flags & IP_ADAPTER_IPV6_ENABLED) != 0);
		dbg::println(" - Ipv6 Managed Address Config: {}", (pAddresses->Flags & IP_ADAPTER_IPV6_MANAGE_ADDRESS_CONFIG) != 0);

		[[maybe_unused]] bool has_ipv4 = (pAddresses->Flags & IP_ADAPTER_IPV4_ENABLED) != 0;
		[[maybe_unused]] bool has_ipv6 = (pAddresses->Flags & IP_ADAPTER_IPV6_ENABLED) != 0;


		dbg::println("IfType: {}", pAddresses->IfType);
		dbg::println("OperStatus: {}", as<u32>(pAddresses->OperStatus));
		dbg::println("Ipv6IfIndex: {}", pAddresses->Ipv6IfIndex);

		bool ipv4_enabled = (pAddresses->Flags & IP_ADAPTER_DHCP_ENABLED) != 0;
		bool ipv6_enabled = (pAddresses->Flags & IP_ADAPTER_IPV6_ENABLED) != 0;

		dbg::println("v4 enabled: {}", ipv4_enabled);
		dbg::println("v6 enabled: {}", ipv6_enabled);

		dbg::println("Connection type: {}", as<u32>(pAddresses->ConnectionType));
		dbg::println("Tunnel type: {}", as<u32>(pAddresses->TunnelType));

		bool is_active = (pAddresses->OperStatus == IfOperStatusUp) and (pAddresses->FirstUnicastAddress != nullptr);
		dbg::println("Active: {}", is_active);

		bool is_tunnel =
		  (pAddresses->IfType == IF_TYPE_TUNNEL)                             // generic encapsulation (6to4, Teredo, ISATAP)
		  or (pAddresses->IfType == IF_TYPE_PPP)                             // PPTP / dial-up VPNs
		  or (pAddresses->IfType == IF_TYPE_PROP_VIRTUAL)                    // TAP-driver VPNs (OpenVPN, WireGuard, ...)
		  or (pAddresses->IfType == IF_TYPE_MPLS_TUNNEL)                     // MPLS tunnels
		  or (pAddresses->IfType == IF_TYPE_PPPMULTILINKBUNDLE)              // PPP multilink
		  or (pAddresses->TunnelType != TUNNEL_TYPE_NONE)                    // explicit tunnel type from OS
		  or (pAddresses->PhysicalAddressLength == 0 and not mtu_unlimited); // no MAC = virtual/tunnel (excludes loopback)
		dbg::println("Tunnel: {}", is_tunnel);

		//
		if (is_tunnel == false and is_active == true and has_mac and not non_tunnel_local_addr.has_value())
		{
			for (auto* ua = pAddresses->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
			{
				if (ua->Address.lpSockaddr->sa_family == AF_INET)
				{
					non_tunnel_local_addr = *reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
					char ip_str[INET_ADDRSTRLEN]{};
					inet_ntop(AF_INET, &non_tunnel_local_addr->sin_addr, ip_str, sizeof(ip_str));
					dbg::println("Non-tunnel local IP captured: {}", ip_str);
					// break;
				}
			}
		}

		// Capture the first active tunnel's IPv4 unicast address for later bind()
		if (is_active and is_tunnel and not tunnel_local_addr.has_value())
		{
			for (auto* ua = pAddresses->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
			{
				if (ua->Address.lpSockaddr->sa_family == AF_INET)
				{
					tunnel_local_addr           = *reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
					tunnel_local_addr->sin_port = 0; // port 0: OS picks the ephemeral source port
					char ip_str[INET_ADDRSTRLEN]{};
					inet_ntop(AF_INET, &tunnel_local_addr->sin_addr, ip_str, sizeof(ip_str));
					dbg::println("Tunnel local IP captured: {}", ip_str);
					break;
				}
			}
		}

		dbg::println();


		pAddresses = pAddresses->Next;
	}


	FREE(pHead);
	pHead = pAddresses = nullptr;

	info("GAA: {}", dw);
	SOCKET tunnel_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#if 1
	if (non_tunnel_local_addr.has_value())
	{
		char ip_str[INET_ADDRSTRLEN]{};
		inet_ntop(AF_INET, &non_tunnel_local_addr->sin_addr, ip_str, sizeof(ip_str));
		dbg::println("Non-tunnel local IP: {}", ip_str);
		if (::bind(tunnel_socket, reinterpret_cast<sockaddr*>(&non_tunnel_local_addr.value()), sizeof(sockaddr_in)) !=
			SOCKET_ERROR)
		{
			dbg::println("Socket bound to non-tunnel interface");
		}
		else
		{
			dbg::println("bind to non-tunnel failed: {}", WSAGetLastError());
			closesocket(tunnel_socket);
			WSACleanup();
			return 4;
		}
	}
#else
	// Send through the tunnel interface: bind the socket to its local IP before connecting
	if (tunnel_local_addr.has_value())
	{
		// socket_old sock(transport::tcp, &ok);
		//  bind() pins this socket to the tunnel interface — all traffic leaves via it
		if (::bind(tunnel_socket, reinterpret_cast<sockaddr*>(&tunnel_local_addr.value()), sizeof(sockaddr_in)) !=
			SOCKET_ERROR)
		{
			dbg::println("Socket bound to tunnel interface");

			// connect() and send() now go through the tunnel
		}
		else
		{
			dbg::println("bind to tunnel failed: {}", WSAGetLastError());
			closesocket(tunnel_socket);
			WSACleanup();
			return 5;
		}
	}

#endif

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return 4;

	timeval timeout{};
	timeout.tv_sec  = 1;
	timeout.tv_usec = 0;

	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, as<const char*>(&timeout), sizeof timeout) == SOCKET_ERROR)
		dbg::println("setsockopt failed: {}", WSAGetLastError());

	// if (bind(s, reinterpret_cast<sockaddr*>(&tunnel_local_addr.value()), sizeof(*tunnel_local_addr)) == SOCKET_ERROR)
	//{
	//	closesocket(s);
	//	WSACleanup();
	//	return 5;
	// }
	//
	addrinfo hints{};
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;


	std::string host = "api.ipify.org";

	addrinfo* res = nullptr;
	if (getaddrinfo(host.c_str(), "80", &hints, &res) != 0 or res == nullptr)
	{
		closesocket(s);
		WSACleanup();
		return 6;
	}

	int rc = connect(s, res->ai_addr, static_cast<int>(res->ai_addrlen));
	freeaddrinfo(res);
	if (rc == SOCKET_ERROR)
	{
		closesocket(s);
		WSACleanup();
		return 7;
	}

	std::string req = std::format("GET / HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n", host);
	if (send(s, req.c_str(), static_cast<int>(req.size()), 0) == SOCKET_ERROR)
	{
		closesocket(s);
		WSACleanup();
		return 8;
	}

	std::array<char, 4096> rx{};
	for (;;)
	{
		int n = recv(s, rx.data(), static_cast<int>(rx.size()), 0);
		if (n <= 0)
			break;
		std::string ret{rx.data(), static_cast<size_t>(n)};
		dbg::println("{}", ret);
	}

	closesocket(s);
	WSACleanup();


	UDPSocket udps;
	TCPSocket tdps;

	udps.send("hello");

	tdps.send("hello");

	info("{}", udps.receive(10));
	info("{}", tdps.receive(20));


	return 0;
}
