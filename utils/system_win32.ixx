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

	struct execute_process_result
	{
		std::string               output;
		std::string               error;
		std::chrono::milliseconds elapsed_time{0};
		int                       exit_code{0};
	};

	export auto execute_process(std::filesystem::path executable, std::string_view commandline = "",
								std::chrono::milliseconds timeout = 0ms,
								std::filesystem::path     working_directory = std::filesystem::current_path()
	)
		-> execute_process_result
	{
		execute_process_result result{.exit_code = -1};

		u32 timeout_ms = timeout.count() == 0 ? INFINITE : as<u32>(timeout.count());

		constexpr u32 TEMP_BUFFER_SIZE = 1024 * 4;

		HANDLE readpipe{nullptr}, writepipe{nullptr};

		SECURITY_ATTRIBUTES sa{.nLength = sizeof(sa)};
		sa.bInheritHandle       = TRUE;
		sa.lpSecurityDescriptor = nullptr;


		if (not CreatePipe(&readpipe, &writepipe, &sa, TEMP_BUFFER_SIZE))
		{
			result.error    = std::format("CreatePipe failed: {}", GetLastError());
			result.exit_code = -1;
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
			  nullptr, (LPSTR)command.data(), nullptr, nullptr, TRUE, flags, nullptr, const_cast<char*>(working_directory.string().c_str()), &si, &pi))
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
				result.error    = std::format("CreateProcess('{}') failed: {}", command, GetLastError());
				result.exit_code = -2;
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

		auto elapsed_start = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - elapsed_start);

		DWORD bytes_read{0};
		while (true)
		{
			// Check if we've exceeded timeout
			if (timeout != 0ms)
			{
				auto current_time = std::chrono::system_clock::now();
				elapsed      = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - elapsed_start);
				if (elapsed > timeout)
				{
					result.output    = std::string(output_buffer.begin(), output_buffer.end());
					result.error    = "Process output reading timed out";
					result.exit_code = -6;

					CloseHandle(readpipe);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					return result;
				}
			}

			auto peek_result = PeekNamedPipe(readpipe, nullptr, 0, nullptr, &bytes_read, nullptr);
			if (not peek_result)
				break;

			if (bytes_read > 0)
			{
				peek_result = ReadFile(
				  readpipe, temp_buffer.data(), static_cast<DWORD>(temp_buffer.size()), &bytes_read,nullptr);

				if (not peek_result or bytes_read == 0)
				{
					result.error    = std::format("ReadFile failed or no data available: {}", GetLastError());
					result.exit_code = -3;
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



		auto timer_end      = std::chrono::system_clock::now();
		result.output       = std::string(output_buffer.begin(), output_buffer.end());
		result.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(timer_end - timer_start);

		auto waitresult     = WaitForSingleObject(pi.hProcess, timeout_ms);
		if (waitresult == WAIT_TIMEOUT)
		{
			result.error    = "WaitForSingleObject timed out";
			result.exit_code = -4;
		}
		else if (waitresult == WAIT_FAILED)
		{
			result.error    = std::format("WaitForSingleObject failed: {}", GetLastError());
			result.exit_code = -5;
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
