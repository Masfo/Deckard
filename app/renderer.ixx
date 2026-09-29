module;
#include <Windows.h>

export module deckard.app2:renderer;

import std;
import deckard.types;
import deckard.colors;
import deckard.debug;

export namespace deckard::app2
{

	export class renderer2;

	using render_callback = std::move_only_function<void(renderer2* render, f32 delta_time) noexcept>;

	export class renderer2
	{
	private:
		extent<u16> size{1920, 1080};
		f32         hue{0.0f};
		HWND        handle{nullptr};

		HDC     back_dc{nullptr};
		HBITMAP back_bitmap{nullptr};
		HBITMAP back_bitmap_old{nullptr};

	private:
		void create_backbuffer()
		{
			destroy_backbuffer();

			HDC window_dc   = GetDC(handle);
			back_dc         = CreateCompatibleDC(window_dc);
			back_bitmap     = CreateCompatibleBitmap(window_dc, size.width, size.height);
			back_bitmap_old = static_cast<HBITMAP>(SelectObject(back_dc, back_bitmap));
			ReleaseDC(handle, window_dc);
		}

		void destroy_backbuffer()
		{
			if (back_dc)
			{
				SelectObject(back_dc, back_bitmap_old);
				DeleteObject(back_bitmap);
				DeleteDC(back_dc);
				back_dc         = nullptr;
				back_bitmap     = nullptr;
				back_bitmap_old = nullptr;
			}
		}

		void update(f32 delta_time)
		{
			hue += 90.0f * delta_time;
			hue = std::fmod(hue, 360.0f);
			if (hue < 0.0f)
				hue += 360.0f;
		}

		// TODO: generic draw quad, with size and color hues
		void paint_background() const
		{
			auto w = size.width;
			auto h = size.height;

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
			GradientFill(back_dc, vertices, 4, triangles, 2, GRADIENT_FILL_TRIANGLE);
		}

		void clear(COLORREF color) const noexcept
		{
			if (handle == nullptr)
				return;

			HDC    hdc = GetDC(handle);
			RECT   rect{0, 0, size.width, size.height};
			HBRUSH brush = CreateSolidBrush(color);
			FillRect(hdc, &rect, brush);
			DeleteObject(brush);
			ReleaseDC(handle, hdc);
		}

	public:
		render_callback on_render;

	public:
		void initialize(HWND hWnd, extent<u16> initial_size)
		{
			handle = hWnd;
			size   = initial_size;
			dbg::println("renderer: initialized with size {}x{}", size.width, size.height);
			create_backbuffer();
		}

		void deinitialize()
		{
			destroy_backbuffer();
			dbg::println("renderer: deinitialized");
		}

		HWND get_handle() const { return handle; }

		void resize(extent<u16> new_size) { notify_size(new_size); }

		extent<u16> get_size() const { return size; }

		void notify_size(extent<u16> new_size)
		{
			size = new_size;
			create_backbuffer();
		}

		void begin_frame()
		{
		}

		void end_frame() 
		{
			HDC window_dc = GetDC(handle);
			BitBlt(window_dc, 0, 0, size.width, size.height, back_dc, 0, 0, SRCCOPY);
			ReleaseDC(handle, window_dc);
		}

		void render(f32 delta)
		{
			update(delta);

			begin_frame();
			paint_background();

			_ = invoke_if(on_render, this, delta);
			end_frame();
		}

		void clear() { clear(RGB(0, 0, 0)); }

		// Solid-color line with adjustable thickness
		void draw_line(f32 x0, f32 y0, f32 x1, f32 y1, const std::array<u8, 3>& color, f32 thickness = 1.0f) const
		{
			LOGBRUSH brush{.lbStyle = BS_SOLID, .lbColor = RGB(color[0], color[1], color[2])};

			DWORD pen_width = static_cast<DWORD>(std::max(1.0f, thickness));
			HPEN  pen       = ExtCreatePen(
			  PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, pen_width, &brush, 0, nullptr);
			HPEN old_pen = static_cast<HPEN>(SelectObject(back_dc, pen));

			MoveToEx(back_dc, static_cast<int>(x0), static_cast<int>(y0), nullptr);
			LineTo(back_dc, static_cast<int>(x1), static_cast<int>(y1));

			SelectObject(back_dc, old_pen);
			DeleteObject(pen);
		}

		void draw_sprite(f32 x, f32 y,std::array<f32,2> sprite_size, const std::array<u8, 3>& color) const
		{
			auto x0 = static_cast<LONG>(x);
			auto y0 = static_cast<LONG>(y);
			auto x1 = x0 + static_cast<LONG>(sprite_size[0]);
			auto y1 = y0 + static_cast<LONG>(sprite_size[1]);

			COLOR16 r = static_cast<COLOR16>(color[0] << 8);
			COLOR16 g = static_cast<COLOR16>(color[1] << 8);
			COLOR16 b = static_cast<COLOR16>(color[2] << 8);

			TRIVERTEX vertices[4] = {
			  {x0, y0, r, g, b, 0},
			  {x1, y0, r, g, b, 0},
			  {x1, y1, r, g, b, 0},
			  {x0, y1, r, g, b, 0},
			};

			GRADIENT_TRIANGLE triangles[2] = {{0, 1, 2}, {0, 2, 3}};
			GradientFill(back_dc, vertices, 4, triangles, 2, GRADIENT_FILL_TRIANGLE);
		}

		void draw_text(f32 x, f32 y, std::string_view text, const std::array<u8, 3>& color, int font_size = 18) const
		{
			if (text.empty())
				return;

			const int len = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
			if (len <= 0)
				return;

			std::wstring wide(static_cast<usize>(len), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), len);

			// negative height = character height in pixels
			HFONT font = CreateFontW(
			  -font_size,
			  0,
			  0,
			  0,
			  FW_NORMAL,
			  FALSE,
			  FALSE,
			  FALSE,
			  DEFAULT_CHARSET,
			  OUT_TT_PRECIS,
			  CLIP_DEFAULT_PRECIS,
			  CLEARTYPE_QUALITY,
			  DEFAULT_PITCH | FF_DONTCARE,
			  L"Segoe UI");

			const auto old_font  = SelectObject(back_dc, font);
			const auto old_color = SetTextColor(back_dc, RGB(color[0], color[1], color[2]));
			const auto old_mode  = SetBkMode(back_dc, TRANSPARENT);

			TextOutW(back_dc, static_cast<int>(x), static_cast<int>(y), wide.c_str(), static_cast<int>(wide.size()));

			SetBkMode(back_dc, old_mode);
			SetTextColor(back_dc, old_color);
			SelectObject(back_dc, old_font);
			DeleteObject(font); // deselected on the line above, so safe to delete
		}
	};
} // namespace deckard::app2
