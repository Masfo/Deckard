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
		assert::if_true(library != nullptr, std::format("{}({}) library '{}' not found.", loc.file_name(), loc.line(), dll));

		T function = reinterpret_cast<T>(static_cast<void *>(GetProcAddress(library, apiname.data())));
		assert::if_true(function != nullptr, std::format("{}({}) function '{}' not found.", loc.file_name(), loc.line(), apiname));

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
		std::unique_ptr<IDXGIFactory4> factory{};

		CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void **)&factory);

		std::unique_ptr<IDXGIAdapter3> adapter;
		factory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter **>(&adapter));

		DXGI_ADAPTER_DESC1 desc{0};
		adapter->GetDesc1(&desc);

		std::cout << "Dedicated vram: " << desc.DedicatedVideoMemory << "\n";

		factory->Release();

		// while (true)
		{
			Sleep(500);
			DXGI_QUERY_VIDEO_MEMORY_INFO videoMemInfo{0};

			adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemInfo);
			std::cout << "Group local:\n";
			std::cout << "\t * GPU RAM  : " << videoMemInfo.CurrentUsage / 1'024 / 1'024 << " MB\n";
			std::cout << "\t * Budget   : " << videoMemInfo.Budget / 1'024 / 1'024 << "MB\n";
			std::cout << "\t * Available: " << videoMemInfo.AvailableForReservation / 1'024 / 1'024 << "MB\n\n";
		}

		adapter->Release();


		using D3DCREATETYPE = LPDIRECT3D9(unsigned int);
		LPDIRECT3D9            lpD3D9{nullptr};
		D3DADAPTER_IDENTIFIER9 id{0};
		std::string            result{"Unknown"};
		auto                   Direct3DCreate9 = system::GetAddress<D3DCREATETYPE *>("d3d9.dll", "Direct3DCreate9");
		if (Direct3DCreate9 == nullptr)
			return result;

		lpD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
		if (lpD3D9 == nullptr)
			return result;

		HRESULT hr{S_OK};

		hr = lpD3D9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &id);
		if (SUCCEEDED(hr))
		{
			const u32 driver_product    = HIWORD(id.DriverVersion.HighPart);
			const u32 driver_version    = LOWORD(id.DriverVersion.HighPart);
			const u32 driver_subversion = HIWORD(id.DriverVersion.LowPart);
			const u32 driver_build      = LOWORD(id.DriverVersion.LowPart);

			result = std::format(
				"{}, v{}.{}.{}.{} {}", id.Description, driver_product, driver_version, driver_subversion, driver_build, id.Driver);
		}
		return result;
	}


} // namespace deckard::system
