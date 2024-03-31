module;

#include <windows.h>


export module deckard.file;

import std;
import deckard.types;
import deckard.assert;

export namespace deckard
{

	enum class FileAccess : u8
	{
		Read,
		ReadWrite,
	};

	class Fileview final
	{
	public:
		Fileview() = default;


		Fileview(Fileview &&) noexcept        = delete;
		Fileview(const Fileview &)            = delete;
		Fileview &operator=(const Fileview &) = delete;
		Fileview &operator=(Fileview &&)      = delete;

		explicit Fileview(std::filesystem::path const filename, FileAccess rw = FileAccess::Read) noexcept { open(filename, rw); }

		bool open(std::filesystem::path const filename, FileAccess rw = FileAccess::Read) noexcept
		{
			m_access_flag      = rw;
			DWORD access       = GENERIC_READ;
			DWORD page_protect = PAGE_READONLY;

			if (m_access_flag == FileAccess::ReadWrite)
			{
				access |= GENERIC_WRITE;
				page_protect = PAGE_READWRITE;
			}

			m_filehandle =
				CreateFile(filename.wstring().c_str(), access, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			m_mappinghandle = CreateFileMapping(m_filehandle, nullptr, page_protect, 0, 0, nullptr);

			assert::if_true(m_mappinghandle != 0, "Could not open file mapping");

			map();

			return is_open();
		}

		bool is_open() const noexcept { return m_mappinghandle != 0; }

		operator bool() const noexcept { return is_open(); }

		void close() noexcept { unmap(); }

		u64 size() const noexcept
		{
			LARGE_INTEGER fs;
			if (GetFileSizeEx(m_filehandle, &fs) != 0)
				return as<u64>(fs.QuadPart);
			return 0;
		}

		template<typename T, typename U>
		T as_type() noexcept
		{
			if (auto *addr = as<U *>(map()); addr != nullptr)
				return T{addr, size()};

			return {};
		}

		auto as_stringview() noexcept { return as_type<std::string_view, char>(); }

		auto as_span() noexcept { return as_type<std::span<char>, char>(); }

		auto &operator[](u64 index) noexcept
		{
			assert::if_true(is_open(), "File view is not open");

			auto ptr = static_cast<char *>(map());
			return ptr[index];
		}

		~Fileview() { close(); }

		bool operator==(const Fileview &other) const = default;

	private:
		void *map() noexcept
		{
			assert::if_true(m_mappinghandle != 0, "File mapping is not open");

			if (m_addr != nullptr)
				return m_addr;

			DWORD access = FILE_MAP_READ;
			if (m_access_flag == FileAccess::ReadWrite)
				access |= FILE_MAP_WRITE;

			m_addr = MapViewOfFile(m_mappinghandle, access, 0, 0, 0);

			assert::if_true(m_addr != nullptr, "Map view is not valid");
			return m_addr;
		}

		void unmap() noexcept
		{
			FlushViewOfFile(m_addr, 0);
			UnmapViewOfFile(m_addr);
			CloseHandle(m_mappinghandle);
			CloseHandle(m_filehandle);
			m_addr = m_filehandle = m_mappinghandle = 0;
		}

		HANDLE     m_filehandle{0};
		HANDLE     m_mappinghandle{0};
		void      *m_addr{nullptr};
		FileAccess m_access_flag{FileAccess::Read};
	};

	class TextFile final
	{
	public:
		Fileview m_file;
	};


} // namespace deckard
