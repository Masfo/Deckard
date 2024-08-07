module;
#include <Windows.h>

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
// Clumsy network simulator
export import deckard.net;


// export import deckard.DTE;
export import deckard.win32;

#ifndef _DEBUG
export import deckard_build;
#endif


class DeckardInit
{
public:
	DeckardInit()
	{
		//
		SetDllDirectoryW(L"");
	}
};

namespace deckard
{
	net::initializer net_dummy;

	DeckardInit deckard_init_dummy;

	export void init() { }
}; // namespace deckard
