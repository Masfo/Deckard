export module deckard.serializer;

import std;
import deckard.types;
import deckard.assert;
import deckard.bitbuffer;
import deckard.as;
import deckard.debug;

namespace deckard
{
	// TODO: fixed size reader/writer
	// TODO: append bitreader/writers
	// TODO: limit array/string to 16-bit max?

#ifdef _DEBUG
	enum class serialize_type : u8
	{
		boolean,
		u8,
		u16,
		u32,
		u64,
		i8,
		i16,
		i32,
		i64,
		f32,
		f64,
		string,
		array,
		nothing
	};

	std::string serialize_to_string(const serialize_type t)
	{
		switch (t)
		{
			case serialize_type::boolean: return "boolean";
			case serialize_type::u8: return "u8";
			case serialize_type::u16: return "u16";
			case serialize_type::u32: return "u32";
			case serialize_type::u64: return "u64";
			case serialize_type::i8: return "i8";
			case serialize_type::i16: return "i16";
			case serialize_type::i32: return "i32";
			case serialize_type::i64: return "i64";
			case serialize_type::f32: return "f32";
			case serialize_type::f64: return "f64";
			case serialize_type::string: return "string";
			case serialize_type::array: return "array";
			default: return "nothing";
		}
	}

	template<typename T>
	serialize_type type_to_serialize()
	{
		if constexpr (std::is_same_v<T, bool>)
			return serialize_type::boolean;
		else if constexpr (std::is_same_v<T, u8>)
			return serialize_type::u8;
		else if constexpr (std::is_same_v<T, u16>)
			return serialize_type::u16;
		else if constexpr (std::is_same_v<T, u32>)
			return serialize_type::u32;
		else if constexpr (std::is_same_v<T, u64>)
			return serialize_type::u64;
		else if constexpr (std::is_same_v<T, i8>)
			return serialize_type::i8;
		else if constexpr (std::is_same_v<T, i16>)
			return serialize_type::i16;
		else if constexpr (std::is_same_v<T, i32>)
			return serialize_type::i32;
		else if constexpr (std::is_same_v<T, i64>)
			return serialize_type::i64;
		else if constexpr (std::is_same_v<T, f32>)
			return serialize_type::f32;
		else if constexpr (std::is_same_v<T, f64>)
			return serialize_type::f64;
		else if constexpr (std::is_same_v<T, std::string>)
			return serialize_type::string;
		else if constexpr (std::is_array_v<T>)
			return serialize_type::array;
	}

	struct types
	{
		u32            size_in_bits{0}; // in bits
		serialize_type type;
	};
#endif

	export enum class padding : u8 { yes, no };

	export class serializer
	{
	private:
		std::vector<u8> buffer;
		u64             writepos{0};
		u64             readpos{0};
		padding         pad{padding::no};

#ifdef _DEBUG
		u32                current_type_index{0};
		std::vector<types> types_written;

		void write_type(serialize_type t, u32 size_in_bits)
		{
			types_written.push_back({size_in_bits, t});
			current_type_index++;
		}

		void check_read_type(serialize_type type, u32 bits) 
		{
			if (types_written.empty())
				dbg::panic("No types written, cannot check type");


			const auto& expected_size = std::max(bits, types_written[current_type_index].size_in_bits);
			const auto& expected_type = types_written[current_type_index].type;

			assert::check(current_type_index <= types_written.size(),
						  std::format("Current type index out of bounds: {}, types written: {}", current_type_index, types_written.size()));

			if (expected_type == type)
			{


				if (expected_size != std::max(bits, expected_size))
				{
					dbg::println("Types match '{}' but expected '{}' bits, got '{}' bits", bits, expected_size);
					dbg::panic("size mismatch");
				}
				current_type_index++;
				return;
			}

			dbg::println("Incorrect type read, expected: '{}', got: '{}'", serialize_to_string(expected_type), serialize_to_string(type));
			dbg::panic("wrong type");
		}


