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
		};

		file()                       = default;
		file(file&&) noexcept        = delete;
		file(const file&)            = delete;
		file& operator=(const file&) = delete;
		file& operator=(file&&)      = delete;

		file(const std::filesystem::path filename, access flag = access::read) { open(filename, flag); }

		std::optional<std::span<u8>> open(std::filesystem::path const filename, access flag = access::read) noexcept
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

			HANDLE handle =
			  CreateFile(filename.wstring().c_str(), rw, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				close();

				dbg::println("Could not open file '{}'", filename.string());
				return {};
			}

			LARGE_INTEGER fs;
			if (GetFileSizeEx(handle, &fs) != 0)
				filesize = as<u64>(fs.QuadPart);


			HANDLE mapping = CreateFileMapping(handle, 0, page, 0, 0, nullptr);
			if (mapping == nullptr)
			{
				close();

				dbg::println("Could not create mapping for file '{}' ({})", filename.string(), PrettyBytes(filesize));
				return {};
			}

			CloseHandle(handle);
			handle = nullptr;

			raw_address = as<u8*>(MapViewOfFile(mapping, filemapping, 0, 0, 0));
			if (raw_address == nullptr)
			{
				close();

				dbg::println("Could not map file '{}'", filename.string());
				return {};
			}

			CloseHandle(mapping);
			mapping = nullptr;

			view = std::span<u8>{as<u8*>(raw_address), size()};
			return view;
		}

		auto data() const noexcept { return view; }

		u64 size() const noexcept { return filesize; }

		bool is_open() const noexcept { return raw_address != nullptr; }

		void save() { flush(); }

		void flush() { FlushViewOfFile(raw_address, 0); }

		void write(std::filesystem::path filename) { }

		void close() noexcept
		{
			flush();
			UnmapViewOfFile(raw_address);

			raw_address = nullptr;
		}

		u8& operator[](u64 index) const noexcept
		{
			assert::check(is_open(), "indexing a closed file");
			assert::check(index < filesize, std::format("indexing out-of-bounds: {} out of {}", index + 1, filesize));
			return view[index];
		}

		std::span<u8> view;
		u8*           raw_address{nullptr};
		u64           filesize{0};
	};


} // namespace deckard
