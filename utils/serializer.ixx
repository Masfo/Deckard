export module deckard.serializer;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.utf8;

namespace deckard
{
	// TODO: fixed size reader/writer
	// TODO: append bitreader/writers
	// TODO: limit array/string to 16-bit max?


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


	export enum class padding : u8 { yes, no };

	export class serializer
	{
	private:
		std::vector<u8> buffer;
		u64             writepos{0};
		u64             readpos{0};
		padding         pad{padding::no};

	public:
		template<std::integral T>
		void write_le(T input)
		{
			align_to_byte_offset(writepos);
			using U = std::make_unsigned_t<T>;
			U v     = static_cast<U>(input);
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				write_byte(static_cast<u8>((v >> (i * 8)) & 0xFF));
			}
		}

		template<std::integral T>
		void write_be(T input)
		{
			align_to_byte_offset(writepos);
			using U = std::make_unsigned_t<T>;
			U v     = static_cast<U>(input);
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				write_byte(static_cast<u8>((v >> ((sizeof(T) - 1 - i) * 8)) & 0xFF));
			}
		}

		template<std::integral T>
		T read_le()
		{
			align_to_byte_offset(readpos);
			assert::check(byte_index(readpos) + sizeof(T) <= buffer.size(), "Buffer has no more data");

			using U = std::make_unsigned_t<T>;
			U v{};
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				v |= static_cast<U>(buffer[byte_index(readpos)]) << (i * 8);
				readpos += 8;
			}
			return static_cast<T>(v);
		}

		template<std::integral T>
		T read_be()
		{
			return read<T>();
		}


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

		serializer(std::span<const u8> data)
			: serializer()
		{
			buffer.insert(buffer.end(), data.begin(), data.end());
			writepos = 0;
			readpos  = 0;
		}

		// Copy
		serializer(const serializer&)            = delete;
		serializer& operator=(const serializer&) = delete;
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
		void write(std::span<T> input)
		{
			for (const u8 c : input)
				write_byte(c);
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


		void write(std::string_view input)
		{
			write<u32>(as<u32>(input.size()));
			write(std::span{input});
		}

		template<typename T, size_t S>
		void write(std::array<T, S> input, const u32 count = S)
		{
			write(std::span<T>{input.data(), count});
		}

		// count == std::dynamic_extent writes the whole vector
		template<typename T>
		void write(std::vector<T>& input, const size_t count = std::dynamic_extent)
		{
			write(std::span<T>{input.data(), count == std::dynamic_extent ? input.size() : count});
		}

		// Reads

		template<typename T>
		void read(std::span<T> output, u32 bits)
		{

			u32 bytecount = bits / 8;

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
				assert::check(byte_index(readpos) <= buffer.size(), "Buffer has no more data");

				T result{};
				if constexpr (std::is_same_v<T, std::string>)
				{
					const u32 count = read<u32>();

					result.reserve(count);

					for (u32 i = 0; i < count; i++)
						result.push_back(read<u8>());
				}

				align_to_byte_offset(readpos);
				return result;
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
		void read(std::array<T, S>& output, const u32 bits = S * 8)
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
			rewind();
		}

		void reset(std::span<u8> data)
		{
			reset();
			buffer.assign(data.begin(), data.end());
		}

		// remaining
		size_t remaining() const { return buffer.size() - byte_index(readpos); }

		// Read all remaining bytes as a span
		std::span<const u8> read_remaining()
		{
			align_to_byte_offset(readpos);
			const size_t start = byte_index(readpos);
			const size_t count = remaining();
			readpos            = buffer.size() * 8; // Move read position to end
			return std::span<const u8>{buffer.data() + start, count};
		}

		// size in bytes
		size_t size() const { return buffer.size(); }

		size_t size_in_bits() const { return writepos; }

		void rewind() { readpos = 0; }

		void reserve(size_t size) { buffer.reserve(size); }

		// TODO: operator &=(?) for simpler serialization
		// struct S { int x;};
		// serializer ser;
		// ser &= s.x;


		// TODO: save types for serialized as a metadata
	};

	// ############################################################################################
	// ############################################################################################

	template<typename S, typename T>
	concept has_member_serialize = requires(S& s, const T& v) { v.serialize(s); };

	template<typename S, typename T>
	concept has_member_deserialize = requires(S& s, T& v) { v.deserialize(s); };

	// non-member (free function) overloads, found via ADL in the type's namespace
	template<typename S, typename T>
	concept has_free_serialize = requires(S& s, const T& v) { serialize(s, v); };

	template<typename S, typename T>
	concept has_free_deserialize = requires(S& s, T& v) { deserialize(s, v); };

	template<typename T>
	struct is_byte_array : std::false_type {};

	template<usize N>
	struct is_byte_array<std::array<u8, N>> : std::true_type {};

	// S is a serializer: write/read a value of type T (defaults to a byte), its destination form, and a byte span
	export template<typename S, typename T = u8>
	concept serializer_type =
	  requires(S& s, const T& value, T& out, std::span<const u8> bytes_in, std::span<u8> bytes_out) {
		s.write(value);
		s.template read<T>();
		s.read(out);
		s.write(bytes_in);
		s.read(bytes_out);
	};


	export class inplace_serializer final
	{
	private:
		static constexpr usize   capacity = 8_KiB;
		std::array<u8, capacity> buffer{};

		usize writepos{0};
		usize readpos{0};

		void write_raw(std::span<const u8> bytes)
		{
			assert::check(bytes.size() <= writable(), "inplace_serializer buffer overflow");
			std::ranges::copy(bytes, buffer.data() + writepos);
			writepos += bytes.size();
		}

		void write_array(std::span<const u8> bytes)
		{
			assert::check(sizeof(u32) + bytes.size() <= writable(), "inplace_serializer buffer overflow");

			write<u32>(as<u32>(bytes.size()));
			write_raw(bytes);
		}

		// byte order is only swapped on little-endian hosts; on big-endian it is already network order
		template<std::integral T>
		[[nodiscard]] static constexpr T byte_swap_if_little(T value) noexcept
		{
			if constexpr (std::endian::native == std::endian::little)
				return std::byteswap(value);
			else
				return value;
		}

		template<typename T>
		[[nodiscard]] std::array<u8, sizeof(T)> read_raw()
		{
			std::array<u8, sizeof(T)> raw{};
			read(std::span<u8>{raw});
			return raw;
		}

	public:
		// helpers
		[[nodiscard]] auto data() const noexcept { return std::span<const u8>{buffer.data(), writepos}; }

		[[nodiscard]] bool empty() const noexcept { return writepos == 0; }

		[[nodiscard]] usize size() const noexcept { return writepos; }

		[[nodiscard]] usize remaining() const noexcept { return writepos - readpos; }

		[[nodiscard]] usize writable() const noexcept { return capacity - writepos; }

		[[nodiscard]] usize max_size() const noexcept { return capacity; }

		void clear() noexcept
		{
			writepos = 0;
			rewind();
		}

		void seek(usize pos) noexcept
		{
			assert::check(pos <= writepos, "inplace_serializer seek position out of bounds");
			readpos = pos;
		}

		void rewind() noexcept { seek(0); }

		[[nodiscard]] const u8& operator[](usize index) const
		{
			assert::check(index < writepos, "inplace_serializer index out of bounds");
			return buffer[index];
		}

		[[nodiscard]] u8& operator[](usize index)
		{
			assert::check(index < writepos, "inplace_serializer index out of bounds");
			return buffer[index];
		}

		// read
		template<std::integral T>
		requires(std::same_as<std::remove_cvref_t<T>, u8>)
		[[nodiscard]] u8 read() { return read(1).front(); }

		template<std::integral T>
		requires(not std::same_as<T, bool> and not std::same_as<std::remove_cvref_t<T>, u8>)
		[[nodiscard]] T read()
		{
			return byte_swap_if_little(std::bit_cast<T>(read_raw<T>()));
		}

		template<std::same_as<bool> T>
		[[nodiscard]] bool read()
		{
			return read<u8>() != 0;
		}

		template<std::floating_point T>
		[[nodiscard]] T read()
		{
			using len = std::conditional_t<sizeof(T) == 4, u32, u64>;
			return std::bit_cast<T>(read<len>());
		}

		void read(std::span<u8> output) { std::ranges::copy(read(output.size()), output.begin()); }

		[[nodiscard]] std::span<const u8> read(usize count)
		{
			assert::check(count <= remaining(), "inplace_serializer buffer underflow");
			std::span<const u8> bytes{buffer.data() + readpos, count};
			readpos += count;
			return bytes;
		}

		[[nodiscard]] std::span<const u8> read() { return read(read<u32>()); }

		template<typename T>
		requires(is_byte_array<T>::value)
		[[nodiscard]] T read()
		{
			T out{};
			const u32 len = read<u32>();
			assert::check(len == out.size(), "inplace_serializer array size mismatch");
			read(std::span<u8>{out});
			return out;
		}

		template<typename T>
		requires(std::same_as<T, std::string> or std::same_as<T, std::string_view> or std::same_as<T, std::vector<u8>>)
		[[nodiscard]] T read()
		{
			const auto bytes = read();

			if constexpr (std::same_as<T, std::vector<u8>>)
				return T{bytes.begin(), bytes.end()};
			else
				return T{reinterpret_cast<const char*>(bytes.data()), bytes.size()};
		}

		template<typename T>
		requires(has_member_deserialize<inplace_serializer, T> and std::is_default_constructible_v<T>)
		[[nodiscard]] T read()
		{
			T value{};
			value.deserialize(*this);
			return value;
		}

		template<typename T>
		requires(has_free_deserialize<inplace_serializer, T> and not has_member_deserialize<inplace_serializer, T>
		         and std::is_default_constructible_v<T>)
		[[nodiscard]] T read()
		{
			T value{};
			deserialize(*this, value);
			return value;
		}

		template<pod_type T>
		requires(not has_member_deserialize<inplace_serializer, T> and not has_free_deserialize<inplace_serializer, T>)
		[[nodiscard]] T read()
		{
			return std::bit_cast<T>(read<std::array<u8, sizeof(T)>>());
		}

		template<typename T>
		requires(std::is_scoped_enum_v<T>)
		[[nodiscard]] T read()
		{
			return static_cast<T>(read<std::underlying_type_t<T>>());
		}

		template<typename T>
		void read(T& out)
		{
			out = read<T>();
		}

		// write
		template<std::integral T>
		requires(std::same_as<std::remove_cvref_t<T>, u8>)
		void write(T byte) { write_raw({&byte, 1}); }

		template<std::integral T>
		requires(not std::same_as<T, bool> and not std::same_as<std::remove_cvref_t<T>, u8>)
		void write(T value)
		{
			write_raw(std::bit_cast<std::array<u8, sizeof(T)>>(byte_swap_if_little(value)));
		}

		template<std::same_as<bool> T>
		void write(T value)
		{
			write(static_cast<u8>(value));
		}

		template<std::floating_point T>
		void write(T value)
		{
			static_assert(sizeof(T) == 4 or sizeof(T) == 8, "Unsupported floating point size");
			using type = std::conditional_t<sizeof(T) == 4, u32, u64>;
			write(std::bit_cast<type>(value));
		}

		void write(std::span<const u8> input) { write_array(input); }

		// explicit array overload: std::array<u8,N> is also convertible to utf8::view
		template<usize S>
		void write(const std::array<u8, S>& input)
		{
			write_array(input);
		}

		void write(std::string_view input) { write_array({reinterpret_cast<const u8*>(input.data()), input.size()}); }

		void write(utf8::view input) { write(input.to_string_view()); }

		template<typename T>
		requires(has_member_serialize<inplace_serializer, T>)
		void write(const T& value)
		{
			value.serialize(*this);
		}

		template<typename T>
		requires(has_free_serialize<inplace_serializer, T> and not has_member_serialize<inplace_serializer, T>)
		void write(const T& value)
		{
			serialize(*this, value);
		}

		template<pod_type T>
		requires(not has_member_serialize<inplace_serializer, T> and not has_free_serialize<inplace_serializer, T>)
		void write(const T& value)
		{
			write(std::bit_cast<std::array<u8, sizeof(T)>>(value));
		}

		template<typename T>
		requires(std::is_scoped_enum_v<T>)
		void write(const T& value)
		{
			write(std::to_underlying(value));
		}
	};


} // namespace deckard
