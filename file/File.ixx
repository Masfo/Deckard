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
			create,
		};

		file() = delete;

		file(file&&) noexcept        = delete;
		file(const file&)            = delete;
		file& operator=(const file&) = delete;
		file& operator=(file&&)      = delete;

		~file() { close(); }

		explicit file(const std::filesystem::path filename, access flag = access::read) { open(filename, flag); }

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
			u64           filesize{0};
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

			u8* raw_address = as<u8*>(MapViewOfFile(mapping, filemapping, 0, 0, 0));
			if (raw_address == nullptr)
			{
				close();

				dbg::println("Could not map file '{}'", filename.string());
				return {};
			}

			CloseHandle(mapping);
			mapping = nullptr;

			view = std::span<u8>{as<u8*>(raw_address), filesize};
			return view;
		}

		auto data() const noexcept { return view; }

		u64 size() const noexcept { return view.size_bytes(); }

		bool is_open() const noexcept { return not view.empty(); }

		void save() { flush(); }

		void flush() { FlushViewOfFile(view.data(), 0); }

		void write(std::filesystem::path filename) { }

		void close() noexcept
		{
			flush();
			UnmapViewOfFile(view.data());
		}

		u8& operator[](u64 index) const noexcept
		{
			assert::check(is_open(), "indexing a closed file");
			assert::check(index < view.size_bytes(), std::format("indexing out-of-bounds: {} out of {}", index + 1, view.size_bytes()));
			return view[index];
		}

		std::span<u8> view;
	};


} // namespace deckard
