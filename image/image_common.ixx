export module deckard.image:common;

import std;
import deckard.types;
import deckard.assert;
import deckard.math.utils;
import deckard.colors;

namespace deckard
{


	template<u64 Channels>
	class image_channels final
	{
		static_assert(
		  Channels == 1 or Channels == 3 or Channels == 4, "Channels must be either 1 (Gray), 3 (RGB) or 4 (RGBA)");


	public:
		static constexpr u64 MAX_WIDTH  = static_cast<u64>(16384);
		static constexpr u64 MAX_HEIGHT = static_cast<u64>(16384);
		using color_format = std::conditional_t<Channels == 1, gray, std::conditional_t<Channels == 4, rgba, rgb>>;

	private:
		std::vector<color_format> m_data;
		u16                       m_width{1};
		u16                       m_height{1};

	public:
		explicit image_channels(u64 w, u64 h)
		{
			m_width  = static_cast<u16>(std::clamp(w, 1ull, static_cast<u64>(MAX_WIDTH)));
			m_height = static_cast<u16>(std::clamp(h, 1ull, static_cast<u64>(MAX_HEIGHT)));

			m_data.resize(m_width * m_height);
		}

		explicit image_channels(u64 w, u64 h, std::span<const color_format> pixels)
			: image_channels(w, h)
		{
			assign(pixels);
		}

		image_channels(const image_channels<4>& other)
			: image_channels(other.width(), other.height())
		{
			for (u16 y = 0; y < m_height; ++y)
			{
				for (u16 x = 0; x < m_width; ++x)
				{
					const auto& rgba_pixel = other.at(x, y);
					at(x, y)               = color_format{rgba_pixel.r, rgba_pixel.g, rgba_pixel.b, rgba_pixel.a};
				}
			}
		}

		image_channels(const image_channels<3>& other)
		requires(Channels == 3 or Channels == 4)
			: image_channels(other.width(), other.height())
		{
			for (u16 y = 0; y < m_height; ++y)
			{
				for (u16 x = 0; x < m_width; ++x)
				{
					const auto& rgb_pixel = other.at(x, y);
					at(x, y)              = color_format{rgb_pixel.r, rgb_pixel.g, rgb_pixel.b};
				}
			}
		}

		u16 width() const { return m_width; }

		u16 height() const { return m_height; }

		std::span<color_format> data() { return m_data; }

		u8 channels() const { return static_cast<u8>(Channels); }

		std::span<const color_format> data() const { return m_data; }

		std::span<const u8> raw_data() const
		{
			return std::span<const u8>{reinterpret_cast<const u8*>(m_data.data()), m_data.size() * sizeof(color_format)};
		}

		std::span<const color_format> row(u16 y) const
		{
			assert::check(y < m_height, "y-coordinate out-of-bounds");
			auto row_start = math::index_from_2d(static_cast<u16>(0), y, static_cast<u16>(m_width));
			return std::span<const color_format>{m_data.data() + row_start, m_width};
		}

		std::span<color_format> row(u16 y)
		{
			assert::check(y < m_height, "y-coordinate out-of-bounds");
			auto row_start = math::index_from_2d(static_cast<u16>(0), y, static_cast<u16>(m_width));
			return std::span<color_format>{m_data.data() + row_start, m_width};
		}

		std::span<const u8> row_bytes(u16 y) const
		{
			auto bytes = std::as_bytes(row(y));
			return {reinterpret_cast<u8*>(bytes.data()), bytes.size()};
		}

		std::span<u8> row_bytes(u16 y)
		{
			auto bytes = std::as_writable_bytes(row(y));
			return {reinterpret_cast<u8*>(bytes.data()), bytes.size()};
		}

		u64 size_in_bytes() const { return raw_data().size_bytes(); }

		void assign(std::span<const color_format> pixels)
		{
			assert::check(pixels.size() == m_data.size(), "pixel span size must match width*height");
			std::ranges::copy(pixels, m_data.begin());
		}

		color_format& at(u16 x, u16 y)
		{
			assert::check(x < m_width, "x-coordinate out-of-bounds");
			assert::check(y < m_height, "y-coordinate out-of-bounds");

			auto idx = math::index_from_2d(x, y, m_width);
			assert::check(idx < m_data.size(), "index out of bounds");
			return m_data[idx];
		}

		const color_format& at(u16 x, u16 y) const
		{
			assert::check(x < m_width, "x-coordinate out-of-bounds");
			assert::check(y < m_height, "y-coordinate out-of-bounds");

			auto idx = math::index_from_2d(x, y, m_width);
			assert::check(idx < m_data.size(), "index out of bounds");
			return m_data[idx];
		}

		color_format& operator[](u16 x, u16 y) { return at(x, y); }

		const color_format& operator[](u16 x, u16 y) const { return at(x, y); }

		bool operator==(const image_channels& other) const
		{
			if (m_width != other.m_width or m_height != other.m_height)
				return false;
			return std::ranges::equal(m_data, other.m_data);
		}
	};

	export using image_gray = image_channels<1>;
	export using image_rgb  = image_channels<3>;
	export using image_rgba = image_channels<4>;

	//


} // namespace deckard
