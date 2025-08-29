module;
#include <Windows.h>
#include <VersionHelpers.h>

export module deckard.threadutil;

import std;
import deckard.types;
import deckard.function_ref;

namespace deckard::thread
{
	export template<typename T>
	class threadsafe
	{
	private:
		T          value;
		std::mutex mutex;

	public:
		template<typename... Args>
		threadsafe(Args&&... args)
			: value(std::forward<Args>(args)...)
		{
		}

		template<typename F>
		auto with_lock(F&& f) -> decltype(f(value))
		{
			std::lock_guard lock(mutex);
			return f(value);
		}
	};

	export void set_thread_name(std::string_view [[maybe_unused]]threadname)
	{
		#ifdef _WIN32
#ifdef _DEBUG
		using THREADNAME_INFO = struct tagTHREADNAME_INFO
		{
			DWORD       dwType{0x1000};          // must be 0x1000
			const char* szName{nullptr};         // pointer to name (in user addr space)
			DWORD       dwThreadID{0xFFFF'FFFF}; // thread ID (-1=caller thread)
			DWORD       dwFlags{0};              // reserved for future use, must be zero
		};

		if (IsWindows10OrGreater())
		{
			constexpr i32 WIDESIZE = 128;

			std::array<wchar_t, WIDESIZE> wide{};

			int len = MultiByteToWideChar(CP_UTF8, 0, threadname.data(), -1, nullptr, 0);
			MultiByteToWideChar(CP_UTF8, 0, threadname.data(), len, wide.data(), WIDESIZE);

			SetThreadDescription(GetCurrentThread(), wide.data());
		}
		else
		{
			THREADNAME_INFO info{};
			info.szName = threadname.data();

			constexpr DWORD MS_VC_EXCEPTION = 0x406D'1388;

			__try
			{
				RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), reinterpret_cast<ULONG_PTR*>(&info));
			}
			__except (EXCEPTION_CONTINUE_EXECUTION)
			{
			}
		}
#endif
		#endif // WIN32
	}


} // namespace deckard::thread
