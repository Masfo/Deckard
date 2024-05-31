cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES  ON)

find_package(Catch2 CONFIG REQUIRED)

project(tests VERSION 0.0.1 LANGUAGES CXX)

add_executable(tests)


target_compile_definitions(tests PRIVATE UNICODE)
target_compile_definitions(tests PRIVATE _UNICODE)

target_compile_definitions(tests PRIVATE NOMINMAX)
target_compile_definitions(tests PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(tests PRIVATE WIN32_EXTRA_LEAN)
target_compile_options(tests PRIVATE /nologo)
target_compile_options(tests PRIVATE -Zc:__cplusplus /Zc:alignedNew)
target_compile_options(tests PRIVATE /utf-8)
target_compile_options(tests PRIVATE /EHsc)

target_compile_options(tests  PRIVATE /fp:precise)
target_compile_options(tests PRIVATE /diagnostics:caret)
target_compile_options(tests PRIVATE /Zc:preprocessor)
target_compile_options(tests PRIVATE /permissive-)
target_compile_options(tests PRIVATE /std:c++latest)
target_compile_options(tests PRIVATE /Zc:__cplusplus)

target_compile_options(tests PRIVATE /W4)

target_compile_options(tests PRIVATE /arch:SSE2)


target_sources(tests PUBLIC
    test-main.cpp

    # Convert to
    tests/to_hex_test.cpp
    tests/sha2_test.cpp
    tests/base64_test.cpp

    
    # Scripting
    tests/scripting/lexer_test.cpp
    tests/scripting/parser_test.cpp

    # Math
    tests/math_vec_test.cpp
    tests/math_matrix_test.cpp


    # UTF8
    tests/utf8_test.cpp

    # Misc
    tests/enumflag_test.cpp 
)


target_link_libraries(tests PRIVATE Catch2::Catch2WithMain deckard)

include(CTest)
include(Catch)
catch_discover_tests(tests)

# This is for because of Test Explorer, it runs the tests.exe from the out directory
target_compile_definitions(tests PRIVATE -DTEST_PATH="${CMAKE_SOURCE_DIR}/bin/tests/")


set_target_properties(tests PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib

    PDB_NAME tests
    PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/pdb"
)





if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
   # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

    # Debug
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")

        set_target_properties(tests PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")

        target_compile_options(tests  PRIVATE /JMC)    # Just my debugging
        target_compile_definitions(tests PRIVATE -DDEBUG)

        target_compile_options(tests  PRIVATE /Od)

        target_compile_options(tests  PRIVATE /RTC1)
        target_compile_options(tests  PRIVATE /GS)
        target_compile_options(tests  PRIVATE /Zi)     # /ZI edit/continue
        target_link_options(tests PRIVATE /INCREMENTAL)
        target_link_options(tests PRIVATE /ILK:${CMAKE_SOURCE_DIR}/bin/pdb/tests.ilk)


        target_link_options(tests PRIVATE /DEBUG)
        #target_link_options(tests PRIVATE /ALIGN:16)
    endif()

    # Release
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        set_target_properties(tests PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

        set_target_properties(tests PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")


        target_compile_definitions(tests PRIVATE -DNDEBUG)
        
        target_compile_options(tests  PRIVATE /O2  /Os)
        target_compile_options(tests  PRIVATE /GS-)

        target_link_options(tests PRIVATE /Release)


        target_link_options(tests PRIVATE /OPT:REF /OPT:ICF)

    endif()
endif()



target_compile_options(tests PRIVATE 
       
    #/wd5039 # pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc.
    #
    #/wd5262 # implicit fall-through occurs here; are you missing a break statement? Use [[fallthrough]] when a break 
    #        # statement is intentionally omitted between cases
    #/wd4710 # function not inlined
    #/wd4711 # function selected for automatic inline expansion
    #/wd5045 # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
    /wd5050 # Possible incompatible environment while importing module
    #
    #/wd4820 # bytes padding added after data member
    #
    #/wd4626 # assignment operator was implicitly defined as deleted
    #/wd5027 # move assignment operator was implicitly defined as deleted
    #/wd5026 # move constructor was implicitly defined as deleted
    #/wd4061 # switch of enum is not explicitly handled by a case label
    #
    #/wd4355 # 'this': used in base member initializer list
    #/wd4625 # copy constructor was implicitly defined as deleted
    #/wd5220 # a non-static data member with a volatile qualified type no longer implies
    #        # that compiler generated copy/move constructors and copy/move assignment operators are not trivial
    #/wd5204 # class has virtual functions, but its trivial destructor is not virtual; instances of 
    #        # objects derived from this class may not be destructed correctly
    #
    #/wd4686 # possible change in behavior, change in UDT return calling convention
    #
    /wd4324 # structure was padded due to alignment specifier
    #/wd5246 # the initialization of a subobject should be wrapped in braces
    /wd4273 # inconsistent dll linkage
)

