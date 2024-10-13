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
export import deckard.arrays;
export import deckard.bigint;
export import deckard.bitbuffer;

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


extern "C" int deckard_main();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	using namespace deckard;

	SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT);
	SetDllDirectoryW(L"");

	using CoInitializePtr   = HRESULT(LPVOID, DWORD);
	using CoUninitializePtr = void(void);
	auto CoInitializeEx     = system::get_address<CoInitializePtr*>("Ole32.dll", "CoInitializeEx");
	auto CoUninitialize     = system::get_address<CoUninitializePtr*>("Ole32.dll", "CoUninitialize");

	if (CoInitializeEx)
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	deckard::dbg::redirect_console(true);
	deckard::random::initialize();
	net::initialize();
	dbg::println("Initialized");


	// main
	int ret = deckard_main();

	//
	dbg::println("Deinitializing");

	if (CoUninitialize)
		CoUninitialize();

	deckard::dbg::redirect_console(false);
	net::deinitialize();

	return ret;
}
