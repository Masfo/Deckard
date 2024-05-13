module;

#include <Windows.h>
#include <d3d9.h>
#include <dxgi1_4.h>

export module deckard.win32;

import std;
import deckard.as;
import deckard.assert;
import deckard.types;
import deckard.debug;
import deckard.helpers;


extern "C"
{
	// Hints for higher performance (choose discrete GPU by default)
	_declspec(dllexport) DWORD NvOptimusEnablement                  = 0x0000'0001;
	_declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x0000'0001;
}

namespace deckard::system
{

	export std::wstring to_wide(std::string_view in) noexcept
	{
		std::wstring wret;
		auto         size = MultiByteToWideChar(CP_UTF8, 0, in.data(), -1, nullptr, 0);
		wret.resize(as<size_t>(size));
		if (size = MultiByteToWideChar(CP_UTF8, 0, in.data(), -1, wret.data(), size); size == 0)
			return {};
		return wret;
	}

	export std::string from_wide(std::wstring_view wstr) noexcept
	{
		int         num_chars = WideCharToMultiByte(CP_UTF8, 0u, wstr.data(), (int)wstr.length(), nullptr, 0, nullptr, nullptr);
		std::string strTo;
		if (num_chars > 0)
		{
			strTo.resize(as<u64>(num_chars));
			WideCharToMultiByte(CP_UTF8, 0u, wstr.data(), (int)wstr.length(), &strTo[0], num_chars, nullptr, nullptr);
		}
		return strTo;
	}

	template<typename T>
	requires std::is_pointer_v<T>
	T LoadDynamicAddress(std::string_view dll, std::string_view apiname,
						 const std::source_location& loc = std::source_location::current()) noexcept
	{
		auto library = LoadLibraryA(dll.data());
		if (not library)
		{
			dbg::println("{}({}) library '{}' not found.", loc.file_name(), loc.line(), dll);
			return nullptr;
		}

		T function = std::bit_cast<T>(GetProcAddress(library, apiname.data()));
		if (not function)
		{
			dbg::println("{}({}) function '{}' not found.", loc.file_name(), loc.line(), apiname);
			return nullptr;
		}

		return function;
	}

	std::string GetLocalRegistryValue(std::string_view key, std::string_view value) noexcept
	{

		using Func_RegOpenKeyExA    = LSTATUS(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
		using Func_RegQueryValueExA = LSTATUS(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

		static auto RegOpenKeyExA_F    = LoadDynamicAddress<Func_RegOpenKeyExA*>("advapi32.dll", "RegOpenKeyExA");
		static auto RegQueryValueExA_F = LoadDynamicAddress<Func_RegQueryValueExA*>("advapi32.dll", "RegQueryValueExA");

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

	export std::string GetOS() noexcept
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

	export u64 GetRAM() noexcept
	{
		MEMORYSTATUSEX status{};
		status.dwLength = sizeof(status);
		GlobalMemoryStatusEx(&status);

		return as<u64>(status.ullTotalPhys);
	}

	export std::string GetRAMString() noexcept { return PrettyBytes(GetRAM()); }

	export std::string GetGPU() noexcept
	{
		std::string                    result{"GPU: "};
		std::unique_ptr<IDXGIFactory4> factory{};

		CreateDXGIFactory1(__uuidof(IDXGIFactory3), (void**)&factory);
		if (!factory)
			return result;

		std::unique_ptr<IDXGIAdapter3> adapter;
		factory->EnumAdapters(0, std::bit_cast<IDXGIAdapter**>(&adapter));
		factory->Release();
		factory.release();

		if (!adapter)
			return result;

		DXGI_ADAPTER_DESC1 desc{0};
		if (S_OK != adapter->GetDesc1(&desc))
			return result;

		adapter->Release();
		adapter.release();


		return std::format("GPU: {}, ({})", from_wide(desc.Description), PrettyBytes(desc.DedicatedVideoMemory));
	}


} // namespace deckard::system
