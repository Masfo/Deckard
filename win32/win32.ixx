module;

#include <Windows.h>
#include <Psapi.h>
#include <Shlobj.h>
#include <VersionHelpers.h>
#include <dwmapi.h>

export module deckard.win32;

import std;
import deckard.as;
import deckard.assert;
import deckard.types;
import deckard.debug;
import deckard.helpers;
import deckard.scope_exit;
import deckard.platform;
import deckard.cpuid;


namespace deckard::system
{
	using namespace std::string_view_literals;

	export std::string get_windows_error(DWORD error = GetLastError());

	

	std::string GetRegistryValue(HKEY computer, std::string_view key, std::string_view value)
	{

		std::string ret;

		HKEY hKey{};

		scope_exit _([&] { RegCloseKey(hKey); });

		auto dwError = RegOpenKeyExA(computer, key.data(), 0u, (REGSAM)KEY_READ | KEY_WOW64_64KEY, &hKey);
		if (dwError != ERROR_SUCCESS)
			return "";

		ULONG cb   = 0;
		ULONG Type = 0;

		dwError = RegQueryValueExA(hKey, value.data(), nullptr, &Type, nullptr, &cb);
		if (dwError != ERROR_SUCCESS)
		{
			dbg::println("Registry query failed: {}", get_windows_error());
			return "";
		}

		if (Type == REG_DWORD)
		{
			unsigned long regvalue = 0;

			dwError = RegQueryValueExA(hKey, value.data(), nullptr, &Type, (PBYTE)&regvalue, &cb);
			if (dwError != ERROR_SUCCESS)
				return "";

			return std::to_string(regvalue);
		}

		if (Type == REG_SZ)
		{
			ret.resize(cb);

			dwError = RegQueryValueExA(hKey, value.data(), nullptr, &Type, (PBYTE)ret.data(), &cb);
			if (dwError != ERROR_SUCCESS)
				return "";

			ret.resize(ret.size() - 1);

			return ret;
		}

		return "";
	}

	std::string GetLocalRegistryValue(std::string_view key, std::string_view value)
	{
		return GetRegistryValue(HKEY_LOCAL_MACHINE, key, value);
	}

	std::string GetCurrentUserRegistryValue(std::string_view key, std::string_view value)
	{
		return GetRegistryValue(HKEY_CURRENT_USER, key, value);
	}

	export bool is_darkmode()
	{
		const std::string_view key("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");

		if (auto result = GetCurrentUserRegistryValue(key, "AppsUseLightTheme"); result == "0"sv)
			return true;

		return false;
	}

	export std::string GetOSVersionString()
	{
		const std::string key("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");

		auto ProductName    = GetLocalRegistryValue(key, "ProductName");
		auto ReleaseId      = GetLocalRegistryValue(key, "ReleaseId");
		auto DisplayVersion = GetLocalRegistryValue(key, "DisplayVersion");
		auto CurrentBuild   = GetLocalRegistryValue(key, "CurrentBuild");
		auto UBR            = GetLocalRegistryValue(key, "UBR");

		if (ProductName.empty() && ReleaseId.empty() && CurrentBuild.empty() && UBR.empty())
			return "Windows";

		return std::format(
		  "Microsoft Windows Version {} (OS Build {}.{}) ", DisplayVersion.empty() ? ReleaseId : DisplayVersion, CurrentBuild, UBR);
	}

	export bool IsWindowsVersion(u32 major, u32 minor, u32 build)
	{
		OSVERSIONINFOEXW osvi             = {sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0};
		DWORDLONG const  dwlConditionMask = VerSetConditionMask(
          VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL),
          VER_BUILDNUMBER,
          VER_GREATER_EQUAL);

		osvi.dwMajorVersion = major;
		osvi.dwMinorVersion = minor;
		osvi.dwBuildNumber  = build;

		return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, dwlConditionMask) != FALSE;
	}

	struct BuildInfo
	{
		u32 major{0};
		u32 minor{0};
		u32 build{0};
	};

	export BuildInfo OSBuildInfo()
	{
		//

		constexpr u32 KUSER_SHARED_DATA_PTR = 0x7FFE'0000u;
		u32           major                 = *(u32*)(KUSER_SHARED_DATA_PTR + 0x026C); // NtMajorVersion, 4.0+
		u32           minor                 = *(u32*)(KUSER_SHARED_DATA_PTR + 0x0270); // NtMinorVersion, 4.0+

		u32 build = 0;
		if (major >= 10)
			build = *(u32*)(KUSER_SHARED_DATA_PTR + 0x0260); // NtBuildNumber, 10.0+
		return {major, minor, build};
	}



	export std::string get_windows_error(DWORD error)
	{
		char err[256]{0};
		if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL) == 0)
			return std::format("Failed to get error from code {:X}", error);
		return std::string{err};
	}

	// in bytes
	export size_t process_ram_usage()
	{
		PROCESS_MEMORY_COUNTERS_EX pmc{};
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		return pmc.WorkingSetSize;
	}

	export std::string GetCPUIDString()
	{
		cpuid::CPUID id;
		return id.as_string();
	}



} // namespace deckard::system
