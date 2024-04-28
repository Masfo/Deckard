export module deckard.sha256;

import deckard.types;
import deckard.helpers;
import std;

using namespace std::string_view_literals;

namespace deckard::sha256
{

	export enum class Uppercase { Yes, No };

	export class [[nodiscard("You are not using your hash digest.")]] sha256_digest final
	{
	public:
		using Type = u32;

		[[nodiscard("You are not using your hash digest.")]] std::string to_string(const Uppercase uppercase = Uppercase::No) const noexcept
		{
			constexpr static std::array fmt_array{"{:08x}"sv, "{:08X}"sv, "{:016x}"sv, "{:016X}"sv};
			const int                   index = (sizeof(Type) == 4 ? 0 : 2) + (uppercase == Uppercase::No ? 0 : 1);

			return std::vformat(
				std::format("{0}{0}{0}{0}{0}{0}{0}{0}", fmt_array[index]),
				std::make_format_args(binary[0], binary[1], binary[2], binary[3], binary[4], binary[5], binary[6], binary[7]));
		}

		Type operator[](int index) const noexcept { return binary[static_cast<u64>(index)]; }

		bool operator==(const sha256_digest &that) const noexcept { return binary == that.binary; }

		std::array<Type, 8> binary;
	};

	export class sha256 final
	{
	public:
		sha256() { reset(); }

		void reset() noexcept
		{
			m_state      = {0x6A09'E667, 0xBB67'AE85, 0x3C6E'F372, 0xA54F'F53A, 0x510E'527F, 0x9B05'688C, 0x1F83'D9AB, 0x5BE0'CD19};
			m_block      = {};
			m_bitlen     = 0ULL;
			m_blockindex = 0ULL;
		}

		void update(std::string_view data) noexcept { generic_update<const char>(data); }

		void update(std::span<u8> const data) noexcept { generic_update<u8>(data); }

		sha256_digest finalize() noexcept
		{
			sha256_digest ret;

			pad();
			ret.binary = m_state;
			reset();
			return ret;
		}

	private:
		static constexpr u32 CHUNK_SIZE_IN_BITS = 512;
		static constexpr u32 BLOCK_SIZE         = CHUNK_SIZE_IN_BITS / 8;
		static constexpr u32 ROUNDS             = 64;

		template<typename T>
		void generic_update(const std::span<T> data) noexcept
		{
			if (data.empty())
				return;

			for (const auto &i : data)
			{
				m_block[m_blockindex++] = static_cast<u8>(i);
				if (m_blockindex == BLOCK_SIZE)
				{
					transform();

					m_bitlen += CHUNK_SIZE_IN_BITS;
					m_blockindex = 0;
				}
			}
		}

		static u32 choose(u32 e, u32 f, u32 g) noexcept { return (e & f) ^ (~e & g); }

		static u32 majority(u32 a, u32 b, u32 c) noexcept { return (a & b) ^ (a & c) ^ (b & c); }

		static u32 sig0(u32 x) noexcept { return std::rotr(x, 7) ^ std::rotr(x, 18) ^ (x >> 3); }

		static u32 sig1(u32 x) noexcept { return std::rotr(x, 17) ^ std::rotr(x, 19) ^ (x >> 10); }

		void transform() // Process the message in successive 512-bit chunks
		{
			u32                maj{}, S0{}, ch{}, S1{}, temp1{}, temp2{}, w[ROUNDS]{0};
			std::array<u32, 8> state;

			// copy chunk into first 16 words w[0..15] of the message schedule array
			for (u32 i = 0, j = 0; i < 16; i++, j += 4)
				w[i] = load_bigendian<u32>(&m_block[j]);

			for (u32 i = 16; i < ROUNDS; i++)
			{
				u32 s0 = sig0(w[i - 15]);
				u32 s1 = sig1(w[i - 2]);
				w[i]   = w[i - 16] + s0 + w[i - 7] + s1;
			}
			// Initialize working variables to current hash value
			state = m_state;

			//  Compression function main loop
			for (u32 i = 0; i < ROUNDS; i++)
			{

				S1    = std::rotr(state[4], 6) ^ std::rotr(state[4], 11) ^ std::rotr(state[4], 25);
				ch    = choose(state[4], state[5], state[6]);
				temp1 = state[7] + S1 + ch + K[i] + w[i];
				S0    = std::rotr(state[0], 2) ^ std::rotr(state[0], 13) ^ std::rotr(state[0], 22);
				maj   = majority(state[0], state[1], state[2]);
				temp2 = S0 + maj;

				state[7] = state[6];         // h = g
				state[6] = state[5];         // g = f
				state[5] = state[4];         // f = e
				state[4] = state[3] + temp1; // d + temp1
				state[3] = state[2];         // d = c
				state[2] = state[1];         // c = b
				state[1] = state[0];         // b = a
				state[0] = temp1 + temp2;    // a = temp1 + temp2
			}

			// Add the compressed chunk to the current hash value
			for (u32 i = 0; i < 8; i++)
				m_state[i] += state[i];
		}

