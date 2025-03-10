module;
#include <Windows.h>
#include <cstdio>
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


// Math
export import deckard.math;


// Utils
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
		static FILE* pNewStdout = nullptr;
		static FILE* pNewStderr = nullptr;


		if (AttachConsole(ATTACH_PARENT_PROCESS) == 0)
			AllocConsole();

		if (::freopen_s(&pNewStdout, "CONOUT$", "w", stdout) == 0)
			std::cout.clear();

		if (::freopen_s(&pNewStderr, "CONOUT$", "w", stderr) == 0)
			std::cerr.clear();

		std::ios::sync_with_stdio(1);
	}
	else
	{
		FreeConsole();
	}
#ifdef _DEBUG
#endif
}

using namespace deckard;

extern "C" i32 deckard_main(std::string_view);

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
	//dbg::println("Initialized");


	// main
	int ret = deckard_main(commandline);

	//
	//dbg::println("Deinitializing");

	if (CoUninitialize)
		CoUninitialize();

	redirect_console(false);
	net::deinitialize();

	return ret;
}
