#include <windows.h>
#include <Commctrl.h>
import deckard;
using namespace deckard;
using namespace deckard::app;
import std;

std::array<unsigned char, 256> previous{0};
std::array<unsigned char, 256> current{0};

void keyboard_callback(vulkanapp& app, i32 key, i32 scancode, Action action, i32 mods)
{
	// dbg::println("key: {:#x} - {:#x}, {} - {}", key, scancode, action == Action::Up ? "UP" : "DOWN", mods);

	bool up = action == Action::Up;

	// num 6 -> 0x66, 0x4d
	// f     -> 0x46, 0x21

	if (up and key == Key::B)
	{
		current.swap(previous);
		{
			ScopeTimer<std::micro> _("state");
			GetKeyboardState(&current[0]);
		}

		auto shift = current[Key::Shift];

		int k = 0;
	}

	if (key == Key::Escape and up)
	{
		dbg::println("quit");
		app.quit();
	}

	if (key == Key::F1 and up)
	{
		app.set(Attribute::vsync);
	}

	if (up and (key == Key::F11 or key == Key::F))
	{
		app.set(Attribute::togglefullscreen);
	}

	if (key == Key::Numpad1 and up)
	{
		app.set(Attribute::gameticks, 60u);
	}
	if (key == Key::Numpad2 and up)
	{
		app.set(Attribute::gameticks, 30u);
	}

	if (key == Key::Numpad3 and up)
	{
		app.set(Attribute::gameticks, 1u);
	}
}

void fixed_update(vulkanapp&, f32 /*fixed_delta*/)
{
	//
}

void update(vulkanapp&, f32 /*delta*/)
{
	//
}

void render(vulkanapp&)
{
	//
}

namespace fs = std::filesystem;

namespace nt
{
#if 0

	class file
	{
	public:
		enum class Flags : u8
		{
			read,
			readwrite,
		};

	private:
		std::span<u8> view{};
		fs::path      filepath;
		u32           offset{0};
		HANDLE        handle{INVALID_HANDLE_VALUE};

		bool is_writeonly() const { return handle != nullptr and view.empty(); }

		bool is_open() const { return not view.empty(); }

		void setpath(fs::path p) { filepath = p; }

		auto create_impl(const fs::path filepath) -> Result<file>
		{
			setpath(filepath);

			DWORD rw     = GENERIC_READ | GENERIC_WRITE;
			DWORD create = CREATE_NEW;

			handle = CreateFileW(filepath.wstring().c_str(), rw, FILE_SHARE_READ, nullptr, create, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				auto error = GetLastError();
				if (error = ERROR_FILE_EXISTS)
				{
					close();
					return Err("File already exists: '{}'", filepath.string());
				}

				return Err("Could not open file '{}'", filepath.string());
			}


			return Ok(*this);
		}

		auto open_impl(const fs::path filepath, Flags flags = Flags::read) -> Result<file>
		{
			setpath(filepath);

			DWORD rw          = GENERIC_READ;
			DWORD page        = PAGE_READONLY;
			DWORD filemapping = FILE_MAP_READ;

			DWORD create = OPEN_EXISTING;

			if (flags == Flags::readwrite)
			{
				rw |= GENERIC_WRITE;
				page = PAGE_READWRITE;
				filemapping |= FILE_MAP_WRITE;
			}

			handle = CreateFileW(filepath.wstring().c_str(), rw, FILE_SHARE_READ, nullptr, create, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				auto error = GetLastError();
				if (error = ERROR_FILE_EXISTS)
				{
					close();
					return Err("File already exists: '{}'", filepath.string());
				}

				return Err("Could not open file '{}'", filepath.string());
			}


			LARGE_INTEGER fs;
			u64           filesize{0};
			if (GetFileSizeEx(handle, &fs) != 0)
				filesize = as<u64>(fs.QuadPart);


			HANDLE mapping = CreateFileMapping(handle, 0, page, 0, 0, nullptr);
			if (mapping == nullptr)
			{
				close();

				return Err("Could not create mapping for file '{}' ({})", filepath.string(), pretty_bytes(filesize));
			}

			CloseHandle(handle);
			handle = nullptr;


			u8* raw_address = as<u8*>(MapViewOfFile(mapping, filemapping, 0, 0, 0));
			if (raw_address == nullptr)
			{
				close();

				return Err("Could not map file '{}'", filepath.string());
			}

			CloseHandle(mapping);
			mapping = nullptr;


			view = std::span<u8>{as<u8*>(raw_address), filesize};

			return Ok(*this);
		}

		void flush() { FlushViewOfFile(view.data(), 0); }

		void close() noexcept
		{
			CloseHandle(handle);
			handle = INVALID_HANDLE_VALUE;

			flush();
			UnmapViewOfFile(view.data());
			view = {};
		}

	public:
		file() = default;

		file(file&&) noexcept = default;
		file(const file&)     = default;

		file& operator=(const file&) = default;
		file& operator=(file&&)      = default;

		~file() { close(); }

		// open existing
		auto open(const fs::path& path, access access_flag = access::read) -> Result<file>
		{

			return open_impl(path, access_flag, creation::open_existing);
		}

		// create new file
		auto create(const fs::path& path, access access_flag = access::read) -> Result<file>
		{
			return open_impl(path, access_flag, creation::create);
		}

