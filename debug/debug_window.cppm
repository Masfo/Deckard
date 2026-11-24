module;
#include <Windows.h>
#include <windowsx.h>

export module deckard.debugwindow;

import deckard.types;
import deckard.vec;
import std;

namespace deckard::dbg
{
	using namespace deckard::vec;
	namespace detail
	{
		struct state
		{
			// Shared framebuffer state is guarded by pixels_mutex.
			std::mutex         pixels_mutex;
			std::vector<u32>  pixels;
			u32               width{};
			u32               height{};
			std::int64_t      origin_x{};
			std::int64_t      origin_y{};
			i32               pan_x{};
			i32               pan_y{};
			double            zoom{1.0};
			BITMAPINFO        bmi{};
			std::atomic<bool> open{true};
			HWND              hwnd{};

			// window thread only
			std::vector<u32> snapshot;
			POINT            last{};
			bool             dragging{};

			std::jthread thread;
		};

		constexpr UINT_PTR       repaint_timer = 1;
		constexpr u32            max_framebuffer_width = 2048;
		constexpr u32            max_framebuffer_height = 2048;
		constexpr double         min_framebuffer_zoom = 0.125;
		constexpr double         max_framebuffer_zoom = 32.0;
		constexpr const wchar_t* class_name    = L"dbg_window";


		 struct text_mask
		{
			u32             w{};
			u32             h{};
			std::vector<u8> a; // coverage 0..255, top-down
		};

		constexpr int font_px = 14;

		[[nodiscard]] HFONT mono_font() noexcept
		{
			static const HFONT f = CreateFontW(
			  -font_px,
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
			  NONANTIALIASED_QUALITY,
			  FIXED_PITCH | FF_MODERN,
			  L"Consolas");
			return f;
		}

		// Renders to an off-screen DIB. No shared GDI state, so safe from any thread.
		[[nodiscard]] text_mask rasterize(std::wstring_view text)
		{
			if (text.empty())
				return {};

			constexpr UINT flags = DT_NOPREFIX | DT_EXPANDTABS | DT_NOCLIP;
			const int      len   = static_cast<int>(text.size());

			HDC     dc       = CreateCompatibleDC(nullptr);
			HGDIOBJ old_font = SelectObject(dc, mono_font());

			RECT r{};
			DrawTextW(dc, text.data(), len, &r, flags | DT_CALCRECT); // measures, handles '\n' and '\t'

			text_mask out;
			if (r.right > 0 && r.bottom > 0)
			{
				BITMAPINFO bmi{};
				bmi.bmiHeader = {
				  .biSize        = sizeof(BITMAPINFOHEADER),
				  .biWidth       = r.right,
				  .biHeight      = -r.bottom,
				  .biPlanes      = 1,
				  .biBitCount    = 32,
				  .biCompression = BI_RGB,
				};

				void*   bits = nullptr;
				HBITMAP bmp  = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
				if (bmp)
				{
					HGDIOBJ old_bmp = SelectObject(dc, bmp); // DIB sections start zeroed (black)
					SetBkMode(dc, TRANSPARENT);
					SetTextColor(dc, RGB(255, 255, 255));
					DrawTextW(dc, text.data(), len, &r, flags);
					GdiFlush();

					out.w = static_cast<u32>(r.right);
					out.h = static_cast<u32>(r.bottom);
					out.a.resize(std::size_t{out.w} * out.h);

					const auto src = std::span{static_cast<const u32*>(bits), out.a.size()};
					std::ranges::transform(src, out.a.begin(), [](u32 p) { return static_cast<u8>((p >> 8) & 0xFF); });

					SelectObject(dc, old_bmp);
					DeleteObject(bmp);
				}
			}

			SelectObject(dc, old_font);
			DeleteDC(dc);
			return out;
		}

