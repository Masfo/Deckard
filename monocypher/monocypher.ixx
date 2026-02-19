module;
#include <monocypher.h>
#include <optional/monocypher-ed25519.h>

export module deckard.monocypher;
import std;
import deckard.types;
import deckard.random;
import deckard.helpers;
import deckard.assert;

namespace deckard::monocypher
{
	// TODO: easy to use helpers

	export template<std::integral T>
	constexpr bool crypto_verify(std::span<const T> a, const T value) noexcept
	{
		using Acc = std::make_unsigned_t<std::conditional_t<(sizeof(T) < 4), u32, T>>;
		Acc diff  = 0;

		for (size_t i = 0; i < a.size(); ++i)
			diff |= static_cast<Acc>(a[i] ^ value);

		return diff == 0;
	}

	export template<std::integral T, size_t N>
	constexpr int crypto_verify(std::span<const T, N> a, std::span<const T, N> b) noexcept
	{
		using Acc = std::make_unsigned_t<std::conditional_t<(sizeof(T) < 4), u32, T>>;
		Acc diff  = 0;

		for (size_t i = 0; i < N; ++i)
			diff |= static_cast<Acc>(a[i] ^ b[i]);

		return static_cast<int>(diff != 0);
	}

	export template<size_t N>
	constexpr int crypto_verify(std::span<const u8, N> a, std::span<const u8, N> b) noexcept
	{
		return crypto_verify<u8, N>(a, b);
	}

	export template<typename T = u8>
	constexpr int crypto_verify(std::span<const T> a, std::span<const T> b) noexcept
	{
		assert::check(a.size() == b.size(), "crypto_verify: input spans must have the same size");
		using Acc = std::make_unsigned_t<std::conditional_t<(sizeof(T) < 4), u32, T>>;

		Acc diff = 0;
		for (size_t i = 0; i < a.size(); ++i)
			diff |= static_cast<Acc>(a[i] ^ b[i]);

		return static_cast<int>(diff != 0);
	}

	export struct [[nodiscard]] key
	{
		std::array<u8, 32> data{};

		key() { random::cryptographic_random_bytes(data); }

		key(std::initializer_list<u8> init)
		{
			data.fill(0);
			std::copy(init.begin(), init.end(), data.begin());
		}

		bool empty() const { return crypto_verify<u8>(data, 0); }

		void clear() { crypto_wipe(data.data(), data.size()); }

		bool operator==(const key& rhs) const { return crypto_verify<32>(data, rhs.data) == 0; }
	};

	export struct [[nodiscard]] nonce
	{
		std::array<u8, 24> data{};

		nonce() { random::cryptographic_random_bytes(data); }

		nonce(std::initializer_list<u8> init)
		{
			data.fill(0);
			std::copy(init.begin(), init.end(), data.begin());
		}

		bool empty() const { return crypto_verify<u8>(data, 0); }

		void clear() { crypto_wipe(data.data(), data.size()); }

		bool operator==(const nonce& rhs) const { return crypto_verify<24>(data, rhs.data) == 0; }
	};

	export struct [[nodiscard("Use the MAC")]] mac
	{
		std::array<u8, 16> data{};
		mac() = default;

		mac(std::initializer_list<u8> init) { std::copy(init.begin(), init.end(), data.begin()); }

		bool empty() const { return crypto_verify<u8>(data, 0); }

		void clear() { crypto_wipe(data.data(), data.size()); }

		bool operator==(const mac& rhs) const { return crypto_verify<16>(data, rhs.data) == 0; }
	};

	export void wipe(key& k) { crypto_wipe(k.data.data(), k.data.size()); }

	export void wipe(nonce& n) { crypto_wipe(n.data.data(), n.data.size()); }

	export void wipe(mac& m) { crypto_wipe(m.data.data(), m.data.size()); }

	// ##########################


	export struct [[nodiscard]] publickey
	{
		std::array<u8, 32> data{};

		bool empty() const { return crypto_verify<u8>(data, 0); }

		void clear() { crypto_wipe(data.data(), data.size()); }

		bool operator==(const publickey& rhs) const { return crypto_verify32(data.data(), rhs.data.data()) == 0; }
	};

	export struct [[nodiscard]] privatekey
	{
		std::array<u8, 32> data{};

		bool empty() const { return crypto_verify<u8>(data, 0); }

		void clear() { crypto_wipe(data.data(), data.size()); }

		bool operator==(const privatekey& rhs) const { return crypto_verify32(data.data(), rhs.data.data()) == 0; }
	};

	export struct [[nodiscard("")]] sharedkey
	{
		std::array<u8, 32> data{};

		bool empty() const { return crypto_verify<u8>(data, 0); }

		void clear() { crypto_wipe(data.data(), data.size()); }

