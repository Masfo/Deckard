module;
#include <Windows.h>
#include <objbase.h>

export module deckard;


export import deckard.debug;
export import deckard.assert;
export import deckard.types;
export import deckard.as;
export import deckard.helpertypes;
export import deckard.uuid;
export import deckard.enums;

// File
export import deckard.filemonitor;
export import deckard.file;


// Math
export import deckard.math;


// Utils
export import deckard.base64;
export import deckard.generator;
export import deckard.helpers;
export import deckard.utils.hash;
export import deckard.scope_exit;
export import deckard.uuid;
export import deckard.sha2;
export import deckard.cpuid;
export import deckard.system;
export import deckard.ringbuffer;
export import deckard.random;
export import deckard.function_ref;
export import deckard.serializer;
export import deckard.grid;

// UTF8
export import deckard.utf8;

// Graph
export import deckard.graph;

// Scripting
export import deckard.lexer;
export import deckard.parser;

// Taskpool
export import deckard.taskpool;


// DB
export import deckard.archive;

// ZSTD
export import deckard.zstd;

// Net
export import deckard.net;

// App
export import deckard.app;


// export import deckard.DTE;
export import deckard.win32;

#ifndef _DEBUG
export import deckard_build;
#endif

#pragma region !Deckard init/deinit

namespace deckard
{


	using CoInitializePtr   = HRESULT(LPVOID, DWORD);
	using CoUninitializePtr = void(void);

	export void initialize()
	{
		deckard::dbg::redirect_console(true);

		SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT);
		SetDllDirectoryW(L"");

		auto CoInitializeEx = system::get_address<CoInitializePtr*>("Ole32.dll", "CoInitializeEx");
		if (CoInitializeEx)
			CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		deckard::random::initialize();
		net::initialize();
		dbg::println("Initialized");
	}

	export void deinitialize()
	{
		dbg::println("Deinitializing");

		auto CoUninitialize = system::get_address<CoUninitializePtr*>("Ole32.dll", "CoUninitialize");
		if (CoUninitialize)
			CoUninitialize();

		deckard::dbg::redirect_console(false);

		net::deinitialize();
	}
}; // namespace deckard

class DeckardInitializer final
{
public:
	DeckardInitializer() noexcept { deckard::initialize(); }

	~DeckardInitializer() noexcept { deckard::deinitialize(); }
};

static const auto global_init = [] { return std::make_unique<DeckardInitializer>(); }();
#pragma endregion


extern "C" int deckard_main();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return deckard_main(); }
