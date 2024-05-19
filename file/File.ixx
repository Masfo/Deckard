module;
#include <windows.h>

export module deckard.file;

import std;
import deckard.debug;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.helpers;

namespace deckard
{

	export class file
	{
	public:
		enum class access : u8
		{
			read,
			readwrite,
			createnew,
			overwrite,
		};

		file() = delete;

		file(file&&) noexcept        = delete;
		file(const file&)            = delete;
		file& operator=(const file&) = delete;
		file& operator=(file&&)      = delete;

		~file() { close(); }

		explicit file(const std::filesystem::path file, access flag = access::read) { open(file, flag); }

		std::optional<std::span<u8>> open(std::filesystem::path const file, access flag = access::read) noexcept
		{
			DWORD rw          = GENERIC_READ;
			DWORD page        = PAGE_READONLY;
			DWORD filemapping = FILE_MAP_READ;

			if (flag == access::readwrite)
			{
				rw |= GENERIC_WRITE;
				page = PAGE_READWRITE;
				filemapping |= FILE_MAP_WRITE;
			}

			HANDLE handle = CreateFile(file.wstring().c_str(), rw, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				close();

				dbg::println("Could not open file '{}'", file.string());
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

				dbg::println("Could not create mapping for file '{}' ({})", file.string(), PrettyBytes(filesize));
				return {};
			}

			CloseHandle(handle);
			handle = nullptr;

			u8* raw_address = as<u8*>(MapViewOfFile(mapping, filemapping, 0, 0, 0));
			if (raw_address == nullptr)
			{
				close();

				dbg::println("Could not map file '{}'", file.string());
				return {};
			}

			CloseHandle(mapping);
			mapping = nullptr;


			view = std::span<u8>{as<u8*>(raw_address), filesize};
			return view;
		}

		std::optional<std::span<u8>> data() const noexcept
		{
			if (view.empty())
				return {};
			return view;
		}

		u64 size() const noexcept { return view.size_bytes(); }

		bool is_open() const noexcept { return not view.empty(); }

		void flush() { FlushViewOfFile(view.data(), 0); }

		void close() noexcept
		{
			flush();
			UnmapViewOfFile(view.data());
			view = {};
		}

		u8& operator[](u64 index) const noexcept
		{
			assert::check(is_open(), "indexing a closed file");
			assert::check(index < view.size_bytes(), std::format("indexing out-of-bounds: {} out of {}", index + 1, view.size_bytes()));
			return view[index];
		}

		static u32 write(std::filesystem::path file, std::optional<std::span<u8>> content, access flag = access::createnew) noexcept
		{
			DWORD bytes_written{0};

			if (not content.has_value())
			{
				dbg::println("write: empty content for file '{}'", file.string());
				return bytes_written;
			}

			auto& data = content.value();


			DWORD mode = CREATE_ALWAYS;
			if (flag == access::overwrite)
				mode = CREATE_NEW;


			HANDLE handle =
			  CreateFile(file.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, mode, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				CloseHandle(handle);
				if (GetLastError() == ERROR_ALREADY_EXISTS)
					dbg::println("write: file '{}' already exists", file.string());
				else
					dbg::println("write: could not open file '{}' for writing", file.string());

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

		static u32 write(std::filesystem::path file, const std::vector<u8>& content, access flag = access::createnew) noexcept

		{
			return write(file, std::span<u8>{as<u8*>(content.data()), content.size()}, flag);
		}

	private:
		std::span<u8> view;
	};


} // namespace deckard
