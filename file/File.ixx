module;
#include <windows.h>

export module deckard.file;

import std;
import deckard.debug;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.helpers;
import deckard.platform;
import deckard.helpers;
import deckard.stringhelper;

namespace fs = std::filesystem;

namespace deckard::file
{
	// TODO: remove many imports, more standalone
	// returns bool -> std::expected<bool, std::string>
	//
	// virtualfile
	// fills buffers, if full dump to disk

	// MemoryBuffer -> flushed to disk
	// read file -> memorybuffer

	// membuf[index]


	export enum class writemode {
		createnew,
		append,
		overwrite,
	};

	namespace impl
	{

		using return_type = std::expected<u32, std::string>;

		// write impl
		template<typename T>
		return_type write_impl(fs::path file, const std::span<T> content, size_t content_size, writemode writemode)
		{
			DWORD bytes_written{0};
			DWORD mode   = CREATE_ALWAYS;
			DWORD access = GENERIC_WRITE;

			file         = std::filesystem::absolute(file);
			content_size = std::min(content_size, content.size_bytes());

			if (writemode == writemode::createnew)
			{
				mode = CREATE_NEW;
			}
			else if (writemode == writemode::append)
			{
				if (not fs::exists(file))
					mode = CREATE_NEW;
				else
					mode = OPEN_ALWAYS;

				access = FILE_APPEND_DATA;
			}
			else if (writemode == writemode::overwrite)
			{
				mode = CREATE_ALWAYS;
			}


			HANDLE handle = CreateFileW(file.wstring().c_str(), access, FILE_SHARE_READ, nullptr, mode, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				if (platform::get_error() == ERROR_FILE_EXISTS)
				{
					return std::unexpected(std::format("write_file: file '{}' already exists. Maybe add overwrite flag?",
													   platform::string_from_wide(file.wstring()).c_str()));
				}


				return std::unexpected(
				  std::format("write_file: could not open file '{}' for writing", platform::string_from_wide(file.wstring()).c_str()));
			}

			if (not WriteFile(handle, content.data(), as<DWORD>(content_size), &bytes_written, nullptr))
			{
				CloseHandle(handle);
				return std::unexpected(
				  std::format("write_file: could not write to file '{}'", platform::string_from_wide(file.wstring()).c_str()));
			}

			CloseHandle(handle);

			if (bytes_written < content_size)
				return std::unexpected(std::format(
				  "write_file: wrote partial {}/{} to file '{}'",
				  bytes_written,
				  content_size,
				  platform::string_from_wide(file.wstring()).c_str()));


			return bytes_written;
		}

		template<typename T>
		return_type write_impl(fs::path file, const std::span<T> content, writemode mode)
		{
			return write_impl(file, content, content.size_bytes(), mode);
		}

		// append impl
		template<typename T>
		return_type append_impl(fs::path file, const std::span<T> content, size_t content_size)
		{
			return write_impl(file, content, content_size, writemode::append);
		}

		template<typename T>
		return_type append_impl(fs::path file, const std::span<T> content)
		{
			return write_impl(file, content, content.size_bytes(), writemode::append);
		}

