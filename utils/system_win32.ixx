module;
#include <Windows.h>

export module deckard.system;


import std;
import deckard.assert;
import deckard.types;
import deckard.as;

using namespace std::chrono_literals;

namespace deckard::system
{


	export template<typename T>
	requires std::is_pointer_v<T>
	T get_address(std::string_view dll, std::string_view apiname, const std::source_location& loc = std::source_location::current())
	{
		auto library = LoadLibraryA(dll.data());
		assert::check(library != nullptr, std::format("library '{}' not found.", dll), loc);
		if (not library)
			return nullptr;

		T function = reinterpret_cast<T>(static_cast<void*>(GetProcAddress(library, apiname.data())));
		assert::check(function != nullptr, std::format("function '{}' in library '{}' not found.", apiname, dll), loc);
		if (not function)
			return nullptr;

		return function;
	}

	enum class exit_code_enum : u8
	{
		ok = 0,
		createpipe_fail,
		createprocess_fail,
		wait_fail,
		timeout,
		readfile_fail,

		unknown = limits::max<u8>
	};

	struct execute_process_result
	{
		std::string               output;
		std::string               error;
		std::chrono::milliseconds elapsed_time{0};

		using enum exit_code_enum;
		exit_code_enum exit_code{ok};
	};

	export auto
	execute_process(std::filesystem::path executable, std::string_view commandline = "", std::chrono::milliseconds timeout = 0ms,
					std::filesystem::path working_directory = std::filesystem::current_path()) -> execute_process_result
	{
		execute_process_result result{};
		u32                    timeout_ms = timeout.count() == 0 ? INFINITE : as<u32>(timeout.count());

		constexpr u32 TEMP_BUFFER_SIZE = 1024 * 4;

		HANDLE readpipe{nullptr}, writepipe{nullptr};

		SECURITY_ATTRIBUTES sa{.nLength = sizeof(sa)};
		sa.bInheritHandle       = TRUE;
		sa.lpSecurityDescriptor = nullptr;


		if (not CreatePipe(&readpipe, &writepipe, &sa, TEMP_BUFFER_SIZE))
		{
			result.error     = std::format("CreatePipe failed: {}", GetLastError());
			result.exit_code = exit_code_enum::createpipe_fail;
			return result;
		}

		SetHandleInformation(readpipe, HANDLE_FLAG_INHERIT, 0);

		PROCESS_INFORMATION pi{};
		STARTUPINFOA        si{.cb = sizeof(si)};

		si.hStdError  = writepipe;
		si.hStdOutput = writepipe;
		si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
		si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		DWORD flags = CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;

		std::string command = std::format("{} {}", executable.string(), commandline);

		if (not CreateProcessA(
			  nullptr,
			  (LPSTR)command.data(),
			  nullptr,
			  nullptr,
			  TRUE,
			  flags,
			  nullptr,
			  const_cast<char*>(working_directory.string().c_str()),
			  &si,
			  &pi))
		{
			// try again with cmd
			command.insert(0, "cmd /c ");

			if (not CreateProcessA(
				  nullptr,
				  (LPSTR)command.data(),
				  nullptr,
				  nullptr,
				  TRUE,
				  flags,
				  nullptr,
				  const_cast<char*>(working_directory.string().c_str()),
				  &si,
				  &pi))
			{
				result.error     = std::format("CreateProcess('{}') failed: {}", command, GetLastError());
				result.exit_code = exit_code_enum::createprocess_fail;
				CloseHandle(readpipe);
				CloseHandle(writepipe);
				return result;
			}
		}

		auto timer_start = std::chrono::system_clock::now();
		CloseHandle(writepipe);

		std::array<char, TEMP_BUFFER_SIZE> temp_buffer{};
		std::vector<char>                  output_buffer;
		output_buffer.reserve(TEMP_BUFFER_SIZE);

		std::chrono::milliseconds elapsed =
		  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timer_start);

		DWORD bytes_read{0};
		while (true)
		{
			elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timer_start);

			if (timeout != 0ms)
			{
				if (elapsed > timeout)
				{
					result.output    = std::string(output_buffer.begin(), output_buffer.end());
					result.error     = "Process output reading timed out";
					result.exit_code = exit_code_enum::timeout;

					result.output       = std::string(output_buffer.begin(), output_buffer.end());
					result.elapsed_time = elapsed;


					CloseHandle(readpipe);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					return result;
				}
			}

			auto peek_result = PeekNamedPipe(readpipe, nullptr, 0, nullptr, &bytes_read, nullptr);
			if (not peek_result)
			{
				break;
			}

			if (bytes_read > 0)
			{
				peek_result = ReadFile(readpipe, temp_buffer.data(), static_cast<DWORD>(temp_buffer.size()), &bytes_read, nullptr);

				if (not peek_result or bytes_read == 0)
				{
					result.error     = std::format("ReadFile failed or no data available: {}", GetLastError());
					result.exit_code = exit_code_enum::readfile_fail;

					result.output = std::string(output_buffer.begin(), output_buffer.end());

					elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timer_start);
					result.elapsed_time = elapsed;


					CloseHandle(readpipe);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					return result;
				}
				output_buffer.insert(output_buffer.end(), temp_buffer.data(), temp_buffer.data() + bytes_read);
				temp_buffer.fill(0);
			}
		}

		CloseHandle(readpipe);

		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timer_start);

		result.elapsed_time = elapsed;
		result.output       = std::string(output_buffer.begin(), output_buffer.end());


		auto wait_timer = std::chrono::system_clock::now();

		auto waitresult = WaitForSingleObject(pi.hProcess, timeout_ms);
		if (waitresult == WAIT_TIMEOUT)
		{
			result.error     = "WaitForSingleObject timed out";
			result.exit_code = exit_code_enum::timeout;
		}
		else if (waitresult == WAIT_FAILED)
		{
			result.error     = std::format("WaitForSingleObject failed: {}", GetLastError());
			result.exit_code = exit_code_enum::wait_fail;
		}
		else
		{
			GetExitCodeProcess(pi.hProcess, reinterpret_cast<LPDWORD>(&result.exit_code));
		}

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return result;
	}


} // namespace deckard::system
