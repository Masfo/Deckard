module;

#include <Windows.h>

export module deckard.win32;

import std;
import deckard.assert;
import deckard.types;

namespace deckard
{
	std::string GetLocalRegistryValue(std::string_view key, std::string_view value) noexcept;

	export template<typename T>
	requires std::is_pointer_v<T>
	T LoadDynamic(const std::string_view dll, const std::string_view function) noexcept
	{
		HMODULE lib = LoadLibraryA(dll.data());
		assert_msg(lib != nullptr, "Failed to load library.");
		if (!lib)
			return nullptr;


		auto function_to_load = GetProcAddress(lib, function.data());
		assert_msg(function_to_load != nullptr, "Failed to load function.");

		if (!function_to_load)
			return nullptr;

		return reinterpret_cast<T>((FARPROC *)function_to_load);
	}

	std::string GetLocalRegistryValue(std::string_view key, std::string_view value) noexcept
	{

		using Func_RegOpenKeyExA    = LSTATUS(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
		using Func_RegQueryValueExA = LSTATUS(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

		static auto RegOpenKeyExA_F    = LoadDynamic<Func_RegOpenKeyExA *>("advapi32.dll", "RegOpenKeyExA");
		static auto RegQueryValueExA_F = LoadDynamic<Func_RegQueryValueExA *>("advapi32.dll", "RegQueryValueExA");

		if (!RegOpenKeyExA_F || !RegQueryValueExA_F)
			return "";

		std::string ret;

		HKEY hKey{};
		auto dwError = RegOpenKeyExA_F(HKEY_LOCAL_MACHINE, key.data(), 0u, (REGSAM)KEY_READ | KEY_WOW64_64KEY, &hKey);
		if (dwError != ERROR_SUCCESS)
			return "";

		ULONG cb   = 0;
		ULONG Type = 0;

		dwError = RegQueryValueExA_F(hKey, value.data(), nullptr, &Type, nullptr, &cb);
		if (dwError != ERROR_SUCCESS)
			return "";

		if (Type == REG_DWORD)
		{
			unsigned long regvalue = 0;

			dwError = RegQueryValueExA_F(hKey, value.data(), nullptr, &Type, (PBYTE)&regvalue, &cb);
			if (dwError != ERROR_SUCCESS)
				return "";

			return std::to_string(regvalue);
		}

		if (Type == REG_SZ)
		{
			ret.resize(cb);

			dwError = RegQueryValueExA_F(hKey, value.data(), nullptr, &Type, (PBYTE)ret.data(), &cb);
			if (dwError != ERROR_SUCCESS)
				return "";

			ret.resize(ret.size() - 1);

			return ret;
		}

		return "";
	}

	export std::string OSVersion() noexcept
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

	export std::wstring to_wide(std::string_view in) noexcept
	{
		std::wstring wret;
		auto         size = MultiByteToWideChar(CP_UTF8, 0, in.data(), -1, nullptr, 0);
		wret.resize(as<size_t>(size));
		if (size = MultiByteToWideChar(CP_UTF8, 0, in.data(), -1, wret.data(), size); size == 0)
			return {};
		return wret;
	}

} // namespace deckard
