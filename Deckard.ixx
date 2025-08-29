module;
#include <Windows.h>
#include <commctrl.h>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <ios>
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
export import deckard.ini;
export import deckard.config;


// Math
export import deckard.math;



// Utils
export import deckard.base32;
export import deckard.base64;
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
export import deckard.grid;
export import deckard.stringhelper;
export import deckard.timers;
export import deckard.sbo;
export import deckard.threadutil;

#ifdef __cpp_pp_embed
#error("Use embed on something");
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
		AllocConsole();
	
		if (AttachConsole(ATTACH_PARENT_PROCESS) == 0)
		{
			AttachConsole(GetCurrentProcessId());
		}
	
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
		SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
		SetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	
		freopen_s(reinterpret_cast<FILE**>(stdout), "CON", "w", stdout);
		freopen_s(reinterpret_cast<FILE**>(stderr), "CON", "w", stderr);
		freopen_s(reinterpret_cast<FILE**>(stdin), "CON", "r", stdin);
	}
	else
	{
		FreeConsole();

	}
#ifdef _DEBUG
#endif
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


	// auto cmd     = GetCommandLineW();
	auto cmdline = utf8::view{commandline};


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
	auto CoInitializeEx     = system::get_address<CoInitializePtr*>("Ole32.dll", "CoInitializeEx");
	auto CoUninitialize     = system::get_address<CoUninitializePtr*>("Ole32.dll", "CoUninitialize");

	//InitCommonControls();

	if (CoInitializeEx)
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

//	dbg::println("Memory usage: {}MB", system::process_ram_usage() / 1_MiB);

	deckard::random::initialize();
	net::initialize();
	// dbg::println("Initialized");


	// main
	int ret = deckard_main(cmdline);

//	dbg::println("Memory usage after deckard_main: {}MB", system::process_ram_usage() / 1_MiB);


	//
	// dbg::println("Deinitializing");

	if (CoUninitialize)
		CoUninitialize();

	net::deinitialize();
	redirect_console(false);

	return ret;
}
