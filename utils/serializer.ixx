export module deckard.serializer;

import std;
import deckard.types;

namespace deckard
{
	class serializer
	{
	public:
		serializer()
			: bytelength{0}
			, byte_remaining{0}
		{
			buffer.reserve(128);
		}

		template<typename T>
		void write_bits(const T input, u32 bits)
		{
			//
		}

		// unsigned
		void write(u8 input) { write_bits<u32>(input, sizeof(u8)); }

		void write(u16 input) { write_bits<u16>(input, sizeof(u16)); }

		void write(u32 input) { write_bits<u32>(input, sizeof(u32)); }

		void write(u64 input) { write_bits<u64>(input, sizeof(u64)); }

		// signed
		void write(i8 input) { write_bits<i8>(input, sizeof(i8)); }

		void write(i16 input) { write_bits<i16>(input, sizeof(i16)); }

		void write(i32 input) { write_bits<i32>(input, sizeof(i32)); }

		void write(i64 input) { write_bits<i64>(input, sizeof(i64)); }

		// floats
		void write(f32 input) { write_bits<f32>(input, sizeof(f32)); }

		void write(f64 input) { write_bits<f64>(input, sizeof(f64)); }


	private:
		std::vector<byte> buffer;

		u8  byte_remaining{0};
		u32 bytelength{0};
	};

} // namespace deckard
