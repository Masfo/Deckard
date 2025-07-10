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

	export class serializer
	{
	private:
		std::vector<u8> buffer;
		u64             bitpos{0};
		padding         pad{padding::no};

	private:
		u64 byte_index() const { return bitpos / 8u; }

		u64 bit_offset() const { return bitpos % 8u; }

		void align_to_byte_offset()
		{
			const u64 offset    = bit_offset();
			const u64 remainder = 8 - offset;

			if (pad == padding::yes and offset != 0)
			{
				bitpos += remainder;
			}
		}

	private: // Writing

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

		template<typename T, size_t Size = 8 * sizeof(T)>
		void write_bits(const T input, u32 bits = Size)
		{
			const u64 offset = bitpos % 8;
			const u32 bytes  = bits / 8;
			bool      aligns = bits % 8 == 0;


			if (offset == 0 and aligns)
			{
				for (u32 i = 0; i < bytes; i++)
				{
					u8 byte = (input >> ((bits - 8) - i * 8)) & 0xFF;
					write_byte(byte);
				}
				return;
			}

			bool value{};
			for (u32 i = 0; i < bits; i++)
			{
				value = ((input >> (bits - 1 - i)) & 0x1);
				write_single_bit(value);
			}
			return;
		}

		void write_single_bit(bool bit) { write_single_bit(static_cast<u8>(bit)); }

	private: // Reading

		bool read_bit() 
		{
			const u64 offset    = bit_offset();
			const u64 remainder = 7 - offset;

			assert::check(empty(), "Buffer has no more data to read");


			u8 byte   = buffer[byte_index()];
			u8 masked = (byte >> remainder) & 1;

			bitpos += 1;
			align_to_byte_offset();

			return masked;
		}


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

		// Copy
		serializer(serializer const&)            = delete;
		serializer& operator=(serializer const&) = delete;
		// Move
		serializer(serializer&&)            = delete;
		serializer& operator=(serializer&&) = delete;

		void write_byte(u8 byte)
		{
			const u64 offset    = bit_offset();
			const u64 remainder = 8 - offset;

			if (offset == 0)
			{
				buffer.push_back(byte);
			}
			else
			{

				buffer[byte_index()] |= byte >> offset;
				buffer.push_back(byte << remainder);
			}

			bitpos += 8;

			align_to_byte_offset();
		}

		template<std::integral T>
		void write(T input, u32 bits = sizeof(T) * 8)
		{
			if constexpr (std::is_same_v<T, bool>)
			{
				write_bits(input, 1);
				return;
			}
			else
			{
				write_bits(input, bits);
			}
		}

		template<std::floating_point T>
		void write(T input)
		{
			if constexpr (sizeof(T) == 4)
			{
				u32 bitcasted = std::bit_cast<u32>(input);
				write_bits(bitcasted);
			}
			else if constexpr (sizeof(T) == 8)
			{
				u64 bitcasted = std::bit_cast<u64>(input);
				write_bits(bitcasted);
			}
			else
			{
				assert::check(false, "Unhandled floating point type");
			}
		}

		// Write arrays


		void write(std::string_view input) { write(std::span{input}); }

		template<typename T, size_t S>
		void write(std::array<T, S> input)
		{
			write(std::span<T>{input.data(), S});
		}

		template<typename T, size_t S>
		void write(std::array<T, S> input, const u32 bits)
		{
			write(std::span<T>{input.data(), bits / 8});
		}

		template<typename T>
		void write(std::vector<T>& input)
		{
			write(std::span{input});
		}

		template<typename T>
		void write(std::vector<T>& input, const u32 bits)
		{
			write(std::span<T>{input.data(), bits / 8});
		}

		template<typename T>
		void write(std::span<T> input)
		{
			for (const u8 c : input)
				write_byte(c);
		}

		// Reads

		template<typename T, size_t Size = 8 * sizeof(T)>
		T read(u32 len = Size)
		{
			align_to_byte_offset();


			if constexpr (std::is_convertible_v<T, std::string>)
			{
				return read_string<T>(len);
			}
			else
			{
				bool aligns = bit_offset() == 0;
				u32  bytes  = len / 8;
				if (aligns)
				{
					T ret{};
					while (bytes--)
					{
						ret <<= 8;
						ret |= buffer[byte_index()];
						bitpos += 8;
					}
					return ret;
				}

				T ret{};
				for (u32 i = 0; i < len; i++)
				{
					ret <<= 1;
					T is = (T)read_bit();
					ret += is;
				}
				return ret;
			}
		}

		template<std::integral T>
		T read()
		{
			align_to_byte_offset();

			assert::check(empty(), "Buffer has no more data");


			if constexpr (std::is_same_v<T, bool>)
			{
				return read_bit();
			}
			else
			{
				u32  bytes  = sizeof(T);
				bool aligns = bit_offset() == 0;

				T ret{};
				if (aligns)
				{
					while (bytes--)
					{
						ret <<= 8;
						ret |= buffer[byte_index()];
						bitpos += 8;
					}
					return ret;
				}

				return read<T>(sizeof(T) * 8);
			}
		}

		template<std::floating_point T>
		T read()
		{
			align_to_byte_offset();


			if constexpr (sizeof(T) == 4)
			{
				u32 value = read<u32>();
				return std::bit_cast<f32>(value);
			}
			else if constexpr (sizeof(T) == 8)
			{
				u64 value = read<u64>();
				return std::bit_cast<f64>(value);
			}
			else
			{
				assert::check(false, "Unhandled floating point type");
			}
		}

		template<typename T, size_t S>
		void read(std::array<T, S>& output)
		{
			read(std::span<u8>{output}, S * 8);
		}

		template<typename T, size_t S>
		void read(std::array<T, S>& output, const u32 bits)
		{
			read(std::span<u8>{output}, bits);
		}

		template<typename T>
		void read(std::vector<T>& output, const u32 bits)
		{
			output.clear();
			output.resize(bits / 8);

			read(std::span<u8>{output}, bits);
		}

		template<typename T>
		void read(std::span<T> buffer, u32 bits)
		{
			const auto count = bits / 8;
			auto       bytes = count;

			assert::check(buffer.size() >= count, "Output buffer not big enough");

			while (bytes--)
			{
				T element                 = read<T>(sizeof(T) * 8);
				buffer[count - 1 - bytes] = element;
			}
		}

		// Other

		std::span<u8> data() { return {buffer.data(), buffer.size()}; }

		bool empty() const { return byte_index() <= buffer.size(); }

		size_t size() const { return buffer.size() * 8; }

		size_t size_in_bytes() const { return buffer.size(); }

		void clear()
		{
			buffer.clear();
			bitpos = 0;
		}

		void reserve(size_t size) { buffer.reserve(size); }

		// TODO: operator &=(?) for simpler serialization
		// struct S { int x;};
		// serializer ser;
		// ser &= s.x;



		// TODO: save types for serialized as a metadata
	};


} // namespace deckard::serializer