		bool operator==(const sharedkey& rhs) const { return crypto_verify32(data.data(), rhs.data.data()) == 0; }
	};

	export struct [[nodiscard]] sessionkey
	{
		std::array<u8, 32> data{};

		bool operator==(const sessionkey& rhs) const { return crypto_verify32(data.data(), rhs.data.data()) == 0; }
	};

	export template<typename T>
	concept KeyType =
	  std::is_same_v<std::remove_cvref_t<T>, privatekey> or std::is_same_v<std::remove_cvref_t<T>, sharedkey> or
	  std::is_same_v<std::remove_cvref_t<T>, publickey> or std::is_same_v<std::remove_cvref_t<T>, sessionkey>;

	export template<KeyType T>
	void wipe(T& key)
	{
		crypto_wipe(key.data.data(), key.data.size());
	}

	void initialize_privatekey(privatekey& privkey) { random::cryptographic_random_bytes(privkey.data); }

	export void wipe(std::span<u8> data) { crypto_wipe(static_cast<u8*>(data.data()), data.size_bytes()); }

	export void create_private_and_public_keys(publickey& pubkey, privatekey& privkey)
	{
		initialize_privatekey(privkey);

		crypto_x25519_public_key(pubkey.data.data(), privkey.data.data());
	}

	export void create_shared_key(sharedkey& skey, privatekey& your_private_key, const publickey& their_public_key)
	{
		crypto_x25519(skey.data.data(), your_private_key.data.data(), their_public_key.data.data());

		wipe(your_private_key);
	}

	export [[nodiscard]] sessionkey
	create_session_key(const sharedkey& shared_key, const publickey& your_public_key, const publickey& their_public_key, u32 counter)
	{
		std::array<u8, 32> out;
		std::array<u8, 4>  ctr{};
		write_le(std::span<u8>{ctr}, 0, counter);

		// Sort public keys to ensure both parties derive the same key for the same counter
		const publickey* p1 = &your_public_key;
		const publickey* p2 = &their_public_key;
		if (crypto_verify<32>(p1->data, p2->data) > 0 and std::memcmp(p1->data.data(), p2->data.data(), p1->data.size()) > 0)
		{
			std::swap(p1, p2);
		}

		crypto_blake2b_ctx ctx;
		crypto_blake2b_init(&ctx, out.size());
		crypto_blake2b_update(&ctx, shared_key.data.data(), shared_key.data.size());
		crypto_blake2b_update(&ctx, p1->data.data(), p1->data.size());
		crypto_blake2b_update(&ctx, p2->data.data(), p2->data.size());
		crypto_blake2b_update(&ctx, ctr.data(), ctr.size());
		crypto_blake2b_final(&ctx, out.data());

		sessionkey key;
		std::copy(out.begin(), out.end(), key.data.begin());

		crypto_wipe(out.data(), out.size());
		crypto_wipe(ctr.data(), ctr.size());

		return key;
	}

	export void wipe(publickey& k) { crypto_wipe(k.data.data(), k.data.size()); }

	export void wipe(privatekey& k) { crypto_wipe(k.data.data(), k.data.size()); }

	export void wipe(sharedkey& k) { crypto_wipe(k.data.data(), k.data.size()); }

	export void wipe(sessionkey& k) { crypto_wipe(k.data.data(), k.data.size()); }

	// encrypt and decrypt


	//

	export key random_key() { return {}; }

	export nonce random_nonce() { return {}; }

	//
	export [[nodiscard("Encrypt returns a MAC. You need it to decrypt the message.")]] mac encrypt(
	  const key& key, const nonce& nonce, std::optional<std::span<const u8>> ad, const std::span<const u8> input, std::span<u8> output)
	{
		assert::check(input.size() == output.size(), "input and output must be the same size");

		mac mac{};
		crypto_aead_lock(
		  output.data(),
		  mac.data.data(),
		  key.data.data(),
		  nonce.data.data(),
		  ad ? ad->data() : nullptr,
		  ad ? ad->size() : 0,
		  input.data(),
		  input.size());

		return mac;
	}

	export [[nodiscard("Check if decrypt failed")]] std::expected<void, std::string>
	decrypt(const key& key, const nonce& nonce, std::optional<std::span<const u8>> ad, const mac& mac, const std::span<const u8> input,
			std::span<u8> output)
	{
		assert::check(input.size() == output.size(), "input and output must be the same size");

		int result = crypto_aead_unlock(
		  output.data(),
		  mac.data.data(),
		  key.data.data(),
		  nonce.data.data(),
		  ad ? ad->data() : nullptr,
		  ad ? ad->size() : 0,
		  input.data(),
		  input.size());
		if (result != 0)
			return std::unexpected("Decryption failed: authentication failed");

		return {};
	}


} // namespace deckard::monocypher
