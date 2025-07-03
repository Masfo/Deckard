export module deckard.serializer;

import std;
import deckard.types;
import deckard.assert;
import deckard.bitbuffer;

namespace deckard::serializer
{
	// TODO: fixed size reader/writer
	// TODO: append bitreader/writers


	// TODO: bitbuffer, combine reader/writer
	/*
	 *  bb.read(
	 */


		export enum class padding : u8 { yes, no };

		// TODO: name bitfield

		export class serializer
		{
		private:
			std::vector<u8> buffer;
			u64             bitpos{0};
			padding         pad{padding::no};

			void align_to_byte_offset()
			{
				const u64 offset    = bit_offset();
				const u64 remainder = 8 - offset;
				if (pad == padding::yes and offset != 0)
				{
					bitpos += remainder;
				}
			}

			void write_single_bit(u8 bit)
			{
				const u64 byteindex = byte_index();
				const u64 offset    = bit_offset();
				if (offset == 0)
					buffer.push_back(0);
				if (bit)
					buffer[byteindex] |= 1 << (7 - offset);
				bitpos++;
				align_to_byte_offset();
			}

			void write_single_bit(bool bit) { write_single_bit(static_cast<u8>(bit)); }

			u64 byte_index() const { return bitpos / 8u; }

			u64 bit_offset() const { return bitpos % 8u; }

		public:
			serializer()
				: bitpos{0}
				, pad{padding::no}
			{
				buffer.reserve(128);
			}

			serializer(padding p)
				: serializer()
			{
				pad = p;
			}
		};


} // namespace deckard::serializer
