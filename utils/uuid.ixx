export module deckard.uuid;

import std;
import deckard.types;
import deckard.helpers;

namespace deckard::uuid
{
	static std::random_device                 rd;
	static std::uniform_int_distribution<u64> dist(0);

	namespace v4
	{

		export std::string to_string() noexcept
		{
			u64 ab = (dist(rd) & 0xFFFF'FFFF'FFFF'0FFFULL) | 0x0000'0000'0000'4000ULL;
			u64 cd = (dist(rd) & 0x3FFF'FFFF'FFFF'FFFFULL) | 0x8000'0000'0000'0000ULL;

			return std::format(
			  "{:08X}-{:04X}-{:04X}-{:04X}-{:012X}",
			  (ab >> 32) & 0xFFFF'FFFF,
			  (ab >> 16) & 0xFFFF,
			  (ab >> 00) & 0xFFFF,
			  (cd >> 48) & 0xFFFF,
			  (cd >> 00) & 0xFFFF'FFFF'FFFF);
		}

	} // namespace v4

	namespace v7
	{
		struct id
		{
			u64 epoch : 48;
			u64 var : 4;
			u64 rand_a : 12;
			u64 var2 : 2;
			u64 rand_b : 62;
		};

		static_assert(sizeof(id) == 16);

		export std::string to_string() noexcept
		{
			id ret{
			  .epoch  = deckard::epoch<std::chrono::milliseconds>(),
			  .rand_a = dist(rd),
			  .rand_b = dist(rd),
			};

			return "";
			// 018aab68-d2dd-7xxx-Yzzz-fcc52619c0e3
			// FD9AA744-EB77-7B7D-8709-F6B9A2CFA61F
			// return std::format(
			//  "{:08X}-{:04X}-7{:03X}-{:04X}-{:012X}",
			//  (ret.epoch >> 32) & 0xFFFF'FFFF,
			//  ret.epoch & 0xFFFF,
			//  (ab >> 00) & 0xFFFF,
			//  (cd >> 48) & 0xFFFF,
			//  (cd >> 00) & 0xFFFF'FFFF'FFFF);
		}
	} // namespace v7

} // namespace deckard::uuid
