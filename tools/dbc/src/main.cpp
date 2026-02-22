#include <Windows.h>
#include <emmintrin.h>
#include <xmmintrin.h>


import std;
import deckard;
import deckard.as;
import deckard.types;
import deckard.helpers;
import deckard.win32;
import deckard.math;
import deckard.utf8;
import deckard.lexer;
import deckard.serializer;

import deckard.app;


#ifndef _DEBUG
import dbc;
#endif
namespace fs = std::filesystem;
using namespace std::chrono_literals;

using namespace std::string_view_literals;
using namespace deckard;
using namespace deckard::utils;
using namespace deckard::system;
using namespace deckard::math;
using namespace deckard::utf8;


using crt_callback = int(void);

int PreMain1(void)
{
	OutputDebugStringA("premain");
	return 0;
}

int PreMain2(void)
{
	OutputDebugStringA("premain");
	return 0;
}

#pragma data_seg(".CRT$XIAC")
static crt_callback* autostart[] = {PreMain1, PreMain2};
#pragma data_seg() /* reset data-segment */




i32 deckard_main(utf8::view /*commandline*/) 

{


#ifndef _DEBUG
	std::print("dbc {} ({}), ", dbc::build::version_string, dbc::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
#endif


	return 0;
}