		void pad()
		{
			u64 i = m_blockindex;

			if (m_blockindex < (BLOCK_SIZE - 8))
			{
				m_block[i++] = 0x80;
				while (i < (BLOCK_SIZE - 8))
					m_block[i++] = 0x00;
			}
			else
			{
				m_block[i++] = 0x80;
				while (i < (BLOCK_SIZE - 8))
					m_block[i++] = 0x00;

				std::fill_n(m_block.begin(), BLOCK_SIZE - 8, 0_u8);
				transform();
			}

			m_bitlen += m_blockindex * 8;
			m_block[63] = as<u8>(m_bitlen >> 0);
			m_block[62] = as<u8>(m_bitlen >> 8);
			m_block[61] = as<u8>(m_bitlen >> 16);
			m_block[60] = as<u8>(m_bitlen >> 24);
			m_block[59] = as<u8>(m_bitlen >> 32);
			m_block[58] = as<u8>(m_bitlen >> 40);
			m_block[57] = as<u8>(m_bitlen >> 48);
			m_block[56] = as<u8>(m_bitlen >> 56);

			transform();
		}

		std::array<u8, BLOCK_SIZE> m_block;
		std::array<u32, 8>         m_state;
		u64                        m_bitlen;
		u32                        m_blockindex;

		static constexpr std::array<u32, 64> K = {
			0x428a'2f98, 0x7137'4491, 0xb5c0'fbcf, 0xe9b5'dba5, 0x3956'c25b, 0x59f1'11f1, 0x923f'82a4, 0xab1c'5ed5,
			0xd807'aa98, 0x1283'5b01, 0x2431'85be, 0x550c'7dc3, 0x72be'5d74, 0x80de'b1fe, 0x9bdc'06a7, 0xc19b'f174,
			0xe49b'69c1, 0xefbe'4786, 0x0fc1'9dc6, 0x240c'a1cc, 0x2de9'2c6f, 0x4a74'84aa, 0x5cb0'a9dc, 0x76f9'88da,
			0x983e'5152, 0xa831'c66d, 0xb003'27c8, 0xbf59'7fc7, 0xc6e0'0bf3, 0xd5a7'9147, 0x06ca'6351, 0x1429'2967,
			0x27b7'0a85, 0x2e1b'2138, 0x4d2c'6dfc, 0x5338'0d13, 0x650a'7354, 0x766a'0abb, 0x81c2'c92e, 0x9272'2c85,
			0xa2bf'e8a1, 0xa81a'664b, 0xc24b'8b70, 0xc76c'51a3, 0xd192'e819, 0xd699'0624, 0xf40e'3585, 0x106a'a070,
			0x19a4'c116, 0x1e37'6c08, 0x2748'774c, 0x34b0'bcb5, 0x391c'0cb3, 0x4ed8'aa4a, 0x5b9c'ca4f, 0x682e'6ff3,
			0x748f'82ee, 0x78a5'636f, 0x84c8'7814, 0x8cc7'0208, 0x90be'fffa, 0xa450'6ceb, 0xbef9'a3f7, 0xc671'78f2};
	};

	export sha256_digest hash(std::string_view input)
	{
		sha256 hasher;
		hasher.update({(u8 *)input.data(), input.size()});
		return hasher.finalize();
	}

	static_assert(sizeof(sha256) == 112);


} // namespace deckard::sha256
