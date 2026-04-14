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
import deckard.utf8;

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

			HANDLE handle = CreateFileW(
			  file.wstring().c_str(), access, FILE_SHARE_READ, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (handle == INVALID_HANDLE_VALUE)
			{
				if (platform::get_error() == ERROR_FILE_EXISTS)
				{
					return std::unexpected(std::format("write_file: file '{}' already exists. Maybe add overwrite flag?",
													   platform::string_from_wide(file.wstring()).c_str()));
				}

				return std::unexpected(std::format(
				  "write_file: could not open file '{}' for writing", platform::string_from_wide(file.wstring()).c_str()));
			}

			if (offset > 0)
			{
				LARGE_INTEGER li{};
				li.QuadPart = static_cast<LONGLONG>(offset);
				if (0 == SetFilePointerEx(handle, li, nullptr, FILE_BEGIN))
				{
					CloseHandle(handle);
					return std::unexpected(std::format(
					  "write_file: could not seek to offset {} in file '{}'",
					  offset,
					  platform::string_from_wide(file.wstring()).c_str()));
				}
			}

			if (not WriteFile(handle, content.data(), as<DWORD>(content_size), &bytes_written, nullptr))
			{
				CloseHandle(handle);
				return std::unexpected(std::format(
				  "write_file: could not write to file '{}'", platform::string_from_wide(file.wstring()).c_str()));
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
				return std::unexpected(
				  std::format("read_file: file '{}' is empty", platform::string_from_wide(file.wstring()).c_str()));

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
				return std::unexpected(std::format(
				  "read_file: buffer size is zero for file '{}'", platform::string_from_wide(file.wstring()).c_str()));

			DWORD  bytes_read{0};
			HANDLE handle = CreateFileW(
			  file.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
					  "read_file: could not seek to offset {} in file '{}'",
					  offset,
					  platform::string_from_wide(file.wstring()).c_str()));
				}
			}

			if (0 == ReadFile(handle, buffer.data(), as<DWORD>(buffer_size), &bytes_read, nullptr))
			{
				CloseHandle(handle);
				return std::unexpected(std::format(
				  "read_file: could not read from file '{}'", platform::string_from_wide(file.wstring()).c_str()));
			}

			CloseHandle(handle);

			return bytes_read;
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

	// return: bytes written
	export auto write(const options& options)
	{
		return impl::write_impl<u8>(
		  options.filename,
		  options.buffer,
		  options.size == 0 ? options.buffer.size_bytes() : options.size,
		  options.offset,
		  options.mode);
	}

	// ##################################################################################################################
	// append

	// return: bytes written
	export auto append(const options& options)
	{
		return impl::append_impl<u8>(
		  options.filename, options.buffer, options.size == 0 ? options.buffer.size_bytes() : options.size);
	}

	// ##################################################################################################################
	// read

	// return: bytes read
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
			assert::check(options.offset < *size,
						  std::format("read_chunks: offset ({}) is beyond file size ({})", options.offset, *size));

			std::vector<u8> buffer;
			buffer.resize(options.chunk_size);
			u64 offset = options.offset;

			while (offset < *size)
			{
				u64  to_read = std::min(options.chunk_size, *size - offset);
				auto res =
				  read({.filename = options.filename,
						.buffer   = std::span<u8>(buffer.data(), static_cast<size_t>(to_read)),
						.size     = to_read,
						.offset   = offset});

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
			assert::check(start_offset < *size,
						  std::format("read_chunks: start_offset ({}) is beyond file size ({})", start_offset, *size));

			std::array<u8, N> buffer;
			u64               offset = start_offset;
			while (offset < *size)
			{
				u64  to_read = std::min(N, *size - offset);
				auto res =
				  read({.file   = file,
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
		std::span<u8> chunk{};

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

		HANDLE handle = CreateFileW(
		  option.filename.wstring().c_str(), rw, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
			dbg::println("filemap: could not create mapping for file '{}'",
						 platform::string_from_wide(option.filename.wstring()).c_str());
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

			view_map chunk{.offset = current_offset, .chunk_size = chunk_size, .chunk = chunk_span};

			co_yield chunk;

			if (chunk.m_stop or chunk.chunk_size == 0)
				break;

			if (option.mode == filemode::readwrite)
				FlushViewOfFile(chunk.chunk.data(), chunk.chunk_size);

			current_offset =
			  (chunk.offset != original_offset) ? (chunk.offset + chunk.chunk_size) : (original_offset + original_size);
			desired_chunk_size = chunk.chunk_size;
		}

		co_return;
	}

	// ##################################################################################################################
	// ##################################################################################################################
	// ##################################################################################################################

	export struct writer_options
	{
		fs::path filename{};
		u64      limit{16_GiB};
		u64      preallocate{0};
	};

	export struct writer_view
	{
		static constexpr u64 chunk_limit{1_GiB};

		u64 initial_size{0};
		u64 write_offset{0};
		u64 m_written{0};
		u64 limit{0};

		bool m_stop{false};

		void stop() { m_stop = true; }

		[[nodiscard]] bool full() const { return m_written >= limit; }

		[[nodiscard]] u64 written() const { return m_written; }

		std::expected<u32, std::string> write(std::string_view data)
		{
			return write(std::span<const u8>(reinterpret_cast<const u8*>(data.data()), data.size()));
		}

		template<typename... Args>
		std::expected<u32, std::string> write(std::format_string<Args...> fmt, Args&&... args)
		{
			auto formatted = std::format(fmt, std::forward<Args>(args)...);
			return write(std::string_view{formatted});
		}

		std::expected<u32, std::string> write(std::span<const u8> data)
		{
			if (handle == INVALID_HANDLE_VALUE)
				return std::unexpected(std::string{"appender: invalid file handle"});

			if (m_stop or full() or data.empty())
				return std::expected<u32, std::string>{0_u32};

			const u64 remaining = limit - m_written;
			const u64 to_buffer = std::min<u64>(remaining, data.size_bytes());

			u64 offset = 0;
			while (offset < to_buffer)
			{
				const u64 current_chunk_limit = chunk_limit;

				if (m_buffer.size() >= current_chunk_limit)
				{
					auto result = flush();
					if (not result)
						return std::unexpected(result.error());
					continue;
				}

				const u64 free_space = current_chunk_limit - m_buffer.size();
				const u64 copy_size  = std::min<u64>(free_space, to_buffer - offset);

				m_buffer.insert(m_buffer.end(),
								data.data() + static_cast<size_t>(offset),
								data.data() + static_cast<size_t>(offset + copy_size));

				offset += copy_size;
				m_written += copy_size;

				if (m_buffer.size() >= current_chunk_limit)
				{
					auto result = flush();
					if (not result)
						return std::unexpected(result.error());
				}

				if (m_written >= limit)
				{
					m_stop      = true;
					auto result = flush();
					if (not result)
						return std::unexpected(result.error());
					break;
				}
			}

			return std::expected<u32, std::string>{static_cast<u32>(to_buffer)};
		}

		std::expected<u32, std::string> flush()
		{
			if (m_buffer.empty())
				return std::expected<u32, std::string>{0_u32};

			if (handle == INVALID_HANDLE_VALUE)
				return std::unexpected(std::string{"appender: invalid file handle"});

			LARGE_INTEGER pos{};
			pos.QuadPart = static_cast<LONGLONG>(write_offset);
			if (SetFilePointerEx(handle, pos, nullptr, FILE_BEGIN) == 0)
				return std::unexpected(std::string{"appender: seek failed"});

			u64 written_total = 0;
			while (written_total < m_buffer.size())
			{
				const u64 remaining = m_buffer.size() - written_total;
				const u64 chunk     = std::min<u64>(remaining, std::numeric_limits<DWORD>::max());

				DWORD bytes_written = 0;
				if (not WriteFile(handle,
								  m_buffer.data() + static_cast<size_t>(written_total),
								  static_cast<DWORD>(chunk),
								  &bytes_written,
								  nullptr))
					return std::unexpected(std::string{"appender: write failed"});

				if (bytes_written == 0)
					return std::unexpected(std::string{"appender: write failed"});

				written_total += bytes_written;

				dbg::println("flush: {}/{}", bytes_written, written_total);
			}

			write_offset += written_total;

			m_buffer.clear();
			return std::expected<u32, std::string>{static_cast<u32>(written_total)};
		}

	private:
		HANDLE          handle{INVALID_HANDLE_VALUE};
		std::vector<u8> m_buffer{};

		friend std::generator<writer_view&> writer(const writer_options option);
	};

	export [[nodiscard]] std::generator<writer_view&> writer(const writer_options option)
	{
		if (option.filename.empty())
		{
			dbg::println("appender: filename is empty");
			co_return;
		}
		auto file = std::filesystem::absolute(option.filename);

		HANDLE handle = CreateFileW(
		  file.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
		{
			dbg::println("appender: could not open file '{}'", platform::string_from_wide(file.wstring()));
			co_return;
		}

		LARGE_INTEGER original_size{};
		if (GetFileSizeEx(handle, &original_size) == 0)
		{
			dbg::println("appender: could not get file size '{}'", platform::string_from_wide(file.wstring()));
			CloseHandle(handle);
			co_return;
		}

		if (option.preallocate > 0)
		{
			LARGE_INTEGER current_size{};
			if (GetFileSizeEx(handle, &current_size) == 0)
			{
				dbg::println("appender: could not get file size '{}'", platform::string_from_wide(file.wstring()));
				CloseHandle(handle);
				co_return;
			}

			if (static_cast<u64>(current_size.QuadPart) < option.preallocate)
			{
				LARGE_INTEGER target_size{};
				target_size.QuadPart = static_cast<LONGLONG>(option.preallocate);

				if (SetFilePointerEx(handle, target_size, nullptr, FILE_BEGIN) == 0 or SetEndOfFile(handle) == 0)
				{
					dbg::println("appender: could not preallocate file '{}'", platform::string_from_wide(file.wstring()));
					CloseHandle(handle);
					co_return;
				}
			}
		}

		writer_view writer{};
		writer.handle       = handle;
		writer.initial_size = static_cast<u64>(original_size.QuadPart);
		writer.write_offset = writer.initial_size;
		writer.limit        = option.limit;

		struct handle_guard
		{
			HANDLE&      handle;
			writer_view& writer;
			fs::path     file;

			~handle_guard()
			{
				if (handle == INVALID_HANDLE_VALUE)
					return;

				if (auto result = writer.flush(); not result)
					dbg::println("appender: flush failed '{}'", result.error());

				FlushFileBuffers(handle);

				LARGE_INTEGER target_size{};
				target_size.QuadPart = static_cast<LONGLONG>(writer.write_offset);

				if (SetFilePointerEx(handle, target_size, nullptr, FILE_BEGIN) == 0)
					dbg::println("appender: seek for resize failed '{}'", platform::string_from_wide(file.wstring()));
				else if (SetEndOfFile(handle) == 0)
					dbg::println("appender: could not resize file '{}' (error {})",
								 platform::string_from_wide(file.wstring()),
								 platform::get_error());

				CloseHandle(handle);
				handle = INVALID_HANDLE_VALUE;
			}
		};

		handle_guard close_guard{handle, writer, file};

		if (writer.limit == 0)
			co_return;

		while (not writer.m_stop and not writer.full())
		{
			u64 previous_written = writer.m_written;
			co_yield writer;

			if (writer.m_stop or writer.full())
				break;

			if (writer.m_written == previous_written)
				break;
		}


		co_return;
	}

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
		if (has_bom_utf8(v))
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

	export auto read_text_file_as_utf8(fs::path path) -> utf8::string
	{
		auto v = read_text_file(path);

		utf8::string ret(v);
		if (auto result = ret.valid(); not result)
			dbg::println("Warning: file '{}' with invalid UTF-8 data: {}", path, result.error());

		return ret;
	}

	export std::string read_text_file_as_string(fs::path path)
	{
		auto v = read_text_file(path);

		return std::string(v.begin(), v.end());
	}

	export std::vector<std::string>
	read_lines_exact(fs::path path, std::string_view delimiter, bool include_empty_lines = true)
	{
		using namespace deckard::string;


		auto s   = read_text_file_as_string(path);
		auto ret = split_exact(s, delimiter, include_empty_lines);

		return ret;
	}

	export std::vector<std::string>
	read_lines_delimiter(fs::path path, std::string_view delimiter = "\n", bool include_empty_lines = false)
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

	export std::vector<std::string>
	read_lines(fs::path path, std::string_view delimiter = "\n", bool include_empty_lines = false)
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
	std::vector<std::optional<T>>
	try_read_lines_as(fs::path path, std::string_view delimiter = "\n", bool include_empty_lines = false)
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

	export auto try_read_all_lines(fs::path path, std::string_view delimiter = "\n")
	{
		return try_read_lines(path, delimiter, true);
	}

	// ##################################################################################################################
	// ##################################################################################################################
	// ##################################################################################################################


} // namespace deckard::file