		// read impl
		template<typename T>
		return_type read_impl(fs::path file, std::span<T> buffer, size_t buffer_size, size_t offset = 0)
		{
			file = std::filesystem::absolute(file);

			if (not fs::exists(file))
				return std::unexpected(
				  std::format("read_file: file '{}' does not exist", platform::string_from_wide(file.wstring()).c_str()));

			auto file_size = fs::file_size(file);
			if (file_size == 0)
				return std::unexpected(std::format("read_file: file '{}' is empty", platform::string_from_wide(file.wstring()).c_str()));

			// Validate offset
			if (static_cast<u64>(offset) > file_size)
				return std::unexpected(std::format(
				  "read_file: offset {} is beyond end of file '{}' (size {})",
				  offset,
				  platform::string_from_wide(file.wstring()).c_str(),
				  file_size));

			// Default buffer_size: read until end from offset
			if (buffer_size == 0)
				buffer_size = static_cast<size_t>(file_size - static_cast<u64>(offset));

			// Clip to buffer capacity
			buffer_size = std::min(buffer_size, buffer.size_bytes());

			// Also clip to remaining bytes in file from offset
			auto remaining = static_cast<size_t>(file_size - static_cast<u64>(offset));
			buffer_size    = std::min(buffer_size, remaining);

			if (buffer_size == 0)
				return std::unexpected(
				  std::format("read_file: buffer size is zero for file '{}'", platform::string_from_wide(file.wstring()).c_str()));

			DWORD  read{0};
			HANDLE handle =
			  CreateFileW(file.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle == INVALID_HANDLE_VALUE)
				return std::unexpected(
				  std::format("read_file: could not open file '{}'", platform::string_from_wide(file.wstring()).c_str()));

			// Seek to the requested offset for synchronous read
			if (offset != 0)
			{
				LARGE_INTEGER li{};
				li.QuadPart = static_cast<LONGLONG>(offset);
				if (0 == SetFilePointerEx(handle, li, nullptr, FILE_BEGIN))
				{
					CloseHandle(handle);
					return std::unexpected(std::format(
					  "read_file: could not seek to offset {} in file '{}'", offset, platform::string_from_wide(file.wstring()).c_str()));
				}
			}

			// Perform synchronous read (no OVERLAPPED)
			if (0 == ReadFile(handle, buffer.data(), as<DWORD>(buffer_size), &read, nullptr))
			{
				CloseHandle(handle);
				return std::unexpected(
				  std::format("read_file: could not read from file '{}'", platform::string_from_wide(file.wstring()).c_str()));
			}

			CloseHandle(handle);

			return read;
		}

	} // namespace impl

	// size
	export std::optional<u64> filesize(fs::path file)
	{
		file = std::filesystem::absolute(file);

		if (fs::exists(file))
			return fs::file_size(file);

		return {};
	}

	// write
	export auto write(fs::path file, const std::span<u8> content, size_t content_size, writemode mode = writemode::createnew)
	{
		return impl::write_impl<u8>(file, content, content_size, mode);
	}

	export auto write(fs::path file, const std::span<u8> content, writemode mode = writemode::createnew)
	{
		return impl::write_impl<u8>(file, content, content.size_bytes(), mode);
	}

	export auto write(fs::path file, const std::string_view content, writemode mode = writemode::createnew)
	{
		return impl::write_impl(file, std::span<char>(as<char*>(content.data()), content.size()), mode);
	}

	// append
	export auto append(fs::path file, const std::span<u8> content, size_t content_size)
	{
		return impl::append_impl<u8>(file, content, content_size);
	}

	export auto append(fs::path file, const std::span<u8> content) { return impl::append_impl<u8>(file, content, content.size_bytes()); }

	export auto append(fs::path file, const std::string_view content)
	{
		return impl::append_impl(file, std::span<char>(as<char*>(content.data()), content.size()));
	}

	// read
	export auto read(fs::path file, std::span<u8> buffer, size_t buffer_size = 0, size_t offset = 0)
	{
		return impl::read_impl<u8>(file, buffer, buffer_size, offset);
	}

	export auto read(fs::path file, std::string_view buffer, size_t buffer_size = 0, size_t offset = 0)
	{
		return impl::read_impl<char>(file, std::span<char>(as<char*>(buffer.data()), buffer.size()), buffer_size, offset);
	}

	export auto read(fs::path file, std::string& buffer, size_t buffer_size = 0, size_t offset = 0)
	{
		if (buffer_size == 0)
			buffer_size = fs::file_size(file);
		if (buffer.size() < buffer_size)
			buffer.resize(buffer_size);
		return impl::read_impl<char>(file, std::span<char>(as<char*>(buffer.data()), buffer.size()), buffer_size, offset);
	}

	export std::vector<u8> read(fs::path file)
	{
		if (auto size = filesize(file); size)
		{
			std::vector<u8> vec;
			vec.resize(*size);

			auto result = read(file, vec, *size);

			if (result)
				return {};

			return vec;
		}
		return {};
	}

	// ##################################################################################################################
	// ##################################################################################################################
	// ##################################################################################################################

	export class file
	{
	private:
	public:
		void resize() { }

		std::span<u8> operator[](size_t start, size_t end) const
		{
			//
			start;
			end;

			return {};
		}
	};

	namespace v1
	{
		export class file
		{
		private:
			struct handle_deleter
			{
				void operator()(HANDLE h) const
				{
					FlushFileBuffers(h);
					CloseHandle(h);
				}
			};

			using handle_ptr = std::unique_ptr<std::remove_pointer_t<HANDLE>, handle_deleter>;
			handle_ptr handle{nullptr};

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

