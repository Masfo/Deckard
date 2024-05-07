module;
#include <Windows.h>
#include <d3d9.h>
#include <dxgi1_4.h>

export module deckard.system;

import std;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.debug;
import deckard.helpers;
import deckard.helpertypes;
import deckard.win32;

extern "C"
{
	// Hints for higher performance (choose discrete GPU by default)
	_declspec(dllexport) DWORD NvOptimusEnablement                  = 0x0000'0001;
	_declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x0000'0001;
}

namespace deckard::system
{

	std::string GetLocalRegistryValue(std::string_view key, std::string_view value) noexcept
	{
		std::string ret;

		HKEY hKey{};
		auto dwError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, key.data(), 0u, (REGSAM)(KEY_READ | KEY_WOW64_64KEY), &hKey);
		if (dwError != ERROR_SUCCESS)
			return "";

		ULONG cb   = 0;
		ULONG Type = 0;

		dwError = RegQueryValueExA(hKey, value.data(), nullptr, &Type, nullptr, &cb);
		if (dwError != ERROR_SUCCESS)
			return "";

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

	template<typename T>
	T GetAddress(std::string_view dll, std::string_view apiname, const std::source_location &loc = std::source_location::current()) noexcept
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

	export std::string GetOS() noexcept
	{

		static constexpr std::string_view key = R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)";

		auto ProductName    = GetLocalRegistryValue(key, "ProductName");
		auto ReleaseId      = GetLocalRegistryValue(key, "ReleaseId");
		auto DisplayVersion = GetLocalRegistryValue(key, "DisplayVersion");
		auto CurrentBuild   = GetLocalRegistryValue(key, "CurrentBuild");
		auto UBR            = GetLocalRegistryValue(key, "UBR");

		if (ProductName.empty() or ReleaseId.empty() or CurrentBuild.empty() or UBR.empty())
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

	export std::string GetRAMString() noexcept { return PrettyBytes(system::GetRAM()); }

	export std::string GetGPU() noexcept
	{
		std::string                    result{"GPU: "};
		std::unique_ptr<IDXGIFactory4> factory{};

		CreateDXGIFactory1(__uuidof(IDXGIFactory3), (void **)&factory);
		if (!factory)
			return result;

		std::unique_ptr<IDXGIAdapter3> adapter;
		factory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter **>(&adapter));
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
