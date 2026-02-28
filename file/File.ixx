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
import deckard.random;
import deckard.utils.hash;

namespace fs = std::filesystem;
using namespace std::string_literals;

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


	export enum class filemode {
		createnew,
		append,
		overwrite,
		readonly,  // map
		readwrite, // map
	};

	export struct options
	{
		fs::path      filename;
		std::span<u8> buffer;
		u64           size{0};
		u64           offset{0};
		u64           chunk_size{4096}; // For read_chunks
		filemode      mode{filemode::overwrite};
	};

	// ##################################################################################################################

	namespace impl
	{

		using return_type = std::expected<u32, std::string>;

		// write impl with offset
		template<typename T>
		return_type write_impl(fs::path file, const std::span<T> content, u64 content_size, u64 offset, filemode filemode)
		{
			DWORD bytes_written{0};
			DWORD creation = CREATE_ALWAYS;
			DWORD access   = GENERIC_WRITE;

			file         = std::filesystem::absolute(file);
			content_size = std::min(content_size, content.size_bytes());

			if (offset > 0)
			{
				if (not fs::exists(file))
					return std::unexpected(std::format(
					  "write_file: cannot write at offset {} to non-existent file '{}'",
					  offset,
					  platform::string_from_wide(file.wstring()).c_str()));

				creation = OPEN_EXISTING;
				access   = GENERIC_READ | GENERIC_WRITE;
			}
			else
			{
				if (filemode == filemode::createnew)
				{
					creation = CREATE_NEW;
				}
				else if (filemode == filemode::append)
				{
					if (not fs::exists(file))
						creation = CREATE_NEW;
					else
						creation = OPEN_ALWAYS;

					access = FILE_APPEND_DATA;
				}
				else if (filemode == filemode::overwrite)
				{
					creation = CREATE_ALWAYS;
				}
			}

			HANDLE handle = CreateFileW(file.wstring().c_str(), access, FILE_SHARE_READ, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);

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

			if (offset > 0)
			{
				LARGE_INTEGER li{};
				li.QuadPart = static_cast<LONGLONG>(offset);
				if (0 == SetFilePointerEx(handle, li, nullptr, FILE_BEGIN))
				{
					CloseHandle(handle);
					return std::unexpected(std::format(
					  "write_file: could not seek to offset {} in file '{}'", offset, platform::string_from_wide(file.wstring()).c_str()));
				}
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
		return_type write_impl(fs::path file, const std::span<T> content, filemode filemode)
		{
			return write_impl(file, content, content.size_bytes(), 0, filemode);
		}

		// append impl
		template<typename T>
		return_type append_impl(fs::path file, const std::span<T> content, u64 content_size)
		{
			return write_impl(file, content, content_size, 0, filemode::append);
		}

		template<typename T>
		return_type append_impl(fs::path file, const std::span<T> content)
		{
			return write_impl(file, content, content.size_bytes(), 0, filemode::append);
		}

		// read impl
		template<typename T>
		return_type read_impl(fs::path file, std::span<T> buffer, u64 buffer_size, u64 offset = 0)
		{
			file = std::filesystem::absolute(file);

			if (not fs::exists(file))
				return std::unexpected(
				  std::format("read_file: file '{}' does not exist", platform::string_from_wide(file.wstring()).c_str()));

			auto file_size = fs::file_size(file);
			if (file_size == 0)
				return std::unexpected(std::format("read_file: file '{}' is empty", platform::string_from_wide(file.wstring()).c_str()));

			if (offset >= file_size)
				return std::unexpected(std::format(
				  "read_file: offset {} is beyond end of file '{}' (size {})",
				  offset,
				  platform::string_from_wide(file.wstring()).c_str(),
				  file_size));

			if (buffer_size == 0)
				buffer_size = file_size - offset;

			buffer_size = std::min(buffer_size, buffer.size_bytes());

			auto remaining = file_size - offset;
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

	// ##################################################################################################################
	// write

	export auto write(const options& options)
	{
		return impl::write_impl<u8>(
		  options.filename, options.buffer, options.size == 0 ? options.buffer.size_bytes() : options.size, options.offset, options.mode);
	}

	// ##################################################################################################################
	// append

	export auto append(const options& options)
	{
		return impl::append_impl<u8>(options.filename, options.buffer, options.size == 0 ? options.buffer.size_bytes() : options.size);
	}

	// ##################################################################################################################
	// read

	export auto read(const options& options)
	{
		return impl::read_impl<u8>(
		  options.filename, options.buffer, options.size == 0 ? options.buffer.size_bytes() : options.size, options.offset);
	}

	export std::vector<u8> read(fs::path file)
	{
		if (auto size = filesize(file); size)
		{
			std::vector<u8> vec;
			vec.resize(*size);

			auto result = read({.filename = file, .buffer = vec, .size = *size});

			if (result)
				return vec;

			return {};
		}
		return {};
	}

	// ##################################################################################################################
	// read_chunks

	export std::generator<std::span<u8>> read_chunks(const options& options)
	{
		if (auto size = filesize(options.filename); size)
		{
			assert::check(options.offset < *size, std::format("read_chunks: offset ({}) is beyond file size ({})", options.offset, *size));

			std::vector<u8> buffer;
			buffer.resize(options.chunk_size);
			u64 offset = options.offset;

			while (offset < *size)
			{
				u64  to_read = std::min(options.chunk_size, *size - offset);
				auto res =
				  read({.filename   = options.filename,
						.buffer = std::span<u8>(buffer.data(), static_cast<size_t>(to_read)),
						.size   = to_read,
						.offset = offset});

				if (res)
				{
					co_yield std::span<u8>(buffer.data(), static_cast<size_t>(*res));
					offset += *res;
				}
				else
				{
					dbg::eprintln("read_chunks('{}'): {}", options.filename.string(), res.error());
					co_return;
				}
			}
		}
		co_return;
	}

	export template<u64 N, u64 start_offset = 0>
	std::generator<std::span<u8>> read_chunks(fs::path file)
	{
		static_assert(N <= 8192, "read_chunks: N must be less than or equal to 8192 bytes");

		if (auto size = filesize(file); size)
		{
			assert::check(
			  start_offset < *size, std::format("read_chunks: start_offset ({}) is beyond file size ({})", start_offset, *size));

			std::array<u8, N> buffer;
			u64               offset = start_offset;
			while (offset < *size)
			{
				u64  to_read = std::min(N, *size - offset);
				auto res     = read(
                  {.file = file, .buffer = std::span<u8>(buffer.data(), static_cast<size_t>(to_read)), .size = to_read, .offset = offset});
				if (res)
				{
					co_yield std::span<u8>(buffer.data(), static_cast<size_t>(*res));
					offset += *res;
				}
				else
				{
					dbg::eprintln("read_chunks('{}'): {}", file.string(), res.error());
					co_return;
				}
			}
		}
		co_return;
	}

	// ##################################################################################################################
	// ##################################################################################################################
	// ##################################################################################################################


	struct view_map
	{
		u64           offset{0};
		u64           chunk_size{0};
		std::span<u8> chunk_buffer{};

		bool m_stop{false};

		void stop() { m_stop = true; }
	};

	// NOTE: Mapping lifetime must be tied to the generator frame.
	// We use a local std::unique_ptr with a lambda deleter inside map() for RAII.

	/*
		for (auto& i : file::map({.file = "260.bin", .chunk_size=32}))
	{

		info("map {}, offset {}, {}", i.chunk_buffer.size(), i.offset, to_hex_string(i.chunk_buffer));

		//i.offset += i.chunk_size;
		i.chunk_size = (i.chunk_size == 64) ? 32 : 64;

		if (i.offset >= 128)
			i.stop();

	}
	*/
	export [[nodiscard]] std::generator<view_map&> map(const options option)
	{
		if (not fs::exists(option.filename))
		{
			dbg::println("filemap: file '{}' does not exist", platform::string_from_wide(option.filename.wstring()).c_str());
			co_return;
		}

		DWORD rw          = GENERIC_READ;
		DWORD page        = PAGE_READONLY;
		DWORD filemapping = FILE_MAP_READ;

		if (option.mode == filemode::readwrite)
		{
			rw |= GENERIC_WRITE;
			page = PAGE_READWRITE;
			filemapping |= FILE_MAP_WRITE;
		}

		HANDLE handle =
		  CreateFileW(option.filename.wstring().c_str(), rw, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
		{
			dbg::println("filemap: could not open file '{}'", platform::string_from_wide(option.filename.wstring()).c_str());
			co_return;
		}

		u64 size = filesize(option.filename).value_or(0);
		if (size == 0)
		{
			CloseHandle(handle);
			dbg::println("filemap: file '{}' is empty", platform::string_from_wide(option.filename.wstring()).c_str());
			co_return;
		}

		if (option.offset >= size)
		{
			CloseHandle(handle);
			dbg::println("filemap: offset {} is beyond end of file '{}' (size {})",
						 option.offset,
						 platform::string_from_wide(option.filename.wstring()).c_str(),
						 size);
			co_return;
		}


		HANDLE mapping = CreateFileMapping(handle, 0, page, 0, 0, nullptr);
		if (mapping == nullptr)
		{
			CloseHandle(handle);
			dbg::println("filemap: could not create mapping for file '{}'", platform::string_from_wide(option.filename.wstring()).c_str());
			co_return;
		}

		CloseHandle(handle);
		handle = nullptr;

		u8* raw_address = as<u8*>(MapViewOfFile(mapping, filemapping, 0, 0, 0));
		if (raw_address == nullptr)
		{
			CloseHandle(mapping);
			dbg::println("filemap: could not map file '{}'", platform::string_from_wide(option.filename.wstring()).c_str());
			co_return;
		}
		CloseHandle(mapping);
		mapping = nullptr;

		std::span<u8> view{raw_address, static_cast<size_t>(size)};

		const auto unmap = [&option](u8* address)
		{
			if (address)
			{
				if (option.mode == filemode::readwrite)
					FlushViewOfFile(address, 0);
				UnmapViewOfFile(address);
				address = nullptr;
			}
		};
		std::unique_ptr<u8, decltype(unmap)> view_guard(raw_address, unmap);

		u64 current_offset     = option.offset;
		u64 desired_chunk_size = option.chunk_size;
		if (desired_chunk_size == 0)
			desired_chunk_size = size - current_offset;

		while (current_offset < size)
		{
			auto remaining       = size - current_offset;
			u64  chunk_size      = std::min<u64>(desired_chunk_size, remaining);
			u64  original_offset = current_offset;
			u64  original_size   = chunk_size;
			auto chunk_span      = view.subspan(static_cast<size_t>(current_offset), static_cast<size_t>(chunk_size));

			view_map chunk{.offset = current_offset, .chunk_size = chunk_size, .chunk_buffer = chunk_span};

			co_yield chunk;

			if (chunk.m_stop or chunk.chunk_size == 0)
				break;

			if (option.mode == filemode::readwrite)
				FlushViewOfFile(chunk.chunk_buffer.data(), chunk.chunk_size);

			current_offset     = (chunk.offset != original_offset) ? (chunk.offset + chunk.chunk_size) : (original_offset + original_size);
			desired_chunk_size = chunk.chunk_size;
		}

		co_return;
	}

	// ##################################################################################################################
	// ##################################################################################################################
	// ##################################################################################################################

	export fs::path get_temp_path() { return fs::temp_directory_path(); }

	export fs::path get_temp_file(std::string_view prefix = "")
	{
		auto     temp_path = get_temp_path();
		fs::path temp_file = fs::path(std::string(prefix) + random::alphanum(12) + ".tmp"s);
		while (fs::exists(temp_path / temp_file))
			temp_file = fs::path(prefix) / fs::path(random::alphanum(12) + ".tmp"s);

		return temp_path / temp_file;
	}

	// ##################################################################################################################
	// ##################################################################################################################
	// ##################################################################################################################

	export u64 hash_file_contents(fs::path file)
	{
		auto contents = read(file);
		if (contents.size() == 0)
			return 0;
		return utils::chibihash64(contents);
	}

	// ##################################################################################################################
	// ##################################################################################################################
	// ##################################################################################################################

	namespace v1
	{


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
		std::vector<u8> ret;

		auto size = filesize(path);
		if (size)
		{
			ret.resize(*size);
			auto r = file::read({.filename = path, .buffer = ret, .size = *size});
			if (not r)
			{
				dbg::eprintln("read_file: could not read file '{}': {}", path.generic_string(), r.error());
				return {};
			}
			return ret;
		}
		return {};
	}

	export auto read_text_file(fs::path path) -> std::vector<u8>
	{
		std::vector<u8> v = read_file(path);

		if (v.empty())
			return v;

		// remove bom
		if (v.size() >= 3 and v[0] == 0xEF and v[1] == 0xBB and v[2] == 0xBF)
			v.erase(v.begin(), v.begin() + 3);

		std::vector<u8> out;
		out.reserve(v.size());

		// normalize newlines
		for (u64 i = 0; i < v.size(); ++i)
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
