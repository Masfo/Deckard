export module deckard.uuid;

import std;
import deckard.types;
import deckard.helpers;

namespace deckard::uuid
{
	static std::random_device                 rd;
	static std::uniform_int_distribution<u64> dist(0);

	using namespace std::string_view_literals;

	namespace v4
	{

		export struct uuid
		{
			u64 ab{};
			u64 cd{};
		};

		export uuid generate()
		{
			return {(dist(rd) & 0xFFFF'FFFF'FFFF'0FFFULL) | 0x0000'0000'0000'4000ULL,
					(dist(rd) & 0x3FFF'FFFF'FFFF'FFFFULL) | 0x8000'0000'0000'0000ULL};
		}

		export std::string to_string(uuid id, bool uppercase)
		{
			if (uppercase)
				return std::format(
				  "{:08X}-{:04X}-{:04X}-{:04X}-{:012X}"sv,
				  (id.ab >> 32) & 0xFFFF'FFFF,
				  (id.ab >> 16) & 0xFFFF,
				  (id.ab >> 00) & 0xFFFF,
				  (id.cd >> 48) & 0xFFFF,
				  (id.cd >> 00) & 0xFFFF'FFFF'FFFF);

			return std::format(
			  "{:08x}-{:04x}-{:04x}-{:04x}-{:012x}"sv,
			  (id.ab >> 32) & 0xFFFF'FFFF,
			  (id.ab >> 16) & 0xFFFF,
			  (id.ab >> 00) & 0xFFFF,
			  (id.cd >> 48) & 0xFFFF,
			  (id.cd >> 00) & 0xFFFF'FFFF'FFFF);
		}

	} // namespace v4

	namespace v7
	{
		export struct uuid
		{
			u64 ab{};
			u64 cd{};
		};

		export uuid generate()
		{
			u64 ab = ((deckard::epoch<std::chrono::milliseconds>() << 16) & 0xFFFF'FFFF'FFFF'0FFFULL) | 0x0000'0000'0000'7000ULL;
			ab |= (dist(rd) & 0x0FFF | 0x7000);
			u64 cd = (dist(rd) & 0x3FFF'FFFF'FFFF'FFFFULL) | 0x8000'0000'0000'0000ULL;

			return {ab, cd};
		}

		export std::string to_string(const uuid id, bool uppercase)
		{
			if (uppercase)
				return std::format(
				  "{:08X}-{:04X}-{:04X}-{:04X}-{:012X}"sv,
				  (id.ab >> 32) & 0xFFFF'FFFF,
				  (id.ab >> 16) & 0xFFFF,
				  (id.ab >> 00) & 0xFFFF,
				  (id.cd >> 48) & 0xFFFF,
				  (id.cd >> 00) & 0xFFFF'FFFF'FFFF);

			return std::format(
			  "{:08x}-{:04x}-{:04x}-{:04x}-{:012x}"sv,
			  (id.ab >> 32) & 0xFFFF'FFFF,
			  (id.ab >> 16) & 0xFFFF,
			  (id.ab >> 00) & 0xFFFF,
			  (id.cd >> 48) & 0xFFFF,
			  (id.cd >> 00) & 0xFFFF'FFFF'FFFF);
		}
	} // namespace v7

} // namespace deckard::uuid

namespace std
{
	using namespace deckard::uuid;

	template<>
	struct formatter<v4::uuid>
	{
		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto pos = ctx.begin();
			while (pos != ctx.end() && *pos != '}')
			{
				uppercase_hex = *pos == 'X';
				++pos;
			}
			return pos;
		}

		auto format(const v4::uuid& id, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "{}", v4::to_string(id, uppercase_hex));
		}

		bool uppercase_hex{false};
	};

	template<>
	struct formatter<v7::uuid>
	{
		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto pos = ctx.begin();
			while (pos != ctx.end() && *pos != '}')
			{
				uppercase_hex = *pos == 'X';
				++pos;
			}
			return pos;
		}

		auto format(const v7::uuid& id, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "{}", v7::to_string(id, uppercase_hex));
		}

		bool uppercase_hex{false};
	};
} // namespace std
