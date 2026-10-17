export module deckard.inflate;

import std;
import deckard.types;
import deckard.assert;

namespace deckard
{

	class bit_reader
	{
		std::span<const u8> data_;
		u64                 pos{};    // next byte to load
		u64                 buffer{}; // LSB-first bit buffer
		u32                 count{};  // valid bits in buffer
		bool                over{};

	public:
		// Plain-pointer copy of the reader state, for hot loops that keep it in locals.
		struct snapshot
		{
			const u8* in{};  // next byte to load
			const u8* end{}; // one past the last input byte
			u64       bb{};  // bit buffer
			u32       bc{};  // valid bits in bb
		};

		explicit bit_reader(std::span<const u8> d) noexcept
			: data_{d}
		{
		}

		[[nodiscard]] snapshot save() const noexcept
		{
			const u8* const b = data_.data();
			return {b + pos, b + data_.size(), buffer, count};
		}

		void restore(const snapshot& s) noexcept
		{
			if (s.bc > 64)
			{
				over   = true;
				buffer = 0;
				count  = 0;
			}
			else
			{
				buffer = s.bb;
				count  = s.bc;
			}
			pos = static_cast<u64>(s.in - data_.data());
		}

		void refill() noexcept
		{
			static_assert(std::endian::native == std::endian::little);

			// Fast path: an empty buffer can take 8 bytes at once.
			if (count == 0 and data_.size() - pos >= 8) 
			{
				std::memcpy(&buffer, data_.data() + pos, 8);
				pos += 8;
				count = 64;
				return;
			}

			// Append only complete bytes that fit in the buffer.
			while (count <= 56 and pos < data_.size())
			{
				buffer |= u64{data_[pos++]} << count;
				count += 8;
			}
		}

		[[nodiscard]] u32 available() const noexcept { return count; }

		[[nodiscard]] u32 peek(u32 n) const noexcept
		{
			assert::check(n <= 32, "bit_reader::peek width");
			return static_cast<u32>(buffer & ((u64{1} << n) - 1));
		}

		void consume(u32 n) noexcept
		{
			if (n > count)
			{
				over   = true;
				buffer = 0;
				count  = 0;
				return;
			}
			buffer >>= n;
			count -= n;
		}

		[[nodiscard]] u32 bits(u32 n) noexcept
		{
			assert::check(n <= 32, "bit_reader::bits width");
			refill();

			if (n > count)
			{
				over   = true;
				buffer = 0;
				count  = 0;
				return 0;
			}

			const u32 v = peek(n);
			consume(n);
			return v;
		}

		void align_to_byte() noexcept
		{
			const u32 drop = count & 7;
			count -= drop;
			pos -= count / 8;
			buffer = 0;
			count  = 0;
		}

		[[nodiscard]] std::span<const u8> take_bytes(u64 n) noexcept
		{
			if (n > data_.size() - pos)
			{
				over = true;
				return {};
			}
			const auto s = data_.subspan(pos, n);
			pos += n;
			return s;
		}

		[[nodiscard]] bool overrun() const noexcept { return over; }
	};

	[[nodiscard]] constexpr u32 adler32(std::span<const u8> data) noexcept
	{
		constexpr usize nmax = 5552;
		u32             a = 1, b = 0;

		const u8* p    = data.data();
		usize     left = data.size();
		while (left != 0)
		{
			const usize n = left < nmax ? left : nmax;
			for (const u8* const e = p + n; p != e; ++p)
			{
				a += *p;
				b += a;
			}
			a %= 65521;
			b %= 65521;
			left -= n;
		}
		return (b << 16) | a;
	}

	struct sink
	{
		u8*   base{};
		usize pos{};
		usize cap{};
	};

	[[nodiscard]] constexpr u32 reverse_bits(u32 v, u32 n) noexcept
	{
		v = ((v >> 1) & 0x5555u) | ((v & 0x5555u) << 1);
		v = ((v >> 2) & 0x3333u) | ((v & 0x3333u) << 2);
		v = ((v >> 4) & 0x0F0Fu) | ((v & 0x0F0Fu) << 4);
		v = ((v >> 8) & 0x00FFu) | ((v & 0x00FFu) << 8);
		return v >> (16 - n);
	}