			~file() = default;

			operator bool() const { return is_open(); }

			bool is_open() const { return handle && handle.get() != INVALID_HANDLE_VALUE; }

			bool open(const fs::path& p)
			{
				path = p;

#ifdef _DEBUG
				constexpr u32 sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
#else
				constexpr u32 sharemode = 0;
#endif

				HANDLE h = CreateFileW(
				  path.generic_wstring().data(),
				  GENERIC_READ | GENERIC_WRITE,
				  sharemode,
				  0,
				  OPEN_ALWAYS,
				  FILE_ATTRIBUTE_NORMAL | FILE_READ_ATTRIBUTES,
				  0);
				if (h == INVALID_HANDLE_VALUE)
				{
					dbg::eprintln("open('{}'): {}", platform::string_from_wide(path.generic_wstring()), platform::get_error_string());
					handle.reset(nullptr);
					return false;
				}

				handle.reset(h);
				return true;
			}

			u64 size() const
			{
				WIN32_FILE_ATTRIBUTE_DATA fad{};
				if (0 == GetFileAttributesExW(path.generic_wstring().data(), GetFileExInfoStandard, &fad))
				{
					dbg::eprintln("size('{}'): {}", platform::string_from_wide(path.generic_wstring()), platform::get_error_string());
					return 0;
				}

				LARGE_INTEGER size{};
				size.HighPart = fad.nFileSizeHigh;
				size.LowPart  = fad.nFileSizeLow;
				return size.QuadPart;
			}

			void flush() const
			{
				if (is_open())
					FlushFileBuffers(handle.get());
			}

			void close() { handle.reset(nullptr); }

			i64 seek(i64 offset) const
			{
				LARGE_INTEGER pos{.QuadPart = offset};
				SetFilePointerEx(handle.get(), pos, nullptr, FILE_BEGIN);
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

				if (0 == WriteFile(handle.get(), buffer.data(), as<DWORD>(size), &written, nullptr))
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

			u64 read(std::span<u8> buffer, u64 size = 0)
			{
				if (size == 0 or size > buffer.size_bytes())
					size = buffer.size_bytes();

				DWORD read{0};

				ReadFile(handle.get(), buffer.data(), as<DWORD>(size), &read, nullptr);

				return read;
			}
		};

		// 1. open, no view to
		// 2. get slice
		// 3. close

		export class filemap
		{
		private:
			std::span<u8> view;
			fs::path      filepath;

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

			explicit filemap(const fs::path file, access flag = access::read) { open(file, flag); }

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

					dbg::println("Could not open file '{}'", platform::string_from_wide(filepath.wstring()).c_str());
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

					dbg::println("Could not create mapping for file '{}' ({})",
								 platform::string_from_wide(filepath.wstring()).c_str(),
								 human_readable_bytes(filesize));
					return {};
				}

				CloseHandle(handle);
				handle = nullptr;

