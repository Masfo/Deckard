export module deckard.image;

import std;
import deckard.as;
import deckard.types;
import deckard.colors;
import deckard.assert;
import deckard.math.utils;
import deckard.utils.hash;
import deckard.file;
import deckard.serializer;
import deckard.helpers;
import deckard.zstd;
import deckard.debug;
import deckard.timers;

export import :common;
export import :bmp;
export import :dif;
export import :png;
export import :qoi;

namespace fs = std::filesystem;

namespace deckard
{


	export std::expected<image_rgb, std::string> convert_rgba_to_rgb(const image_rgba& img)
	{
		const u16 width  = img.width();
		const u16 height = img.height();

		if (width == 0 or height == 0)
			return std::unexpected("convert_rgba_to_rgb: image has zero dimensions");

		image_rgb rgb_img(width, height);
		for (u16 y = 0; y < height; ++y)
		{
			for (u16 x = 0; x < width; ++x)
			{
				const auto& rgba_pixel = img.at(x, y);
				rgb_img.at(x, y)       = rgb{rgba_pixel.r, rgba_pixel.g, rgba_pixel.b};
			}
		}
		return rgb_img;
	}

	bool per_pixel_compare(const image_rgb& lhs, const image_rgb& img2)
	{
		if (lhs.width() != img2.width() or lhs.height() != img2.height())
			return false;

		for (u16 y = 0; y < lhs.height(); ++y)
		{
			for (u16 x = 0; x < lhs.width(); ++x)
			{
				const auto& p1 = lhs.at(x, y);
				const auto& p2 = img2.at(x, y);
				if (p1.r != p2.r || p1.g != p2.g || p1.b != p2.b)
					return false;
			}
		}
		return true;
	}

	export void dump_to_ppm(const image_rgba& img, const std::filesystem::path& path)
	{
		std::ofstream f(path, std::ios::binary);
		if (not f)
			return;

		f << "P6\n" << img.width() << ' ' << img.height() << "\n255\n";

		auto pixels = img.raw_data();

		// RGBA -> RGB, drop alpha
		std::vector<u8> rgb;
		rgb.reserve(static_cast<size_t>(img.width()) * img.height() * 3);

		for (size_t i = 0; i < pixels.size(); i += 3)
		{
			rgb.push_back(pixels[i + 0]);
			rgb.push_back(pixels[i + 1]);
			rgb.push_back(pixels[i + 2]);
		}

		f.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
	}

	export void dump_to_ppm(const image_rgb& img, const std::filesystem::path& path) { 
		return dump_to_ppm(image_rgba{img}, path);
	}

}; // namespace deckard
