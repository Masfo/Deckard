export module deckard.serializer;

import std;
import deckard.types;

namespace deckard
{
	export class serializer
	{
	public:
		serializer()
			: bitlen{0}
		{
			buffer.reserve(128);
		}

		template<typename T>
		void write_bits(const T input, u32 bits)
		{
			const u32 byteindex = bitlen / 8;
			const u32 offset    = bitlen % 8;
			const u32 remainder = 8 - offset;

			if (byteindex > buffer.size())
				buffer.resize(byteindex * 2);

			bitlen += bits;
		}

		// TODO: option to automatically pad bits to byte alignment
		//	- push 2 bits, pad 6

		// unsigned
		void write(u8 input) { write_bits<u8>(input, 8 * sizeof(u8)); }

		void write(u16 input) { write_bits<u16>(input, 8 * sizeof(u16)); }

		void write(u32 input) { write_bits<u32>(input, 8 * sizeof(u32)); }

		void write(u64 input) { write_bits<u64>(input, 8 * sizeof(u64)); }

		// signed
		void write(i8 input) { write_bits<i8>(input, 8 * sizeof(i8)); }

		void write(i16 input) { write_bits<i16>(input, 8 * sizeof(i16)); }

		void write(i32 input) { write_bits<i32>(input, 8 * sizeof(i32)); }

		void write(i64 input) { write_bits<i64>(input, 8 * sizeof(i64)); }

		// floats
		void write(f32 input) { write_bits<f32>(input, 8 * sizeof(f32)); }

		void write(f64 input) { write_bits<f64>(input, 8 * sizeof(f64)); }

		// string
		void write(std::string_view input)
		{
			for (const char c : input)
				write_bits<char>(c, 8);
		}


	private:
		std::vector<byte> buffer;

		u32 bitlen{0};
	};

} // namespace deckard