				u8* raw_address = as<u8*>(MapViewOfFile(mapping, filemapping, 0, 0, 0));
				if (raw_address == nullptr)
				{
					close();

					dbg::println("Could not map file '{}'", platform::string_from_wide(filepath.wstring()).c_str());
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

			std::span<u8> span() const { return view; }

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

			std::span<u8> operator[](u64 index, u64 size) const { return view.subspan(index, size); }

			static u32 write(std::filesystem::path file, std::optional<std::span<u8>> content, access flag = access::createnew)
			{
				DWORD bytes_written{0};

				if (not content.has_value())
				{
					dbg::println("write: empty content for file '{}'", platform::string_from_wide(file.wstring()).c_str());
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
					if (platform::get_error() == ERROR_ALREADY_EXISTS)
						dbg::println("write: file '{}' already exists", platform::string_from_wide(file.wstring()).c_str());
					else
						dbg::println("write: could not open file '{}' for writing", platform::string_from_wide(file.wstring()).c_str());

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
		};
	}; // namespace v1

	// simple api

	export std::vector<u8> read_file(fs::path path)
	{
		v1::file f(path);
		if (not f)
		{
			dbg::eprintln("read_text_file: could not open '{}'", path.generic_string());
			return {};
		}

		if (f.size() == 0)
		{
			dbg::eprintln("read_file: file '{}' is empty", path.generic_string());
			return {};
		}

		std::vector<u8> ret;
		ret.resize(f.size());

		auto read_size = f.read({as<u8*>(ret.data()), ret.size()});
		if (read_size < ret.size())
		{
			dbg::eprintln("read_text_file: read partial {}/{}", read_size, ret.size());
		}
		ret.shrink_to_fit();

		return ret;
	}

	export auto read_text_file(fs::path path) -> std::vector<u8>
	{
		auto v = read_file(path);

		// remove bom
		if (v.size() >= 3 and v[0] == 0xEF and v[1] == 0xBB and v[2] == 0xBF)
			v.erase(v.begin(), v.begin() + 3);

		if (v.empty())
			return v;

		std::vector<u8> out;
		out.reserve(v.size());

		// normalize newlines
		for (size_t i = 0; i < v.size(); ++i)
		{
			u8 c = v[i];
			if (c == '\r')
			{
				if (i + 1 < v.size() and v[i + 1] == '\n')
					continue;

				out.push_back('\n');
			}
			else
			{
				out.push_back(c);
			}
		}

		out.shrink_to_fit();
		return out;
	}

	export std::string read_text_file_as_string(fs::path path)
	{
		auto v = read_text_file(path);

		return std::string(v.begin(), v.end());
	}

	export std::vector<std::string> read_lines_exact(fs::path path, std::string_view delimiter, bool include_empty_lines = true)
	{
		using namespace deckard::string;


		auto s   = read_text_file_as_string(path);
		auto ret = split_exact(s, delimiter, include_empty_lines);

		return ret;
	}

	export std::vector<std::string> read_lines_delimiter(fs::path path, std::string_view delimiter = "\n", bool include_empty_lines = false)
	{
		using namespace deckard::string;

		std::vector<std::string> ret;

		if (include_empty_lines == false)
		{
			auto f = read_text_file_as_string(path);
			if (f.empty())
				return {};
			ret = split<std::string>(f, delimiter);
		}
		else
		{
			ret = read_lines_exact(path, delimiter, include_empty_lines);
		}

		return ret;
	}

	export std::vector<std::string> read_lines(fs::path path, std::string_view delimiter = "\n", bool include_empty_lines = false)
	{
		return read_lines_delimiter(path, delimiter, include_empty_lines);
	}

	export std::vector<std::string> read_all_lines(fs::path path, std::string_view delimiter = "\n")
	{
		return read_lines(path, delimiter, true);
	}

	// try read

	export template<arithmetic T>
	std::vector<T> read_lines_as(fs::path path, std::string_view delimiter = "\n", bool include_empty_lines = false)
	{
		using namespace deckard::string;

		const auto     lines = read_lines(path, delimiter, include_empty_lines);
		std::vector<T> ret;

		ret.reserve(lines.size());

		for (const auto& line : lines)
		{
			auto value = try_to_number<T>(trim(line));
			if (value)
				ret.emplace_back(*value);
		}

		return ret;
	}

	export template<arithmetic T>
	std::vector<T> read_all_lines_as(fs::path path, std::string_view delimiter = "\n")
	{
		return read_lines_as<T>(path, delimiter, true);
	}

	export template<arithmetic T>
	std::vector<std::optional<T>> try_read_lines_as(fs::path path, std::string_view delimiter = "\n", bool include_empty_lines = false)
	{
		using namespace deckard::string;

		const auto                    lines = read_lines(path, delimiter, include_empty_lines);
		std::vector<std::optional<T>> ret;

		ret.reserve(lines.size());

		for (const auto& line : lines)
			ret.emplace_back(try_to_number<T>(trim(line)));

		return ret;
	}

	export template<arithmetic T>
	std::vector<std::optional<T>> try_read_all_lines_as(fs::path path, std::string_view delimiter = "\n")
	{
		return try_read_lines_as<T>(path, delimiter, true);
	}

	std::vector<std::optional<std::string>>
	try_read_lines(fs::path path, std::string_view delimiter = "\n", bool include_empty_lines = false)
	{
		using namespace deckard::string;

		const auto                              lines = read_lines(path, delimiter, include_empty_lines);
		std::vector<std::optional<std::string>> ret;
		ret.reserve(lines.size());

		for (const auto& line : lines)
		{
			if (line.empty())
				ret.emplace_back(std::nullopt);
			else
				ret.emplace_back(line);
		}
		return ret;
	}

	export auto try_read_all_lines(fs::path path, std::string_view delimiter = "\n") { return try_read_lines(path, delimiter, true); }


} // namespace deckard::file
