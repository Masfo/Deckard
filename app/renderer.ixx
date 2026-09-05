module;
#include <Windows.h>

export module deckard.app2:renderer;

import std;
import deckard.types;
import deckard.colors;

namespace deckard::app
{
	export class renderer
	{
	private:
		f32 hue{0.0f};

	public:
		void update(f32 delta_time)
		{
			hue += 90.0f * delta_time;
			hue = std::fmod(hue, 360.0f);
			if (hue < 0.0f)
				hue += 360.0f;
		}

		void paint(HWND hwnd) const
		{
			RECT client_rect{};
			GetClientRect(hwnd, &client_rect);
			const LONG w = client_rect.right - client_rect.left;
			const LONG h = client_rect.bottom - client_rect.top;

			if (w <= 0 || h <= 0)
				return;

			auto rgb0 = to_rgb(hue, 1.0f, 1.0f);
			auto rgb1 = to_rgb(std::fmod(hue + 90.0f, 360.0f), 1.0f, 1.0f);
			auto rgb2 = to_rgb(std::fmod(hue + 180.0f, 360.0f), 1.0f, 1.0f);
			auto rgb3 = to_rgb(std::fmod(hue + 270.0f, 360.0f), 1.0f, 1.0f);

			TRIVERTEX vertices[4] = {
			  {0,
			   0,
			   static_cast<COLOR16>(rgb0[0] << 8),
			   static_cast<COLOR16>(rgb0[1] << 8),
			   static_cast<COLOR16>(rgb0[2] << 8),
			   0},
			  {w,
			   0,
			   static_cast<COLOR16>(rgb1[0] << 8),
			   static_cast<COLOR16>(rgb1[1] << 8),
			   static_cast<COLOR16>(rgb1[2] << 8),
			   0},
			  {w,
			   h,
			   static_cast<COLOR16>(rgb2[0] << 8),
			   static_cast<COLOR16>(rgb2[1] << 8),
			   static_cast<COLOR16>(rgb2[2] << 8),
			   0},
			  {0,
			   h,
			   static_cast<COLOR16>(rgb3[0] << 8),
			   static_cast<COLOR16>(rgb3[1] << 8),
			   static_cast<COLOR16>(rgb3[2] << 8),
			   0}};

			GRADIENT_TRIANGLE triangles[2] = {{0, 1, 2}, {0, 2, 3}};

			HDC hdc = GetDC(hwnd);
			GradientFill(hdc, vertices, 4, triangles, 2, GRADIENT_FILL_TRIANGLE);
			ReleaseDC(hwnd, hdc);
		}
	};
} // namespace deckard::app
