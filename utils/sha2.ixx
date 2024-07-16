export module deckard.sha2;

import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.types;
import deckard.helpers;
import std;

using namespace deckard;


export enum class Uppercase { Yes, No };

template<typename Type>
requires std::is_integral_v<Type>
[[nodiscard("You are not using your hash digest.")]] class generic_sha2_digest final
{
public:
	using type = Type;

	generic_sha2_digest() = default;

	generic_sha2_digest(std::string_view string_hash)
	{

		assert::check(string_hash.size() == binary.size() * sizeof(type) * 2, "Hash parameter not correct size for sha256");
		constexpr size_t wordsize = sizeof(type) * 2;

		u32 count{0};
		for (const auto& word : string_hash | std::views::chunk(wordsize))
		{
			type result{};
			if (std::from_chars(word.data(), word.data() + word.size(), result, 16).ec == std::errc{})
			{
				binary[count++] = result;
			}
			else
			{
				auto ws = word | std::ranges::to<std::string>();
				dbg::println("Cannot convert word '{}' in hash '{}'", ws, string_hash);
				binary.fill(0);
			}
		}
	}

	generic_sha2_digest(const std::initializer_list<Type>& digits)
	{
		assert::check(digits.size() == binary.size(), "Initializer-list must be same size as digest");

		std::copy(digits.begin(), digits.end(), binary.begin());
	};

	[[nodiscard("You are not using your hash digest string.")]]
	std::string to_string(const Uppercase uppercase = Uppercase::No) noexcept
	{
		return to_hex_string<Type>(
		  binary, {.delimiter = "", .endian_swap = true, .lowercase = (uppercase != Uppercase::Yes), .show_hex = false});
	}

	Type operator[](int index) const noexcept
	{
		assert::check(index < binary.size(), "Indexing out-of-bounds");
		return binary[index];
	}

	bool operator==(const generic_sha2_digest<Type>& that) const noexcept { return binary == that.binary; }

	std::array<Type, 8> binary{0};
};

namespace deckard::sha256
{

	export using digest = generic_sha2_digest<u32>;

	static_assert(sizeof(digest) == 32);

	constexpr u32 CHUNK_SIZE_IN_BITS = 512;
	constexpr u32 BLOCK_SIZE         = CHUNK_SIZE_IN_BITS / 8;
	constexpr u32 ROUNDS             = 64;

	export class hasher
	{
	public:
		hasher() { reset(); }

		void reset() noexcept
		{
			m_state      = {0x6A09'E667, 0xBB67'AE85, 0x3C6E'F372, 0xA54F'F53A, 0x510E'527F, 0x9B05'688C, 0x1F83'D9AB, 0x5BE0'CD19};
			m_block      = {};
			m_bitlen     = 0ULL;
			m_blockindex = 0ULL;
		}

		void update(std::string_view data) noexcept { generic_update<const char>(data); }

		void update(std::span<u8> const data) noexcept { generic_update<u8>(data); }

		digest finalize() noexcept
		{
			digest ret;

			pad();
			ret.binary = m_state;
			reset();
			return ret;
		}

