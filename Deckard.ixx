
export module deckard;


export import deckard.debug;
export import deckard.assert;
export import deckard.types;
export import deckard.as;
export import deckard.helpertypes;
export import deckard.uuid;

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


// export import deckard.DTE;
export import deckard.win32;

#ifndef _DEBUG
export import deckard_build;
#endif
