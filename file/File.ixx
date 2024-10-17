module;
#include <windows.h>

export module deckard.file;

import std;
import deckard.debug;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.helpers;
import deckard.win32;
namespace fs = std::filesystem;

namespace deckard
{

	// virtualfile
	// fills buffers, if full dump to disk

	export class file
	{
	private:
		HANDLE   handle{nullptr};
		fs::path path;

	public:
		enum class access : u8
		{
			read,
			readwrite,
		};

		file() = default;

		explicit file(const fs::path& p)
			: path(p)
		{
			open(p);
		}

		file(file&&)      = delete;
		file(const file&) = delete;

		file& operator=(const file&) = delete;
		file& operator=(file&&)      = delete;

		~file() { close(); }

		bool open(const fs::path& p)
		{
			path = p;

#ifdef _DEBUG
			constexpr u32 sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
#else
			constexpr u32 sharemode = 0;
#endif

			handle = CreateFileW(
			  path.generic_wstring().data(),
			  GENERIC_READ | GENERIC_WRITE,
			  sharemode,
			  0,
			  OPEN_ALWAYS,
			  FILE_ATTRIBUTE_NORMAL | FILE_READ_ATTRIBUTES,
			  0);
			if (handle == INVALID_HANDLE_VALUE)
			{
				dbg::eprintln("open('{}'): {}", system::from_wide(path.generic_wstring()), system::get_windows_error());
				return false;
			}


			return true;
		}

		i64 size() const
		{
			WIN32_FILE_ATTRIBUTE_DATA fad{};
			if (0 == GetFileAttributesExW(path.generic_wstring().data(), GetFileExInfoStandard, &fad))
			{
				dbg::eprintln("size('{}'): {}", system::from_wide(path.generic_wstring()), system::get_windows_error());
				return -1;
			}

			LARGE_INTEGER size{};
			size.HighPart = fad.nFileSizeHigh;
			size.LowPart  = fad.nFileSizeLow;
			return size.QuadPart;
		}

		void flush() const { FlushFileBuffers(handle); }

		void close() const
		{
			flush();
			CloseHandle(handle);
		}

		i64 seek(i64 offset) const
		{
			LARGE_INTEGER pos{.QuadPart = offset};
			SetFilePointerEx(handle, pos, nullptr, FILE_BEGIN);
			return offset;
		}

		u64 seek_write(const std::span<u8> buffer, size_t size, i64 offset) const
		{

			i64 old_offset = seek(offset);
			u64 written    = write(buffer, size);
			seek(old_offset);

			return written;
		}

		// TODO: writing advances offset automatically

		u64 write(const std::span<u8> buffer, size_t size = 0) const
		{
			DWORD written{};


			if (size == 0 or size > buffer.size())
				size = buffer.size();


			if (0 == WriteFile(handle, buffer.data(), as<DWORD>(size), &written, nullptr))
				return 0;

			flush();
			return written;
		}

		u64 seek_read(std::span<u8> buffer, u32 size, i64 offset)

		{
			i64 old_offset = seek(offset);
			u64 readcount  = read(buffer, size);
			seek(old_offset);

			return readcount;
		}

		u64 read(std::span<u8> buffer, u32 size)
		{

			if (size == 0 or size > buffer.size_bytes())
				size = as<u32>(buffer.size_bytes());

			DWORD read{0};

			ReadFile(handle, buffer.data(), size, &read, nullptr);

			return read;
		}
	};

	export class filemap
	{
	public:
		enum class access : u8
		{
			read,
			readwrite,
			createnew,
			overwrite,
		};

		filemap() = default;

		filemap(filemap&&)      = delete;
		filemap(const filemap&) = delete;

		filemap& operator=(const filemap&) = delete;
		filemap& operator=(filemap&&)      = delete;

		~filemap() { close(); }

		explicit filemap(const std::filesystem::path file, access flag = access::read) { open(file, flag); }

