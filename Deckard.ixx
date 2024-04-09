
export module Deckard;


export import deckard.debug;
export import deckard.assert;
export import deckard.file;
export import deckard.filemonitor;
export import deckard.math;
export import deckard.types;

export import deckard.base64;

#if __cpp_lib_generator && __has_include(<generator>)
#error "Generator is supported, remove Deckard/TL version"
#else
export import deckard.generator;
#endif

// export import deckard.DTE;
export import deckard.win32;

export import DeckardBuild;
