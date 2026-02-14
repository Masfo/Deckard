export module deckard.utf8:utf8_span;

import std;
import deckard.types;
import deckard.as;

export namespace deckard::utf8
{
    template<size_t N>
	[[nodiscard]] constexpr auto as_ro_bytes(const std::array<u8, N>& a) -> std::span<const std::byte>
	{
		auto first = as<const std::byte*>(a.data());
		return std::span<const std::byte>(first, first + N);
	}

	template<size_t N>
	[[nodiscard]] constexpr auto as_ro_bytes(std::array<u8, N>& a) -> std::span<const std::byte>
	{
		return as_ro_bytes(static_cast<const std::array<u8, N>&>(a));
	}

 [[nodiscard]] constexpr auto as_ro_bytes(std::span<u8> s) -> std::span<const std::byte>
	{
		auto first = as<const std::byte*>(s.data());
		return std::span<const std::byte>(first, first + s.size());
	}

	[[nodiscard]] constexpr auto as_ro_bytes(std::span<const u8> s) -> std::span<const std::byte>
	{
		auto first = as<const std::byte*>(s.data());
		return std::span<const std::byte>(first, first + s.size());
	}

	[[nodiscard]] constexpr auto as_ro_bytes(std::string_view s) -> std::span<const std::byte>
	{
		auto first = as<const std::byte*>(s.data());
		return std::span<const std::byte>(first, first + s.size());
	}

	[[nodiscard]] constexpr auto as_ro_bytes(const char* s, u32 len) -> std::span<const std::byte>
	{
		auto first = as<const std::byte*>(s);
		return std::span<const std::byte>(first, first + static_cast<size_t>(len));
	}

 [[nodiscard]] constexpr auto u8_at(std::span<const std::byte> s, size_t i) -> u8
	{
		return static_cast<u8>(s[i]);
	}

	[[nodiscard]] constexpr auto u8_data(std::span<const std::byte> s) -> const u8*
	{
		return as<const u8*>(s.data());
	}
}