		template<typename T>
		void check_read_type(u32 size)
		{
			const auto type = type_to_serialize<T>();
			check_read_type(type, size);
		}
#endif


	private:
		u64 byte_index(u64 offset) const { return offset / 8u; }

		u64 bit_offset(u64 offset) const { return offset % 8u; }

		void align_to_byte_offset(auto& pos)
		{
			const u64 offset    = bit_offset(pos);
			const u64 remainder = 8 - offset;

			if (pad == padding::yes and offset != 0)
			{
				pos += remainder;
			}
		}


	private: // Writing
		void write_single_bit(u8 bit)
		{
			const u64 byteindex = byte_index(writepos);
			const u64 offset    = bit_offset(writepos);
			if (offset == 0)
				buffer.push_back(0);
			if (bit)
				buffer[byteindex] |= 1 << (7 - offset);
			writepos++;
			align_to_byte_offset(writepos);
		}

		template<typename T, size_t Size = 8 * sizeof(T)>
		void write_bits(const T input, u32 bits = Size)
		{
			const u64 offset = writepos % 8;
			const u32 bytes  = bits / 8;
			bool      aligns = bits % 8 == 0;


			if (offset == 0 and aligns)
			{
				for (u32 i = 0; i < bytes; i++)
				{
					u8 byte = (input >> ((bits - 8) - i * 8)) & 0xFF;
					write_byte(byte);
				}

#ifdef _DEBUG
				write_type(type_to_serialize<T>(), bits);
#endif

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
			const u64 offset    = bit_offset(readpos);
			const u64 remainder = 7 - offset;

			assert::check(byte_index(readpos) <= buffer.size(), "Buffer has no more data to read");


			u8 byte   = buffer[byte_index(readpos)];
			u8 masked = (byte >> remainder) & 1;

			readpos += 1;
			align_to_byte_offset(readpos);


			return masked;
		}


	public:
		serializer()
			: writepos{0}
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
			const u64 offset    = bit_offset(writepos);
			const u64 remainder = 8 - offset;

			if (offset == 0)
			{
				buffer.push_back(byte);
			}
			else
			{

				buffer[byte_index(writepos)] |= byte >> offset;
				buffer.push_back(byte << remainder);
			}

			writepos += 8;


			align_to_byte_offset(writepos);
		}

		template<typename T>
		void write(std::span<T> input, u32 size = 0)
		{
			u32 count = size == 0 ? as<u32>(input.size()) : size;

#ifdef _DEBUG
			write_type(serialize_type::array, count * 8);
#endif

			write<u32>(count);

			for (const u8 c : input)
				write_byte(c);
		}

		template<std::integral T>
		void write(T input, u32 bits = sizeof(T) * 8)
		{


			if constexpr (std::is_same_v<T, bool>)
			{
				write_bits(input, 1);
#ifdef _DEBUG
				write_type(type_to_serialize<T>(), 1);
#endif
				return;
			}
			else
			{
				write_bits(input, bits);
#ifdef _DEBUG
				write_type(type_to_serialize<T>(), bits);
#endif
			}
		}

		template<std::floating_point T>
		void write(T input)
		{
			if constexpr (sizeof(T) == 4)
			{
				u32 bitcasted = std::bit_cast<u32>(input);

#ifdef _DEBUG
				write_type(serialize_type::f32, 32);
				write_type(serialize_type::u32, 32); // HACK
#endif

				write_bits(bitcasted);
			}
			else if constexpr (sizeof(T) == 8)
			{
				u64 bitcasted = std::bit_cast<u64>(input);

#ifdef _DEBUG
				write_type(serialize_type::f64, 64);
				write_type(serialize_type::u64, 64); // HACK

#endif

				write_bits(bitcasted);
			}
			else
			{
				assert::check(false, "Unhandled floating point type");
			}
		}

		// Write arrays


		void write(std::string_view input)
		{
#ifdef _DEBUG
			write_type(serialize_type::string, as<u32>(input.size() * 8ull));
#endif

			write(std::span{input});
		}