	static constexpr u16 length_base[29] = {
	  3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
	static constexpr u8 length_extra[29] = {
	  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
	static constexpr u16 dist_base[30] = {
	  1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
	  193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
	static constexpr u8 dist_extra[30] = {
	  0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

	struct huffman
	{
		static constexpr u32 fast_bits = 10;
		u16                  fast[1u << fast_bits]{};
		u16                  count[16]{};
		u16                  symbol[288]{};

		// `h` must be freshly value-initialized. Returns false on an over-subscribed code.
		[[nodiscard]] static constexpr bool build_into(huffman& h, std::span<const u8> lengths) noexcept
		{
			for (u8 l : lengths)
				++h.count[l];

			i32 left = 1;
			for (u32 len = 1; len < 16; ++len)
			{
				left <<= 1;
				left -= h.count[len];
				if (left < 0)
					return false;
			}
			h.count[0] = 0;

			u16 offs[16]{};
			u32 next_code[16]{};
			for (u32 len = 1, code = 0; len < 16; ++len)
			{
				if (len < 15)
					offs[len + 1] = static_cast<u16>(offs[len] + h.count[len]);
				code           = (code + h.count[len - 1]) << 1;
				next_code[len] = code;
			}

			for (u32 sym = 0; sym < lengths.size(); ++sym)
			{
				const u32 len = lengths[sym];
				if (len == 0)
					continue;

				h.symbol[offs[len]++] = static_cast<u16>(sym);

				const u32 code = next_code[len]++;
				if (len <= fast_bits)
				{
					const u16 entry = static_cast<u16>(sym | (len << 9));
					for (u32 i = reverse_bits(code, len); i < (1u << fast_bits); i += 1u << len)
						h.fast[i] = entry;
				}
			}
			return true;
		}

		[[nodiscard]] static constexpr std::expected<huffman, std::string> build(std::span<const u8> lengths) noexcept
		{
			huffman h;
			if (not build_into(h, lengths))
				return std::unexpected("bad huffman");
			return h;
		}

		[[nodiscard]] i32 decode_prefilled(bit_reader& br) const noexcept
		{
			if (const u32 e = fast[br.peek(fast_bits)]; e != 0) [[likely]]
			{
				br.consume(e >> 9);
				return static_cast<i32>(e & 0x1FF);
			}

			u32       len = 0;
			const i32 sym = slow(br.peek(15), len);
			if (sym >= 0)
				br.consume(len);
			return sym;
		}

		[[nodiscard]] i32 decode(bit_reader& br) const noexcept
		{
			br.refill();
			return decode_prefilled(br);
		}

		[[nodiscard]] i32 slow(u32 window, u32& len_out) const noexcept
		{
			i32 code = 0, first = 0, index = 0;
			for (u32 len = 1; len < 16; ++len)
			{
				code |= static_cast<i32>((window >> (len - 1)) & 1);
				const i32 c = count[len];
				if (code - c < first)
				{
					len_out = len;
					return symbol[index + (code - first)];
				}
				index += c;
				first += c;
				first <<= 1;
				code <<= 1;
			}
			return -1;
		}
	};

	[[msvc::noinline]] [[nodiscard]] std::expected<void, std::string> inflate_codes(
	  bit_reader& br, const huffman& lit, const huffman& dist, sink& out) noexcept
	{
		static_assert(std::endian::native == std::endian::little);

		const auto      s   = br.save();
		const u8*       in  = s.in;
		const u8* const end = s.end;
		u64             bb  = s.bb;
		u32             bc  = s.bc;

		assert::check(bc <= 64, "bit buffer overflow");

		u8* const   base = out.base;
		const usize cap  = out.cap;
		usize       pos  = out.pos;
		const u16*  lf   = lit.fast;
		const u16*  df   = dist.fast;

		auto leave = [&](const std::string& r)
		{
			br.restore({in, end, bb, bc});
			out.pos = pos;
			return std::unexpected(r);
		};

		for (;;)
		{

			// prefill the bit buffer if we have at least 8 bytes left in the input, otherwise refill slowly
			if (bc == 0 and end - in >= 8)
			{
				std::memcpy(&bb, in, 8);
				in += 8;
				bc = 64;
			}
			else
			{
				while (bc <= 56 and in < end)
				{
					bb |= u64{*in++} << bc;
					bc += 8;
				}
			}

			// decode a symbol from the literal/length tree
			u32 e = lf[bb & 1023], l = 0;
			i32 sym{};
			if (e)
			{
				l   = e >> 9;
				sym = static_cast<i32>(e & 0x1FF);
			}
			else if ((sym = lit.slow(u32(bb & 0x7FFF), l)) < 0)
				return leave("bad huffman");

			bb >>= l;
			bc -= l;
			if (bc > 63) // u32 wrapped: consumed more bits than the input had
				return leave("truncated");

			if (sym < 256)
			{
				if (pos == cap)
					return leave("too large");
				base[pos++] = static_cast<u8>(sym);
				continue;
			}
			if (sym == 256)
			{
				br.restore({in, end, bb, bc});
				out.pos = pos;
				return {};
			}

			const u32 li = static_cast<u32>(sym - 257);
			if (li >= 29)
				return leave("bad symbol");

			const u32 xl  = length_extra[li];
			const u32 len = length_base[li] + u32(bb & ((1u << xl) - 1));
			bb >>= xl;
			bc -= xl;

			i32 ds{};
			e = df[bb & 1023];
			if (e)
			{
				l  = e >> 9;
				ds = static_cast<i32>(e & 0x1FF);
			}
			else if ((ds = dist.slow(u32(bb & 0x7FFF), l)) < 0)
				return leave("bad huffman");
			bb >>= l;
			bc -= l;
			if (bc > 63) // u32 wrapped
				return leave("truncated");

			if (ds >= 30)
				return leave("bad symbol");

			const u32 xd = dist_extra[ds];
			const u32 d  = dist_base[ds] + u32(bb & ((1u << xd) - 1));
			bb >>= xd;
			bc -= xd;

			if (bc > 63)
				return leave("truncated");
			if (d > pos)
				return leave("bad distance");
			if (len > cap - pos)
				return leave("too large");

			// copy selection from already written output, possibly overlapping
			u8* const dst = base + pos;
			const u8* src = dst - d;

			if (d >= 8 and len >= 8)
			{
				u32 i = 0;

				for (; i + 8 <= len; i += 8)
					std::memcpy(dst + i, src + i, 8);

				if (i < len)
					std::memcpy(dst + i, src + i, len - i);
			}
			else if (d >= len)
			{
				std::memcpy(dst, src, len);
			}
			else if (d == 1)
			{
				std::memset(dst, *src, len);
			}
			else
			{
				std::memcpy(dst, src, d);

				usize done = d;
				while (done < len)
				{
					const usize n = std::min<usize>(done, len - done);
					std::memcpy(dst + done, dst, n);
					done += n;
				}
			}

			pos += len;
		}
	}

	[[nodiscard]] std::expected<void, std::string> inflate_dynamic(bit_reader& br, sink& out) noexcept
	{
		// HLIT(5) + HDIST(5) + HCLEN(4) = 14 bits, one refill
		br.refill();
		const u32 hdr = br.peek(14);
		br.consume(14);
		const u32 hlit  = (hdr & 31) + 257;
		const u32 hdist = ((hdr >> 5) & 31) + 1;
		const u32 hclen = (hdr >> 10) + 4;
		if (hlit > 286 or hdist > 30)
			return std::unexpected("bad block");

		static constexpr u8 order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
		u8                  cl_lengths[19]{};
		for (u32 i = 0; i < hclen; ++i)
		{
			if (i == 0 or i == 18) // 18 * 3 = 54 bits fit in one refill
				br.refill();
			cl_lengths[order[i]] = static_cast<u8>(br.peek(3));
			br.consume(3);
		}
		if (br.overrun())
			return std::unexpected("truncated");

		huffman cl{};
		if (not huffman::build_into(cl, cl_lengths))
			return std::unexpected("bad huffman");
		const huffman& cl_ref = cl;

		u8        lengths[286 + 30]{};
		u8* const L     = lengths;
		const u32 total = hlit + hdist;

		for (u32 i = 0; i < total;)
		{
			// code (<= 7 bits) + extra bits (<= 7) always fit in what one refill guarantees
			br.refill();
			const i32 sym = cl_ref.decode_prefilled(br);
			if (sym < 0)
				return std::unexpected(br.overrun() ? "truncated" : "bad huffman");

			if (sym < 16)
			{
				L[i++] = static_cast<u8>(sym);
				continue;
			}

			u32 repeat;
			u8  value = 0;
			if (sym == 16)
			{
				if (i == 0)
					return std::unexpected("bad huffman");
				value  = L[i - 1];
				repeat = 3 + br.peek(2);
				br.consume(2);
			}
			else if (sym == 17)
			{
				repeat = 3 + br.peek(3);
				br.consume(3);
			}
			else
			{
				repeat = 11 + br.peek(7);
				br.consume(7);
			}

			if (repeat > total - i)
				return std::unexpected("bad huffman");
			if (value != 0) // zeros are already there
				std::memset(L + i, value, repeat);
			i += repeat;
		}

		if (br.overrun())
			return std::unexpected("truncated");
		if (L[256] == 0)
			return std::unexpected("bad huffman"); // no end-of-block

		huffman lit{};
		huffman dist{};
		if (not huffman::build_into(lit, {L, hlit}) or not huffman::build_into(dist, {L + hlit, hdist}))
			return std::unexpected("bad huffman");

		return inflate_codes(br, lit, dist, out);
	}

	[[nodiscard]] std::expected<void, std::string> inflate_fixed(bit_reader& br, sink& out) noexcept
	{
		static constexpr auto tables = []
		{
			std::array<u8, 288> l{};
			std::ranges::fill(l | std::views::take(144), u8{8});
			std::ranges::fill(l | std::views::drop(144) | std::views::take(112), u8{9});
			std::ranges::fill(l | std::views::drop(256) | std::views::take(24), u8{7});
			std::ranges::fill(l | std::views::drop(280), u8{8});

			std::array<u8, 30> d{};
			d.fill(5);

			return std::pair{*huffman::build(l), *huffman::build(d)};
		}();
		return inflate_codes(br, tables.first, tables.second, out);
	}

	[[nodiscard]] std::expected<void, std::string> inflate_stored(bit_reader& br, sink& out) noexcept
	{
		br.align_to_byte();
		const auto hdr = br.take_bytes(4);
		if (br.overrun())
			return std::unexpected("truncated");

		const u16 len  = static_cast<u16>(hdr[0] | (hdr[1] << 8));
		const u16 nlen = static_cast<u16>(hdr[2] | (hdr[3] << 8));
		if (static_cast<u16>(~nlen) != len)
			return std::unexpected("bad stored length");

		const auto raw = br.take_bytes(len);
		if (br.overrun())
			return std::unexpected("truncated");
		if (len > out.cap - out.pos)
			return std::unexpected("too large");

		if (len)
			std::memcpy(out.base + out.pos, raw.data(), len);

		out.pos += len;
		return {};
	}

	// raw deflate stream
	[[nodiscard]] std::expected<std::vector<u8>, std::string> inflate(std::span<const u8> in, u64 exact_size) noexcept
	{
		bit_reader      br{in};
		std::vector<u8> buf(exact_size);

		if (buf.size() < exact_size)
			return std::unexpected("buffer did not allocate enough memory");

		sink out{buf.data(), 0, buf.size()};

		bool is_final = false;
		while (not is_final)
		{
			is_final       = br.bits(1) != 0;
			const u32 type = br.bits(2);
			if (br.overrun())
				return std::unexpected("truncated");

			std::expected<void, std::string> r;
			switch (type)
			{
				case 0: r = inflate_stored(br, out); break;
				case 1: r = inflate_fixed(br, out); break;
				case 2: r = inflate_dynamic(br, out); break;
				default: return std::unexpected("bad block");
			}
			if (not r)
				return std::unexpected(r.error());
		}
		buf.resize(out.pos);
		return buf;
	}

	export [[nodiscard]] std::expected<std::vector<u8>, std::string> png_decompress(
	  std::span<const u8> input, u64 size_hint = 0, bool validate_checksum = true) noexcept
	{
		if (input.size() < 6)
			return std::unexpected("input too small");

		const u32 cmf = input[0];
		const u32 flg = input[1];

		if ((cmf & 0x0F) != 8 || (cmf >> 4) > 7 || // deflate, window <= 32K
			((cmf << 8) | flg) % 31 != 0 ||        // header check bits
			(flg & 0x20))                          // preset dictionary: not valid in PNG
			return std::unexpected("bad header");

		auto out = inflate(input.subspan(2, input.size() - 6), size_hint);
		if (not out)
			return out;

		if (validate_checksum)
		{

			const u32 stored = (u32{input[input.size() - 4]} << 24) | (u32{input[input.size() - 3]} << 16)
							   | (u32{input[input.size() - 2]} << 8) | u32{input[input.size() - 1]};

			if (stored != adler32(*out))
				return std::unexpected("bad checksum");
		}
		return out;
	}

} // namespace deckard