		std::optional<std::span<u8>> open(fs::path const file, access flag = access::read)
		{
			filepath          = file;
			DWORD rw          = GENERIC_READ;
			DWORD page        = PAGE_READONLY;
			DWORD filemapping = FILE_MAP_READ;

			if (flag == access::readwrite)
			{
				rw |= GENERIC_WRITE;
				page = PAGE_READWRITE;
				filemapping |= FILE_MAP_WRITE;
			}

			HANDLE handle =
			  CreateFileW(filepath.wstring().c_str(), rw, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				close();

				dbg::println("Could not open file '{}'", system::from_wide(filepath.wstring()).c_str());
				return {};
			}

			LARGE_INTEGER fs;
			u64           filesize{0};
			if (GetFileSizeEx(handle, &fs) != 0)
				filesize = as<u64>(fs.QuadPart);


			HANDLE mapping = CreateFileMapping(handle, 0, page, 0, 0, nullptr);
			if (mapping == nullptr)
			{
				close();

				dbg::println(
				  "Could not create mapping for file '{}' ({})", system::from_wide(filepath.wstring()).c_str(), pretty_bytes(filesize));
				return {};
			}

			CloseHandle(handle);
			handle = nullptr;

			u8* raw_address = as<u8*>(MapViewOfFile(mapping, filemapping, 0, 0, 0));
			if (raw_address == nullptr)
			{
				close();

				dbg::println("Could not map file '{}'", system::from_wide(filepath.wstring()).c_str());
				return {};
			}

			CloseHandle(mapping);
			mapping = nullptr;


			view = std::span<u8>{as<u8*>(raw_address), filesize};
			return view;
		}

		std::vector<u8> data() const
		{
			std::vector<u8> ret;
			ret.reserve(view.size());

			std::ranges::copy_n(view.data(), view.size(), std::back_inserter(ret));
			return ret;
		}

		std::optional<std::span<u8>> opt_data() const
		{
			if (view.empty())
				return {};
			return view;
		}

		u64 size() const { return view.size_bytes(); }

		bool is_open() const { return not view.empty(); }

		void flush() { FlushViewOfFile(view.data(), 0); }

		fs::path name() const { return filepath; }

		void close()
		{
			flush();
			UnmapViewOfFile(view.data());
			view = {};
		}

		u8& operator[](u64 index) const
		{
			assert::check(is_open(), "indexing a closed file");
			assert::check(index < view.size_bytes(), std::format("indexing out-of-bounds: {} out of {}", index + 1, view.size_bytes()));
			return view[index];
		}

		static u32 write(std::filesystem::path file, std::optional<std::span<u8>> content, access flag = access::createnew)
		{
			DWORD bytes_written{0};

			if (not content.has_value())
			{
				dbg::println("write: empty content for file '{}'", system::from_wide(file.wstring()).c_str());
				return bytes_written;
			}

			const auto& data = content.value();


			DWORD mode = CREATE_ALWAYS;
			if (flag == access::overwrite)
				mode = CREATE_NEW;


			HANDLE handle =
			  CreateFile(file.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, mode, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				CloseHandle(handle);
				if (GetLastError() == ERROR_ALREADY_EXISTS)
					dbg::println("write: file '{}' already exists", system::from_wide(file.wstring()).c_str());
				else
					dbg::println("write: could not open file '{}' for writing", system::from_wide(file.wstring()).c_str());

				return bytes_written;
			}
			if (!WriteFile(handle, data.data(), as<u32>(data.size_bytes()), &bytes_written, nullptr))
			{
				CloseHandle(handle);

				return bytes_written;
			}

			CloseHandle(handle);

			return bytes_written;
		}

		static u32 write(std::filesystem::path file, const u8* content, u32 content_len, access flag = access::createnew)
		{
			return write(file, std::span<u8>{as<u8*>(content), content_len}, flag);
		}

		static u32 write(std::filesystem::path file, const std::vector<u8>& content, access flag = access::createnew)

		{
			return write(file, std::span<u8>{as<u8*>(content.data()), content.size()}, flag);
		}

	private:
		std::span<u8> view;
		fs::path      filepath;
	};


} // namespace deckard