		[[nodiscard]] const text_mask* cached_character(std::wstring_view text)
		{
			if (text.size() != 1 or text.front() >= 128)
				return nullptr;

			static std::mutex                    cache_mutex;
			static std::array<text_mask, 128>    cache;
			static std::array<bool, 128>         cached{};

			const auto index = static_cast<std::size_t>(text.front());
			std::lock_guard lock{cache_mutex};
			if (not cached[index])
			{
				cache[index]  = rasterize(text);
				cached[index] = true;
			}
			return &cache[index];
		}

		[[nodiscard]] constexpr u32 blend(u32 dst, u32 src, u32 a) noexcept
		{
			const auto mix = [&](u32 shift)
			{
				const u32 s = (src >> shift) & 0xFF;
				const u32 d = (dst >> shift) & 0xFF;
				return (s * a + d * (255 - a) + 127) / 255;
			};
			return 0xFF00'0000u | (mix(16) << 16) | (mix(8) << 8) | mix(0);
		}


		LRESULT CALLBACK wnd_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
		{
			if (msg == WM_NCCREATE)
			{
				auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
				SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
				return DefWindowProcW(h, msg, wp, lp);
			}

			auto* s = reinterpret_cast<state*>(GetWindowLongPtrW(h, GWLP_USERDATA));
			if (!s)
				return DefWindowProcW(h, msg, wp, lp);

			switch (msg)
			{
				case WM_ERASEBKGND: return 1;

				case WM_TIMER: InvalidateRect(h, nullptr, FALSE); return 0;

				case WM_PAINT:
				{
					u32        width;
					u32        height;
					i32        pan_x;
					i32        pan_y;
					double     zoom;
					BITMAPINFO bmi;
					{
						std::lock_guard lock{s->pixels_mutex};
						s->snapshot.resize(s->pixels.size());
						std::copy(s->pixels.begin(), s->pixels.end(), s->snapshot.begin());
						width  = s->width;
						height = s->height;
						pan_x  = s->pan_x;
						pan_y  = s->pan_y;
						zoom   = s->zoom;
						bmi    = s->bmi;
					}

					PAINTSTRUCT ps;
					HDC         dc = BeginPaint(h, &ps);
					const auto  scaled_width  = width == 0 ? 0 : std::max(1, static_cast<i32>(std::lround(width * zoom)));
					const auto  scaled_height = height == 0 ? 0 : std::max(1, static_cast<i32>(std::lround(height * zoom)));

					const auto right = static_cast<i32>(std::clamp<std::int64_t>(
					  static_cast<std::int64_t>(pan_x) + scaled_width, -0x8000'0000LL, 0x7FFF'FFFFLL));
					const auto bottom = static_cast<i32>(std::clamp<std::int64_t>(
					  static_cast<std::int64_t>(pan_y) + scaled_height, -0x8000'0000LL, 0x7FFF'FFFFLL));
					ExcludeClipRect(dc, pan_x, pan_y, right, bottom);
					FillRect(dc, &ps.rcPaint, static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
					SelectClipRgn(dc, nullptr);

					SetStretchBltMode(dc, COLORONCOLOR);
					StretchDIBits(
					  dc,
					  pan_x,
					  pan_y,
					  scaled_width,
					  scaled_height,
					  0,
					  0,
					  width,
					  height,
					  s->snapshot.data(),
					  &bmi,
					  DIB_RGB_COLORS,
					  SRCCOPY);

					EndPaint(h, &ps);
					return 0;
				}

				case WM_RBUTTONDOWN:
					s->dragging = true;
					s->last     = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
					SetCapture(h);
					return 0;

				case WM_RBUTTONUP: ReleaseCapture(); return 0;

				case WM_CAPTURECHANGED: s->dragging = false; return 0;

				case WM_MOUSEMOVE:
					if (s->dragging)
					{
						const POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
						{
							std::lock_guard lock{s->pixels_mutex};
							s->pan_x += p.x - s->last.x;
							s->pan_y += p.y - s->last.y;
							s->last = p;
						}
						InvalidateRect(h, nullptr, FALSE);
					}
					return 0;

				case WM_MOUSEWHEEL:
				{
					POINT cursor{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
					ScreenToClient(h, &cursor);
					const auto delta = GET_WHEEL_DELTA_WPARAM(wp);
					if (delta != 0)
					{
						std::lock_guard lock{s->pixels_mutex};
						const auto next_zoom = std::clamp(
						  s->zoom * std::pow(1.2, static_cast<double>(delta) / WHEEL_DELTA),
						  min_framebuffer_zoom,
						  max_framebuffer_zoom);
						const auto image_x = (static_cast<double>(cursor.x) - s->pan_x) / s->zoom;
						const auto image_y = (static_cast<double>(cursor.y) - s->pan_y) / s->zoom;
						s->pan_x = static_cast<i32>(std::clamp(
						  std::round(cursor.x - image_x * next_zoom), -2147483648.0, 2147483647.0));
						s->pan_y = static_cast<i32>(std::clamp(
						  std::round(cursor.y - image_y * next_zoom), -2147483648.0, 2147483647.0));
						s->zoom = next_zoom;
					}
					InvalidateRect(h, nullptr, FALSE);
					return 0;
				}

				case WM_KEYDOWN:
					if (wp == 'R')
					{
						{
							std::lock_guard lock{s->pixels_mutex};
							s->pan_x = s->pan_y = 0;
						}
						InvalidateRect(h, nullptr, FALSE);
					}
					return 0;

				case WM_CLOSE: DestroyWindow(h); return 0;

				case WM_DESTROY:
					s->open.store(false);
					KillTimer(h, repaint_timer);
					PostQuitMessage(0);
					return 0;
			}
			return DefWindowProcW(h, msg, wp, lp);
		}

		[[nodiscard]] std::wstring to_wide(std::string_view utf8)
		{
			if (utf8.empty())
				return {};
			const int    n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
			std::wstring out(static_cast<std::size_t>(n), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), n);
			return out;
		}

		[[nodiscard]] bool register_class() noexcept
		{
			static const bool ok = []
			{
				WNDCLASSEXW wc{
				  .cbSize        = sizeof(WNDCLASSEXW),
				  .lpfnWndProc   = wnd_proc,
				  .hInstance     = GetModuleHandleW(nullptr),
				  .hCursor       = LoadCursorW(nullptr, IDC_ARROW),
				  .lpszClassName = class_name,
				};
				return RegisterClassExW(&wc) != 0;
			}();
			return ok;
		}

		// Runs on the window thread: create, report result, pump until closed.
		void window_thread(state* s, std::wstring title, std::promise<bool> created)
		{
			if (not register_class())
			{
				created.set_value(false);
				return;
			}

			constexpr DWORD style = WS_OVERLAPPEDWINDOW;
			RECT            r{0, 0, static_cast<LONG>(s->width), static_cast<LONG>(s->height)};
			AdjustWindowRect(&r, style, FALSE);

			s->hwnd = CreateWindowExW(
			  0,
			  class_name,
			  title.c_str(),
			  style,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  r.right - r.left,
			  r.bottom - r.top,
			  nullptr,
			  nullptr,
			  GetModuleHandleW(nullptr),
			  s);
			if (!s->hwnd)
			{
				created.set_value(false);
				return;
			}

			SetTimer(s->hwnd, repaint_timer, 16, nullptr);
			ShowWindow(s->hwnd, SW_SHOW);
			created.set_value(true);

			MSG msg;
			while (GetMessageW(&msg, nullptr, 0, 0) > 0)
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
	} // namespace detail


	export [[nodiscard]] constexpr u32 rgb(u8 r, u8 g, u8 b) noexcept
	{
		return 0xFF00'0000u | (u32{r} << 16) | (u32{g} << 8) | u32{b};
	}


	class window
	{
	private:
		explicit window(std::unique_ptr<detail::state> s) noexcept
			: m_state{std::move(s)}
		{
		}

		std::unique_ptr<detail::state> m_state;

		bool resize_framebuffer_locked(std::int64_t left, std::int64_t top, std::int64_t right, std::int64_t bottom)
		{
			auto& s = *m_state;
			const auto min_x = std::min(s.origin_x, left);
			const auto min_y = std::min(s.origin_y, top);
			const auto max_x = std::max(s.origin_x + s.width, right);
			const auto max_y = std::max(s.origin_y + s.height, bottom);
			const auto new_width = max_x - min_x;
			const auto new_height = max_y - min_y;
			if (new_width > detail::max_framebuffer_width or new_height > detail::max_framebuffer_height)
				return false;
			if (new_width == s.width and new_height == s.height)
				return true;

			const auto width_size = static_cast<std::size_t>(new_width);
			const auto height_size = static_cast<std::size_t>(new_height);
			if (height_size == 0 or width_size > s.pixels.max_size() / height_size)
				return false;
			const auto pixel_count = width_size * height_size;

			const auto shift_x = s.origin_x - min_x;
			const auto shift_y = s.origin_y - min_y;
			const auto new_pan_x = static_cast<std::int64_t>(s.pan_x) - std::llround(shift_x * s.zoom);
			const auto new_pan_y = static_cast<std::int64_t>(s.pan_y) - std::llround(shift_y * s.zoom);
			if (new_pan_x < -0x8000'0000LL or new_pan_x > 0x7FFF'FFFFLL or new_pan_y < -0x8000'0000LL
				or new_pan_y > 0x7FFF'FFFFLL)
				return false;

			std::vector<u32> expanded(pixel_count, 0xFF00'0000u);
			for (std::size_t row = 0; s.width != 0 and row < s.height; ++row)
			{
				const auto source = row * s.width;
				const auto destination = (row + static_cast<std::size_t>(shift_y)) * width_size
				  + static_cast<std::size_t>(shift_x);
				std::copy_n(s.pixels.begin() + source, s.width, expanded.begin() + destination);
			}

			s.pixels.swap(expanded);
			s.origin_x = min_x;
			s.origin_y = min_y;
			s.width    = static_cast<u32>(new_width);
			s.height   = static_cast<u32>(new_height);
			s.pan_x    = static_cast<i32>(new_pan_x);
			s.pan_y    = static_cast<i32>(new_pan_y);
			s.bmi.bmiHeader.biWidth  = static_cast<LONG>(s.width);
			s.bmi.bmiHeader.biHeight = -static_cast<LONG>(s.height);
			return true;
		}

		void write_pixel_locked(const ivec2 pos, const u32 color)
		{
			const auto x = static_cast<std::size_t>(static_cast<std::int64_t>(pos.x) - m_state->origin_x);
			const auto y = static_cast<std::size_t>(static_cast<std::int64_t>(pos.y) - m_state->origin_y);
			m_state->pixels[y * m_state->width + x] = color;
		}

	public:
		[[nodiscard]] static auto create(u32 width, u32 height, std::string_view title) -> std::expected<window, std::string>
		{
			if (width > detail::max_framebuffer_width or height > detail::max_framebuffer_height)
				return std::unexpected{"Framebuffer dimensions cannot exceed 2048x2048"};

			auto s    = std::make_unique<detail::state>();
			s->width  = width;
			s->height = height;
			s->pixels.assign(std::size_t{width} * height, 0xFF00'0000u);
			s->snapshot.resize(s->pixels.size());
			s->bmi.bmiHeader = {
			  .biSize        = sizeof(BITMAPINFOHEADER),
			  .biWidth       = static_cast<LONG>(width),
			  .biHeight      = -static_cast<LONG>(height), // top-down
			  .biPlanes      = 1,
			  .biBitCount    = 32,
			  .biCompression = BI_RGB,
			};

			std::promise<bool> created;
			auto               result = created.get_future();
			s->thread = std::jthread{detail::window_thread, s.get(), detail::to_wide(title), std::move(created)};

			if (!result.get())
				return std::unexpected{"Failed to create window"};

			return window{std::move(s)};
		}

		window(window&&) noexcept            = default;
		window& operator=(window&&) noexcept = default;
		window(const window&)                = delete;
		window& operator=(const window&)     = delete;

		~window() { wait(); }

		// Safe to call from any thread. The framebuffer expands to contain out-of-bounds coordinates.
		void pixel(ivec2 pos, u32 color)
		{
			std::lock_guard lock{m_state->pixels_mutex};
			if (resize_framebuffer_locked(
				  pos.x, pos.y, static_cast<std::int64_t>(pos.x) + 1, static_cast<std::int64_t>(pos.y) + 1))
				write_pixel_locked(pos, color);
		}

		// Draws text with its top-left at (x, y), in image coordinates, so it pans with the image.
		void draw_text(ivec2 pos, u32 color, std::string_view text)
		{
			const auto wide_text  = detail::to_wide(text);
			auto       mask       = detail::cached_character(wide_text);
			const auto rasterized = mask ? detail::text_mask{} : detail::rasterize(wide_text);
			if (not mask)
				mask = &rasterized;
			if (mask->w == 0 or mask->h == 0)
				return;

			std::lock_guard lock{m_state->pixels_mutex};
			if (not resize_framebuffer_locked(
				  pos.x,
				  pos.y,
				  static_cast<std::int64_t>(pos.x) + mask->w,
				  static_cast<std::int64_t>(pos.y) + mask->h))
				return;

			for (u32 j = 0; j < mask->h; ++j)
			{
				for (u32 i = 0; i < mask->w; ++i)
				{
					const u32 a  = mask->a[std::size_t{j} * mask->w + i];
					if (a == 0)
						continue;

					const auto px = static_cast<std::size_t>(static_cast<std::int64_t>(pos.x) + i - m_state->origin_x);
					const auto py = static_cast<std::size_t>(static_cast<std::int64_t>(pos.y) + j - m_state->origin_y);
					auto& dst = m_state->pixels[py * m_state->width + px];
					dst = detail::blend(dst, color, a);
				}
			}
		}

		template<class... Args>
		void print(ivec2 pos, u32 color, std::format_string<Args...> fmt, Args&&... args)
		{
			draw_text(pos, color, std::format(fmt, std::forward<Args>(args)...));
		}

		void clear(u32 color)
		{
			std::lock_guard lock{m_state->pixels_mutex};
			std::ranges::fill(m_state->pixels, color);
		}

		[[nodiscard]] std::expected<void, std::string> framebuffer(u32 width, u32 height)
		{
			if (width > detail::max_framebuffer_width or height > detail::max_framebuffer_height)
				return std::unexpected{"Framebuffer dimensions cannot exceed 2048x2048"};

			{
				std::lock_guard lock{m_state->pixels_mutex};
				auto&           s = *m_state;
				std::vector<u32> resized(std::size_t{width} * height, 0xFF00'0000u);
				const auto       copy_width  = std::min(width, s.width);
				const auto       copy_height = std::min(height, s.height);
				for (u32 row = 0; row < copy_height; ++row)
					std::copy_n(
					  s.pixels.begin() + std::size_t{row} * s.width,
					  copy_width,
					  resized.begin() + std::size_t{row} * width);

				s.pixels.swap(resized);
				s.width  = width;
				s.height = height;
				s.bmi.bmiHeader.biWidth  = static_cast<LONG>(width);
				s.bmi.bmiHeader.biHeight = -static_cast<LONG>(height);
			}
			InvalidateRect(m_state->hwnd, nullptr, FALSE);
			return {};
		}


		void line(ivec2 p0, ivec2 p1, u32 color)
		{
			std::lock_guard lock{m_state->pixels_mutex};
			if (not resize_framebuffer_locked(
				  std::min(p0.x, p1.x),
				  std::min(p0.y, p1.y),
				  static_cast<std::int64_t>(std::max(p0.x, p1.x)) + 1,
				  static_cast<std::int64_t>(std::max(p0.y, p1.y)) + 1))
				return;

			const auto      dx  = std::abs(p1.x - p0.x);
			const auto      dy  = std::abs(p1.y - p0.y);
			const auto      sx  = p0.x < p1.x ? 1 : -1;
			const auto      sy  = p0.y < p1.y ? 1 : -1;
			auto            err = dx - dy;
			while (true)
			{
				write_pixel_locked(p0, color);
				if (p0 == p1)
					break;
				const auto e2 = 2 * err;
				if (e2 > -dy)
				{
					err -= dy;
					p0.x += sx;
				}
				if (e2 < dx)
				{
					err += dx;
					p0.y += sy;
				}
			}
		}

		[[nodiscard]] bool is_open() const noexcept { return m_state->open.load(); }

		[[nodiscard]] u32 width() const
		{
			std::lock_guard lock{m_state->pixels_mutex};
			return m_state->width;
		}

		[[nodiscard]] u32 height() const
		{
			std::lock_guard lock{m_state->pixels_mutex};
			return m_state->height;
		}

		// Block the calling thread until the window is closed (no-op if already closed).
		void wait() noexcept
		{
			if (m_state && m_state->thread.joinable())
				m_state->thread.join();
		}

		// Ask the window to close; wait() / the destructor then return promptly.
		void close() noexcept
		{
			if (m_state && m_state->open.load())
				PostMessageW(m_state->hwnd, WM_CLOSE, 0, 0);
		}
	};


	 class global_window
	{
	public:
		// Optional: pick size/title. Only has an effect if called before the first draw call.
		bool open(u32 width, u32 height, std::string_view title = "debug view")
		{
			std::call_once(m_once, [&] { init(width, height, title); });
			return m_win.has_value();
		}

		void pixel(ivec2 pos, u32 color)
		{
			if (auto* w = get())
				w->pixel(pos, color);
		}

		void line(ivec2 p0, ivec2 p1, u32 color)
		{
			if (auto* w = get())
				w->line(p0, p1, color);
		}

		void draw_text(ivec2 pos, u32 color, std::string_view text)
		{
			if (auto* w = get())
				w->draw_text(pos, color, text);
		}

		template<class... Args>
		void print(ivec2 pos, u32 color, std::format_string<Args...> fmt, Args&&... args)
		{
			if (auto* w = get())
				w->print(pos, color, fmt, std::forward<Args>(args)...);
		}

		void clear(u32 color)
		{
			if (auto* w = get())
				w->clear(color);
		}

		[[nodiscard]] std::expected<void, std::string> framebuffer(u32 width, u32 height)
		{
			if (width > detail::max_framebuffer_width or height > detail::max_framebuffer_height)
				return std::unexpected{"Framebuffer dimensions cannot exceed 2048x2048"};
			if (auto* w = get())
				return w->framebuffer(width, height);
			return std::unexpected{"Failed to create window"};
		}

		[[nodiscard]] bool is_open()
		{
			auto* w = get();
			return w && w->is_open();
		}

		void close()
		{
			if (m_win)
				m_win->close();
		}

		void wait()
		{
			if (m_win)
				m_win->wait();
		}

		void keep_open()
		{
			if (not m_win or not m_win->is_open())
				return;

			while (m_win->is_open())
				std::this_thread::sleep_for(std::chrono::milliseconds{16});
		}

		// Blocks at program exit until the user closes the window (only if one was ever opened).
		~global_window() { wait(); }

	private:
		[[nodiscard]] window* get()
		{
			std::call_once(m_once, [&] { init(512, 512, "debug view"); });
			return m_win ? &*m_win : nullptr;
		}

		void init(u32 width, u32 height, std::string_view title)
		{
			if (auto w = window::create(width, height, title))
				m_win.emplace(std::move(*w));
		}

		std::once_flag        m_once;
		std::optional<window> m_win;
	};

	export  global_window win;
} // namespace deckard::dbg
