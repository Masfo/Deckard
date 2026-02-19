#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.monocypher;
import deckard.helpers;

using namespace deckard;
using namespace deckard::monocypher;

TEST_CASE("monocypher", "[monocypher]")
{
	SECTION("crypto_verify")
	{
		std::array<u8, 8> a{0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70};
		std::array<u8, 8> b{0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70};

		std::array<u8, 8> c{0x00, 0x10, 0x20, 0xFF, 0x40, 0x50, 0x60, 0x70};
		//                                    xxxx

		CHECK(0 == crypto_verify<8>(a, b));
		CHECK(0 == crypto_verify<8>(b, a));

		CHECK(0 != crypto_verify<8>(a, c));
		CHECK(0 != crypto_verify<8>(b, c));


		std::vector<u8> d{0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70};
		std::vector<u8> e{0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70};

		std::vector<u8> f{0x00, 0x10, 0x20, 0xFF, 0x40, 0x50, 0x60, 0x70};
		//                                  xxxx

		CHECK(0 == crypto_verify<u8>(d, e));
		CHECK(0 == crypto_verify<u8>(e, d));

		CHECK(0 != crypto_verify<u8>(d, f));
		CHECK(0 != crypto_verify<u8>(e, f));

		std::vector<u32> g{0x0000'0000, 0x0000'0010, 0x0000'0020, 0x0000'0030, 0x0000'0040, 0x0000'0050, 0x0000'0060, 0x0000'0070};
		std::vector<u32> h{0x0000'0000, 0x0000'0010, 0x0000'0020, 0x0000'0030, 0x0000'0040, 0x0000'0050, 0x0000'0060, 0x0000'0070};

		std::vector<u32> i{0x0000'0000, 0x0000'0010, 0x0000'0020, 0x0000'00FF, 0x0000'0040, 0x0000'0050, 0x0000'0060, 0x0000'0070};
		//                                                        xxxxxxxxxxx
		CHECK(0 == crypto_verify<u32>(g, h));
		CHECK(0 == crypto_verify<u32>(h, g));
		CHECK(0 != crypto_verify<u32>(g, i));
		CHECK(0 != crypto_verify<u32>(h, i));
	}

	SECTION("create sharedkeys")
	{
		const privatekey emptykey;

		publickey  alice_publickey;
		privatekey alice_privatekey;
		sharedkey  alice_sharedkey;

		publickey  bob_publickey;
		privatekey bob_privatekey;
		sharedkey  bob_sharedkey;

		create_private_and_public_keys(alice_publickey, alice_privatekey);
		create_private_and_public_keys(bob_publickey, bob_privatekey);

		create_shared_key(alice_sharedkey, alice_privatekey, bob_publickey);
		create_shared_key(bob_sharedkey, bob_privatekey, alice_publickey);

		CHECK(emptykey == alice_privatekey);
		CHECK(emptykey == bob_privatekey);

		CHECK(alice_sharedkey == bob_sharedkey);

		const sharedkey  empty_sharedkey;
		const sessionkey empty_sessionkey;

		auto alice_k0 = create_session_key(alice_sharedkey, alice_publickey, bob_publickey, 0);
		auto bob_k0   = create_session_key(bob_sharedkey, bob_publickey, alice_publickey, 0);
		CHECK(alice_k0 == bob_k0);
		CHECK(alice_k0 != empty_sessionkey);
		CHECK(bob_k0 != empty_sessionkey);

		auto alice_k1 = create_session_key(alice_sharedkey, alice_publickey, bob_publickey, 1);
		auto bob_k1   = create_session_key(bob_sharedkey, bob_publickey, alice_publickey, 1);
		CHECK(alice_k1 == bob_k1);
		CHECK(alice_k1 != empty_sessionkey);
		CHECK(bob_k1 != empty_sessionkey);
		CHECK(alice_k1 != alice_k0);

		wipe(alice_sharedkey);
		wipe(bob_sharedkey);
		CHECK(alice_sharedkey == empty_sharedkey);
		CHECK(bob_sharedkey == empty_sharedkey);

		SECTION("counter based")
		{
			publickey  alice_publickey2;
			privatekey alice_privatekey2;
			publickey  bob_publickey2;
			privatekey bob_privatekey2;

			create_private_and_public_keys(alice_publickey2, alice_privatekey2);
			create_private_and_public_keys(bob_publickey2, bob_privatekey2);

			sharedkey alice_sharedkey2;
			sharedkey bob_sharedkey2;

			create_shared_key(alice_sharedkey2, alice_privatekey2, bob_publickey2);
			create_shared_key(bob_sharedkey2, bob_privatekey2, alice_publickey2);

			auto k0_a = create_session_key(alice_sharedkey2, alice_publickey2, bob_publickey2, 0);
			auto k0_b = create_session_key(bob_sharedkey2, bob_publickey2, alice_publickey2, 0);
			CHECK(k0_a == k0_b);
			CHECK(k0_a != empty_sessionkey);

			auto k1_a = create_session_key(alice_sharedkey2, alice_publickey2, bob_publickey2, 1);
			auto k1_b = create_session_key(bob_sharedkey2, bob_publickey2, alice_publickey2, 1);
			CHECK(k1_a == k1_b);
			CHECK(k1_a != k0_a);

			wipe(alice_sharedkey2);
			wipe(bob_sharedkey2);
			CHECK(alice_sharedkey2 == empty_sharedkey);
			CHECK(bob_sharedkey2 == empty_sharedkey);
		}
	}

	SECTION("encrypt/decrypt")
	{
		std::array<u8, 16> data{};
		for (int i = 0; i < 16; ++i)
			data[i] = static_cast<u8>(i);

		std::array<u8, 16> cipher{};
		key   k = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
		nonce n = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
		mac   m{};

		SECTION("empty message")
		{
			const std::array<u8, 16> correct_cipher_empty{
			  0x95, 0x25, 0xd2, 0x6c, 0xc9, 0x1d, 0x07, 0x46, 0xf7, 0x7c, 0xfd, 0x82, 0x15, 0x09, 0x0d, 0x3c};

			const mac correct_empty_mac{0x8b, 0x91, 0x3f, 0xf3, 0x8c, 0x04, 0x87, 0x40, 0x7c, 0x76, 0xe1, 0xc2, 0xb6, 0x71, 0x10, 0x5f};

			m = encrypt(k, n, {}, std::array<u8, 16>{}, cipher);
			CHECK(cipher == correct_cipher_empty);
			CHECK(m == correct_empty_mac);
		}


		m = encrypt(k, n, {}, data, cipher);

		const std::array<u8, 16> correct_cipher{
		  {0x95, 0x24, 0xd0, 0x6f, 0xcd, 0x18, 0x01, 0x41, 0xff, 0x75, 0xf7, 0x89, 0x19, 0x04, 0x03, 0x33}};

		mac correct_mac{0x60, 0x3b, 0xeb, 0xcf, 0xdb, 0xc7, 0xda, 0x53, 0xb9, 0x2a, 0x4e, 0x71, 0x55, 0xd7, 0x90, 0xe1};

		CHECK(cipher == correct_cipher);
		CHECK(m == correct_mac);
		CHECK(cipher != data);

		// decrypt with correct key and nonce
		std::array<u8, 16> decrypted_data{};
		auto               result = decrypt(k, n, {}, m, cipher, decrypted_data);
		CHECK(result.has_value());
		CHECK(decrypted_data == data);


		const std::array<u8, 16> emptydata{};

		SECTION("decrypt with wrong key/nonce/mac")
		{
			// wrong key
			decrypted_data = {};
			auto kc        = k;
			kc.data[0] ^= 0xFF;
			result = decrypt(kc, n, {}, m, cipher, decrypted_data);
			CHECK(not result.has_value());
			CHECK(decrypted_data == emptydata);

			// wrong nonce
			decrypted_data = {};
			auto nc        = n;
			nc.data[0] ^= 0xFF;
			result = decrypt(k, nc, {}, m, cipher, decrypted_data);
			CHECK(not result.has_value());
			CHECK(decrypted_data == emptydata);

			// wrong mac
			decrypted_data = {};
			auto mc        = m;
			mc.data[0] ^= 0xFF;
			result = decrypt(k, n, {}, mc, cipher, decrypted_data);
			CHECK(not result.has_value());
			CHECK(decrypted_data == emptydata);

			// wrong mac (byte > 0)
			decrypted_data = {};
			mc             = m;
			mc.data[15] ^= 0xFF;
			result = decrypt(k, n, {}, mc, cipher, decrypted_data);
			CHECK(not result.has_value());
			CHECK(decrypted_data == emptydata);


			// modified cipher
			auto cipher_copy = cipher;
			cipher_copy[0] ^= 0xFF;
			decrypted_data = {};
			result         = decrypt(k, n, {}, m, cipher_copy, decrypted_data);
			CHECK(not result.has_value());
			CHECK(decrypted_data == emptydata);

			cipher_copy = cipher;
			cipher_copy[7] ^= 0xFF;
			decrypted_data = {};
			result         = decrypt(k, n, {}, m, cipher_copy, decrypted_data);
			CHECK(not result.has_value());
			CHECK(decrypted_data == emptydata);

			cipher_copy = cipher;
			cipher_copy[15] ^= 0xFF;
			decrypted_data = {};
			result         = decrypt(k, n, {}, m, cipher_copy, decrypted_data);
			CHECK(not result.has_value());
			CHECK(decrypted_data == emptydata);
		}

		SECTION("associated data")
		{
			std::array<u8, 8> ad{0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7};
			std::fill(cipher.begin(), cipher.end(), 0_u8);
			m = encrypt(k, n, std::span<u8>{ad}, data, cipher);

			std::array<u8, 16> out{};
			result = decrypt(k, n, std::span<u8>{ad}, m, cipher, out);
			CHECK(result.has_value());
			CHECK(out == data);

			// wrong AD
			auto ad2 = ad;
			ad2[3] ^= 0xFF;
			out    = {};
			result = decrypt(k, n, std::span<u8>{ad2}, m, cipher, out);
			CHECK(not result.has_value());
			CHECK(out == emptydata);
		}

		SECTION("different nonce yields different output")
		{
			std::array<u8, 16> cipher2{};
			auto               n2 = n;
			n2.data[0] ^= 0x01;
			auto m2 = encrypt(k, n2, {}, data, cipher2);

			CHECK(cipher2 != cipher);
			CHECK(not(m2 == m));
		}

		SECTION("wipe keys/data")
		{
			key   empty_key;
			nonce empty_nonce;
			mac   empty_mac;
			empty_key.clear();
			empty_nonce.clear();
			empty_mac.clear();

			key   k4;
			nonce n4;
			mac   m4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

			CHECK(not k4.empty());
			CHECK(not n4.empty());
			CHECK(not m4.empty());

			wipe(k4);
			wipe(n4);
			wipe(m4);

			CHECK(k4.empty());
			CHECK(n4.empty());
			CHECK(m4.empty());

			// data
			const std::array<u8, 8> empty_data{};
			std::array<u8, 8>       data4{0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70};
			std::vector<u8>         data4_vec = make_vector<u8>(0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70);


			CHECK(false == crypto_verify<u8>(data4, 0));
			CHECK(false == crypto_verify<u8>(data4_vec, 0));

			wipe(data4);
			wipe(data4_vec);

			CHECK(true == crypto_verify<u8>(data4, 0));
			CHECK(true == crypto_verify<u8>(data4_vec, 0));
		}

		SECTION("longer test")
		{
			const key k1{
			  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
			  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
			};

			const nonce n1{0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b,
						   0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57};

			const std::array<u8, 12> ad{0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7};


			std::array<u8, 114> plaintext{
			  0x4c, 0x61, 0x64, 0x69, 0x65, 0x73, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x47, 0x65, 0x6e, 0x74, 0x6c, 0x65, 0x6d, 0x65,
			  0x6e, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x20, 0x6f, 0x66, 0x20, 0x27,
			  0x39, 0x39, 0x3a, 0x20, 0x49, 0x66, 0x20, 0x49, 0x20, 0x63, 0x6f, 0x75, 0x6c, 0x64, 0x20, 0x6f, 0x66, 0x66, 0x65,
			  0x72, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x6f, 0x6e, 0x6c, 0x79, 0x20, 0x6f, 0x6e, 0x65, 0x20, 0x74, 0x69, 0x70, 0x20,
			  0x66, 0x6f, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x74, 0x75, 0x72, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x6e,
			  0x73, 0x63, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x77, 0x6f, 0x75, 0x6c, 0x64, 0x20, 0x62, 0x65, 0x20, 0x69, 0x74, 0x2e};

			std::array<u8, 114> out{};
			auto                mac1 = encrypt(k1, n1, ad, plaintext, out);

			std::array<u8, 114> decrypted{};
			CHECK(decrypt(k1, n1, ad, mac1, out, decrypted).has_value());

			CHECK(plaintext == decrypted);
		}
	}
}