		auto read() -> Result<std::span<u8>> { return read((u64)-1); }

		auto read(u32 size) -> Result<std::span<u8>> { return read(size, 0); }

		auto read(u64 size, u64 offset = 0) -> Result<std::span<u8>>
		{
			if (is_writeonly())
				return Err("File '{}' is opened for write-only", filepath.string());

			if (not is_open())
				return Err("File not open");
			//
			return Err("not implemented");
		}

		auto data() const -> Result<std::span<u8>>
		{
			if (not is_open())
				return Err("File is not open. Cannot give view to file");

			if (view.empty())
				return Err("There is not mapping for file '{}'", filepath.string());

			return Ok(view);
		}

		auto write(std::span<u8> input) -> Result<u32>
		{
			DWORD bytes_written{0};

			if (not is_writeonly())
				return Err("Could not write to file");

			if (handle == INVALID_HANDLE_VALUE)
			{
				CloseHandle(handle);
				return Err("Did not have a valid filehandle: '{}'", filepath.string());
			}

			WriteFile(handle, input.data(), as<u32>(input.size_bytes()), &bytes_written, nullptr);

			return bytes_written;
		}
	};

	void read(std::span<u8> output) { }
#endif


	class commandliner
	{
	private:
		std::string_view commandline;

	public:
		commandliner(std::string_view i)
			: commandline(i)
		{
		}

		void process()
		{
			//

			int x = 0;
		}
	};

} // namespace nt

int deckard_main()
{


	std::string cmdparse("-v -o\"file.txt\" -d 1024");

	nt::commandliner cmd(cmdparse);
	cmd.process();


#if 0
	{
		int                     nButtonPressed = 0;
		TASKDIALOGCONFIG        config         = {0};
		const TASKDIALOG_BUTTON buttons[]      = {
          {100, L"Update"},
          {101, L"Play now"},
        };

		const TASKDIALOG_BUTTON radiobuttons[] = {
		  {100, L"Update 2 "},
		  {101, L"Play now 2"},
		};
		config.cbSize             = sizeof(config);
		config.pszWindowTitle     = L"Deckard";
		config.hInstance          = 0;
		config.dwFlags            = TDF_USE_COMMAND_LINKS | TDF_ENABLE_HYPERLINKS;
		config.dwCommonButtons    = TDCBF_CLOSE_BUTTON;
		config.pszMainIcon        = TD_INFORMATION_ICON;
		config.pszMainInstruction = L"Taboo Builder";
		config.pszContent =
		  L"New update available, v1.0.1234.DEADBEEF\n\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings Fixed server pings Fixed server pings Fixed server pings\n"
		  L"* Update function no longer hangs\n"
		  "\n\n<a href=\"https://www.taboobuilder.com/patchnotes/\">Read more patch notes</a>\n";
		config.pButtons = buttons;
		config.cButtons = ARRAYSIZE(buttons);


		config.pfCallback = [](HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData) -> HRESULT
		{
			switch (uNotification)
			{
				case TDN_HYPERLINK_CLICKED: dbg::println("url"); break;
				default: break;
			}
			return S_OK;
		};

		using TaskDialogIndirectFunc =
		  HRESULT(const TASKDIALOGCONFIG* pTaskConfig, int* pnButton, int* pnRadioButton, BOOL* pfVerificationFlagChecked);
		TaskDialogIndirectFunc* TaskDialogIndirect = nullptr;

		HMODULE mod = LoadLibraryA("Comctl32.dll");
		if (mod)
		{
			TaskDialogIndirect = reinterpret_cast<TaskDialogIndirectFunc*>(GetProcAddress(mod, "TaskDialogIndirect"));
		}

		if (TaskDialogIndirect)
		{
			auto err = TaskDialogIndirect(&config, &nButtonPressed, NULL, NULL);
			if (SUCCEEDED(err))
			{
				switch (nButtonPressed)
				{
					case 0: dbg::println("Dialog failed"); break;
					case IDCANCEL: dbg::println("Dialog cancel"); break;
					case IDNO: dbg::println("Dialog no"); break;
					case IDRETRY: dbg::println("Dialog retry"); break;
					case IDYES: dbg::println("Dialog yes"); break;
					case 100: dbg::println("Update"); break;
					case 101: dbg::println("Play"); break;


					default: break;
				}
			}
		}

		FreeLibrary(mod);
	}
#endif
	// TODO: register key bindings to apps own enum
	//
	// enum player_movement
	// up,down,left,right, fire
	// bind(KEY_LEFT, left)
	// bind(KEY_SPACE, fire)  // both space and pad_a fires
	// bind(PAD_A, fire)

	// special enter textmode for input
	// keys.enter_text_mode()
	// end_text_mode(), // inputs keys as text?

	vulkanapp app01(
	  {.title  = "Example 01", //
	   .width  = 1280,
	   .height = 720,
	   .flags  = Attribute::vsync | Attribute::resizable});

	app01.set_title(std::format("{}", sizeof(vulkanapp)));

	app01.set_keyboard_callback(keyboard_callback);
	app01.set_fixed_update_callback(fixed_update);
	app01.set_update_callback(update);
	app01.set_render_callback(render);


	return app01.run();
}