		template<typename T, size_t S>
		void write(std::array<T, S> input)
		{
#ifdef _DEBUG
			write_type(serialize_type::array, as<u32>(input.size() * 8ull));
#endif

			write(std::span<T>{input.data(), S});
		}

		template<typename T, size_t S>
		void write(std::array<T, S> input, const u32 bits)
		{
#ifdef _DEBUG
			write_type(serialize_type::array, bits);
#endif

			write(std::span<T>{input.data(), bits});
		}

		template<typename T>
		void write(std::vector<T>& input)
		{

			write(std::span{input});
		}

		template<typename T>
		void write(std::vector<T>& input, const u32 bits)
		{
			write(std::span<T>{input.data(), bits});
		}

		// Reads

		template<typename T>
		void read(std::span<T> output, u32 bits)
		{

			u32 bytecount = read<u32>();

			const u32 count = bits == 0 ? bytecount : bits / 8;

			assert::check(output.size() >= bytecount, "Output buffer not big enough");

			while (bytecount--)
			{
				T element                     = read<T>(sizeof(T) * 8);
				output[count - 1 - bytecount] = element;
			}
		}

		template<typename T, size_t Size = 8 * sizeof(T)>
		T read(u32 len = Size)
		{
			align_to_byte_offset(readpos);


			if constexpr (std::is_convertible_v<T, std::string>)
			{
				return read_string<T>();
			}
			else
			{
				bool aligns = bit_offset(readpos) == 0;
				u32  bytes  = len / 8;
				if (aligns)
				{
					T ret{};
					while (bytes--)
					{
						ret <<= 8;
						ret |= buffer[byte_index(readpos)];
						readpos += 8;
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
			align_to_byte_offset(readpos);


			assert::check(byte_index(readpos) <= buffer.size(), "Buffer has no more data");

#ifdef _DEBUG
				check_read_type<T>(sizeof(T) * 8);
#endif


			if constexpr (std::is_same_v<T, bool>)
			{
				return read_bit();
			}
			else
			{

				u32  bytes  = sizeof(T);
				bool aligns = bit_offset(readpos) == 0;

				T ret{};
				if (aligns)
				{
					while (bytes--)
					{
						ret <<= 8;
						ret |= buffer[byte_index(readpos)];
						readpos += 8;
					}
					return ret;
				}

				return read<T>(sizeof(T) * 8);
			}
		}

		template<std::floating_point T>
		T read()
		{
			align_to_byte_offset(readpos);

			assert::check(byte_index(readpos) <= buffer.size(), "Buffer has no more data");

#ifdef _DEBUG
			check_read_type<T>(sizeof(T) * 8);
#endif

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

		template<typename T>
		T read_string()
		{
			align_to_byte_offset(readpos);
			assert::check(byte_index(readpos) <= buffer.size(), "Buffer has no more data");

#ifdef _DEBUG
			check_read_type(serialize_type::string, 0);
			check_read_type(serialize_type::array, 0);
#endif

			T result{};
			if constexpr (std::is_same_v<T, std::string>)
			{
				auto count = read<u32>();

				result.reserve(count);
				for (u32 i = 0; i < count; i++)
					result.push_back(read<u8>());
			}

			align_to_byte_offset(readpos);
			return result;
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

		// Other

		std::span<u8> data() { return {buffer.data(), buffer.size()}; }

		bool empty() const { return buffer.empty(); }

		void reset()
		{
			buffer.clear();
			writepos = 0;
		}

		// size in bytes
		size_t size() const { return buffer.size(); }

		size_t size_in_bits() const { return writepos; }

		void clear()
		{
			buffer.clear();
			rewind();
		}

		void rewind()
		{
			readpos = 0;

#ifdef _DEBUG
			current_type_index = 0;
#endif
		}

		void reserve(size_t size) { buffer.reserve(size); }

		// TODO: operator &=(?) for simpler serialization
		// struct S { int x;};
		// serializer ser;
		// ser &= s.x;


		// TODO: save types for serialized as a metadata
	};


} // namespace deckard
