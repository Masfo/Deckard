module;
#include <Windows.h>
#include <commctrl.h>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <ios>
#include <objbase.h>

export module deckard;

export import deckard.as;
export import deckard.assert;
export import deckard.debug;
export import deckard.enums;
export import deckard.helpertypes;
export import deckard.log;
export import deckard.types;
export import deckard.uuid;
export import deckard.image;

export import deckard.allocator;

// File
export import deckard.config;
export import deckard.file;
export import deckard.filemonitor;
export import deckard.ini;

// Formats
export import deckard.qoi;


// Math
export import deckard.math;


// Utils
export import deckard.arrays;
export import deckard.base32;
export import deckard.base64;
export import deckard.bigint;
export import deckard.commandline;
export import deckard.colors;
export import deckard.cpuid;
export import deckard.function_ref;
export import deckard.grid;
export import deckard.helpers;
export import deckard.hmac;
export import deckard.random;
export import deckard.ringbuffer;
export import deckard.sbo;
export import deckard.scope_exit;
export import deckard.serializer;
export import deckard.sha;
export import deckard.stringhelper;
export import deckard.platform;
export import deckard.threadutil;
export import deckard.timers;
export import deckard.utils.hash;
export import deckard.uuid;
export import deckard.logger;

#ifdef __cpp_lib_optional_ref
#error ("use optional ref instead");
#endif


export import deckard.monocypher;

#ifdef __cpp_pp_embed
#error ("Use embed on something");
#endif

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
export import deckard.db;

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


void redirect_console(bool show)
{
	if (show)
	{
		if (GetConsoleWindow() == nullptr)
		{
			if (AttachConsole(ATTACH_PARENT_PROCESS) == 0)
			{
				if (AllocConsole() == 0)
				{
					return;
				}
			}
		}

		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);

		constexpr DWORD output_mode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), output_mode);
		SetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), output_mode);

		FILE* dummy = nullptr;
		freopen_s(&dummy, "CONOUT$", "w", stdout);
		freopen_s(&dummy, "CONOUT$", "w", stderr);
		freopen_s(&dummy, "CONIN$", "r", stdin);

		std::ios::sync_with_stdio(true);

		std::cout.clear();
		std::cerr.clear();
		std::cin.clear();
		std::wcout.clear();
		std::wcerr.clear();
		std::wcin.clear();
	}
	else if (GetConsoleWindow() != nullptr)
	{
		FreeConsole();
	}
}

using namespace deckard;

extern "C" i32 deckard_main(utf8::view);

#if 0
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR commandline, int) 
{ 
	SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT);
	SetDllDirectoryW(L"");

	using CoInitializePtr   = HRESULT(LPVOID, DWORD);
	using CoUninitializePtr = void(void);
	auto CoInitializeEx     = system::get_address<CoInitializePtr*>("Ole32.dll", "CoInitializeEx");
	auto CoUninitialize     = system::get_address<CoUninitializePtr*>("Ole32.dll", "CoUninitialize");

	if (CoInitializeEx)
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	redirect_console(true);

	deckard::random::initialize();
	net::initialize();


	// main
	int ret = deckard_main(U"");

	//

	if (CoUninitialize)
		CoUninitialize();

	redirect_console(false);
	net::deinitialize();

	return ret;
}
#endif
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR commandline, int)
{


	// clang-format off
	#if 0
		
	{+[](){};}

	{[&][[]](){};}
	
	{[][[]](){[][[]](){[][[]](){[][[]](){[][[]](){[][[]](){[][[]](){};};};};};};};}

	[]{};
	[](){};
	+[]{};

	!+[]{};
	!+[](){};
	!+[][[]](){};
	{-!&*+[][[]](){};}
	{~-!&*+[][[]](){};}
	{~-!&*+[][[]](...){};}
	
	{[]{};}
	{[][[]]{};}
	{[][[]](){};}
	{[=][[]](){};}
	{[&][[]](){};}
	
	#endif
	// clang-format on

	redirect_console(true);

	SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT);
	SetDllDirectoryW(L"");

	using CoInitializePtr   = HRESULT(LPVOID, DWORD);
	using CoUninitializePtr = void(void);
	auto CoInitializeEx     = platform::get_dynamic_address<CoInitializePtr*>("Ole32.dll", "CoInitializeEx");
	auto CoUninitialize     = platform::get_dynamic_address<CoUninitializePtr*>("Ole32.dll", "CoUninitialize");

	// InitCommonControls();

	if (CoInitializeEx)
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	//	dbg::println("Memory usage: {}MB", system::process_ram_usage() / 1_MiB);

	deckard::random::initialize();
	net::initialize();
	dbg::println("Initialized");


	// main
	auto cmdline = utf8::view{commandline};

	int ret = deckard_main(cmdline);

	//	dbg::println("Memory usage after deckard_main: {}MB", system::process_ram_usage() / 1_MiB);


	//
	dbg::println("Deinitializing");

	if (CoUninitialize)
		CoUninitialize();

	net::deinitialize();
	redirect_console(false);

	return ret;
}