	private:
		template<typename T>
		void generic_update(const std::span<T> data) noexcept
		{
			if (data.empty())
				return;

			for (const auto& i : data)
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

		u32 choose(u32 e, u32 f, u32 g) noexcept { return (e & f) ^ (~e & g); }

		u32 majority(u32 a, u32 b, u32 c) noexcept { return (a & b) ^ (a & c) ^ (b & c); }

		u32 sig0(u32 x) noexcept { return std::rotr(x, 7) ^ std::rotr(x, 18) ^ (x >> 3); }

		u32 sig1(u32 x) noexcept { return std::rotr(x, 17) ^ std::rotr(x, 19) ^ (x >> 10); }

		void transform() noexcept
		{
			u32                maj{}, S0{}, ch{}, S1{}, temp1{}, temp2{}, w[ROUNDS]{0};
			std::array<u32, 8> state;

			// copy chunk into first 16 words w[0..15] of the message schedule array
			for (u32 i = 0, j = 0; i < 16; i++, j += 4)
				w[i] = load_as_be<u32>(&m_block[j]);

			for (u32 i = 16; i < ROUNDS; i++)
			{
				u32 s0 = sig0(w[i - 15]);
				u32 s1 = sig1(w[i - 2]);
				w[i]   = w[i - 16] + s0 + w[i - 7] + s1;
			}
			state = m_state;

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
			m_block[63] = as<u8>(m_bitlen >> 0 & 0xFF);
			m_block[62] = as<u8>(m_bitlen >> 8 & 0xFF);
			m_block[61] = as<u8>(m_bitlen >> 16 & 0xFF);
			m_block[60] = as<u8>(m_bitlen >> 24 & 0xFF);
			m_block[59] = as<u8>(m_bitlen >> 32 & 0xFF);
			m_block[58] = as<u8>(m_bitlen >> 40 & 0xFF);
			m_block[57] = as<u8>(m_bitlen >> 48 & 0xFF);
			m_block[56] = as<u8>(m_bitlen >> 56 & 0xFF);

			transform();
		}

		std::array<u8, BLOCK_SIZE> m_block;
		std::array<u32, 8>         m_state;
		u64                        m_bitlen;
		u32                        m_blockindex;

		static constexpr std::array<u32, 64> K = {
		  0x428a'2f98, 0x7137'4491, 0xb5c0'fbcf, 0xe9b5'dba5, 0x3956'c25b, 0x59f1'11f1, 0x923f'82a4, 0xab1c'5ed5, 0xd807'aa98, 0x1283'5b01,
		  0x2431'85be, 0x550c'7dc3, 0x72be'5d74, 0x80de'b1fe, 0x9bdc'06a7, 0xc19b'f174, 0xe49b'69c1, 0xefbe'4786, 0x0fc1'9dc6, 0x240c'a1cc,
		  0x2de9'2c6f, 0x4a74'84aa, 0x5cb0'a9dc, 0x76f9'88da, 0x983e'5152, 0xa831'c66d, 0xb003'27c8, 0xbf59'7fc7, 0xc6e0'0bf3, 0xd5a7'9147,
		  0x06ca'6351, 0x1429'2967, 0x27b7'0a85, 0x2e1b'2138, 0x4d2c'6dfc, 0x5338'0d13, 0x650a'7354, 0x766a'0abb, 0x81c2'c92e, 0x9272'2c85,
		  0xa2bf'e8a1, 0xa81a'664b, 0xc24b'8b70, 0xc76c'51a3, 0xd192'e819, 0xd699'0624, 0xf40e'3585, 0x106a'a070, 0x19a4'c116, 0x1e37'6c08,
		  0x2748'774c, 0x34b0'bcb5, 0x391c'0cb3, 0x4ed8'aa4a, 0x5b9c'ca4f, 0x682e'6ff3, 0x748f'82ee, 0x78a5'636f, 0x84c8'7814, 0x8cc7'0208,
		  0x90be'fffa, 0xa450'6ceb, 0xbef9'a3f7, 0xc671'78f2};
	};

	export std::string quickhash(std::string_view input) noexcept
	{
		sha256::hasher hasher;
		hasher.update({(u8*)input.data(), input.size()});

		sha256::digest digest = hasher.finalize();
		return digest.to_string();
	}

	static_assert(sizeof(hasher) == 112);


} // namespace deckard::sha256

namespace deckard::sha512
{
	export using digest = generic_sha2_digest<u64>;

	static_assert(sizeof(digest) == 64);
	constexpr u32 CHUNK_SIZE_IN_BITS = 1'024;
	constexpr u32 BLOCK_SIZE         = CHUNK_SIZE_IN_BITS / 8;
	constexpr u32 ROUNDS             = 80;

	export class hasher
	{
	public:
		hasher() { reset(); }

		void reset() noexcept
		{
			m_state = {
			  0x6a09'e667'f3bc'c908,
			  0xbb67'ae85'84ca'a73b,
			  0x3c6e'f372'fe94'f82b,
			  0xa54f'f53a'5f1d'36f1,
			  0x510e'527f'ade6'82d1,
			  0x9b05'688c'2b3e'6c1f,
			  0x1f83'd9ab'fb41'bd6b,
			  0x5be0'cd19'137e'2179};
			m_block      = {};
			m_bitlen     = 0ULL;
			m_blockindex = 0ULL;
		}

		void update(std::string_view data) noexcept { generic_update<const char>(data); }

		void update(std::span<u8> const data) noexcept { generic_update<u8>(data); }

		digest finalize() noexcept
		{
			digest ret;

			pad();
			ret.binary = m_state;
			reset();
			return ret;
		}

	private:
		template<typename T>
		void generic_update(const std::span<T> data) noexcept
		{
			if (data.empty())
				return;

			for (const auto& i : data)
			{
				m_block[m_blockindex++] = as<u8>(i & 0xFF);
				if (m_blockindex == BLOCK_SIZE)
				{
					transform();

					m_bitlen += CHUNK_SIZE_IN_BITS;
					m_blockindex = 0;
				}
			}
		}

		u64 choose(u64 e, u64 f, u64 g) noexcept { return (e & f) ^ ((~e) & g); }

		u64 majority(u64 a, u64 b, u64 c) noexcept { return (a & b) ^ (a & c) ^ (b & c); }

		u64 sig0(u64 x) noexcept { return (std::rotr(x, 1) ^ std::rotr(x, 8) ^ (x >> 7)); }

		u64 sig1(u64 x) noexcept { return (std::rotr(x, 19) ^ std::rotr(x, 61) ^ (x >> 6)); }

		void transform() noexcept
		{
			u64                maj{}, S0{}, ch{}, S1{}, temp1{}, temp2{}, w[ROUNDS]{0};
			std::array<u64, 8> state;

			for (u64 i = 0, j = 0; i < 16; i++, j += 8)
				w[i] = load_as_be<u64>(&m_block[j]);


			state = m_state;

			for (u8 i = 0; i < ROUNDS; i++)
			{
				if (i >= 16)
				{
					u64 s0 = sig0(w[i - 15]);
					u64 s1 = sig1(w[i - 2]);
					w[i]   = w[i - 16] + s0 + w[i - 7] + s1;
				}
				S1    = std::rotr(state[4], 14) ^ std::rotr(state[4], 18) ^ std::rotr(state[4], 41);
				ch    = choose(state[4], state[5], state[6]);
				temp1 = state[7] + S1 + ch + K[i] + w[i];
				S0    = std::rotr(state[0], 28) ^ std::rotr(state[0], 34) ^ std::rotr(state[0], 39);
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

				transform();
				std::fill_n(&m_block[0], BLOCK_SIZE - 8, 0_u8);
			}

			m_bitlen += m_blockindex * 8;

			m_block[BLOCK_SIZE - 1] = as<u8>(m_bitlen >> 0 & 0xFF);
			m_block[BLOCK_SIZE - 2] = as<u8>(m_bitlen >> 8 & 0xFF);
			m_block[BLOCK_SIZE - 3] = as<u8>(m_bitlen >> 16 & 0xFF);
			m_block[BLOCK_SIZE - 4] = as<u8>(m_bitlen >> 24 & 0xFF);
			m_block[BLOCK_SIZE - 5] = as<u8>(m_bitlen >> 32 & 0xFF);
			m_block[BLOCK_SIZE - 6] = as<u8>(m_bitlen >> 40 & 0xFF);
			m_block[BLOCK_SIZE - 7] = as<u8>(m_bitlen >> 48 & 0xFF);
			m_block[BLOCK_SIZE - 8] = as<u8>(m_bitlen >> 56 & 0xFF);

			transform();
		}

		std::array<u8, BLOCK_SIZE> m_block;
		std::array<u64, 8>         m_state;
		u64                        m_bitlen;
		u32                        m_blockindex;

		static constexpr std::array<u64, ROUNDS> K = {
		  0x428a'2f98'd728'ae22, 0x7137'4491'23ef'65cd, 0xb5c0'fbcf'ec4d'3b2f, 0xe9b5'dba5'8189'dbbc, 0x3956'c25b'f348'b538,
		  0x59f1'11f1'b605'd019, 0x923f'82a4'af19'4f9b, 0xab1c'5ed5'da6d'8118, 0xd807'aa98'a303'0242, 0x1283'5b01'4570'6fbe,
		  0x2431'85be'4ee4'b28c, 0x550c'7dc3'd5ff'b4e2, 0x72be'5d74'f27b'896f, 0x80de'b1fe'3b16'96b1, 0x9bdc'06a7'25c7'1235,
		  0xc19b'f174'cf69'2694, 0xe49b'69c1'9ef1'4ad2, 0xefbe'4786'384f'25e3, 0x0fc1'9dc6'8b8c'd5b5, 0x240c'a1cc'77ac'9c65,
		  0x2de9'2c6f'592b'0275, 0x4a74'84aa'6ea6'e483, 0x5cb0'a9dc'bd41'fbd4, 0x76f9'88da'8311'53b5, 0x983e'5152'ee66'dfab,
		  0xa831'c66d'2db4'3210, 0xb003'27c8'98fb'213f, 0xbf59'7fc7'beef'0ee4, 0xc6e0'0bf3'3da8'8fc2, 0xd5a7'9147'930a'a725,
		  0x06ca'6351'e003'826f, 0x1429'2967'0a0e'6e70, 0x27b7'0a85'46d2'2ffc, 0x2e1b'2138'5c26'c926, 0x4d2c'6dfc'5ac4'2aed,
		  0x5338'0d13'9d95'b3df, 0x650a'7354'8baf'63de, 0x766a'0abb'3c77'b2a8, 0x81c2'c92e'47ed'aee6, 0x9272'2c85'1482'353b,
		  0xa2bf'e8a1'4cf1'0364, 0xa81a'664b'bc42'3001, 0xc24b'8b70'd0f8'9791, 0xc76c'51a3'0654'be30, 0xd192'e819'd6ef'5218,
		  0xd699'0624'5565'a910, 0xf40e'3585'5771'202a, 0x106a'a070'32bb'd1b8, 0x19a4'c116'b8d2'd0c8, 0x1e37'6c08'5141'ab53,
		  0x2748'774c'df8e'eb99, 0x34b0'bcb5'e19b'48a8, 0x391c'0cb3'c5c9'5a63, 0x4ed8'aa4a'e341'8acb, 0x5b9c'ca4f'7763'e373,
		  0x682e'6ff3'd6b2'b8a3, 0x748f'82ee'5def'b2fc, 0x78a5'636f'4317'2f60, 0x84c8'7814'a1f0'ab72, 0x8cc7'0208'1a64'39ec,
		  0x90be'fffa'2363'1e28, 0xa450'6ceb'de82'bde9, 0xbef9'a3f7'b2c6'7915, 0xc671'78f2'e372'532b, 0xca27'3ece'ea26'619c,
		  0xd186'b8c7'21c0'c207, 0xeada'7dd6'cde0'eb1e, 0xf57d'4f7f'ee6e'd178, 0x06f0'67aa'7217'6fba, 0x0a63'7dc5'a2c8'98a6,
		  0x113f'9804'bef9'0dae, 0x1b71'0b35'131c'471b, 0x28db'77f5'2304'7d84, 0x32ca'ab7b'40c7'2493, 0x3c9e'be0a'15c9'bebc,
		  0x431d'67c4'9c10'0d4c, 0x4cc5'd4be'cb3e'42b6, 0x597f'299c'fc65'7e2a, 0x5fcb'6fab'3ad6'faec, 0x6c44'198c'4a47'5817};
	};

	static_assert(sizeof(hasher) == 208);

	export std::string quickhash(std::string_view input) noexcept
	{
		sha512::hasher hasher;
		hasher.update({(u8*)input.data(), input.size()});

		sha512::digest digest = hasher.finalize();

		return digest.to_string();
	}


} // namespace deckard::sha512
