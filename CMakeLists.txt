
cmake_minimum_required (VERSION 3.29)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES  ON)


project(deckard CXX)
add_library(deckard STATIC)


target_compile_definitions(deckard PRIVATE UNICODE)
target_compile_definitions(deckard PRIVATE _UNICODE)

target_compile_definitions(deckard PRIVATE NOMINMAX)
target_compile_definitions(deckard PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(deckard PRIVATE WIN32_EXTRA_LEAN)
target_compile_options(deckard PRIVATE /nologo)
target_compile_options(deckard PRIVATE -Zc:__cplusplus /Zc:alignedNew)
target_compile_options(deckard PRIVATE /utf-8)
target_compile_options(deckard PRIVATE /EHsc)
#target_compile_options(deckard PRIVATE /Za)

target_compile_options(deckard  PRIVATE /fp:precise)
target_compile_options(deckard PRIVATE /diagnostics:caret)


#target_compile_options(deckard PRIVATE /experimental:module)
target_compile_options(deckard PRIVATE /Zc:preprocessor)
target_compile_options(deckard PRIVATE /permissive-)
target_compile_options(deckard PRIVATE /std:c++latest)
target_compile_options(deckard PRIVATE /Zc:__cplusplus)

target_compile_options(deckard PRIVATE /W4)

set_target_properties(deckard PROPERTIES DEBUG_POSTFIX "d")

if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")


    # Debug
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")

        target_compile_options(deckard  PRIVATE /JMC)    # Just my debugging
        target_compile_definitions(deckard PRIVATE -DDEBUG)

        target_compile_options(deckard  PRIVATE /Od)

        target_compile_options(deckard  PRIVATE /RTC1)
        target_compile_options(deckard  PRIVATE /GS)
        target_compile_options(deckard  PRIVATE /Zi)     # /ZI edit/continue

        target_link_options(deckard PRIVATE /DEBUG)

        set_target_properties(deckard PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")

        #target_link_options(deckard PRIVATE /ALIGN:16)
    endif()

    # Release
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        #set_target_properties(deckard PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

        target_compile_definitions(deckard PRIVATE -DNDEBUG)
        
        target_compile_options(deckard  PRIVATE /O2  /Os)
        target_compile_options(deckard  PRIVATE /GS-)

        target_link_options(deckard PRIVATE /Release)
        target_link_options(deckard PRIVATE /INCREMENTAL:NO)


        target_link_options(deckard PRIVATE /OPT:REF /OPT:ICF)

        set_target_properties(deckard PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")


        # 0x800: LOAD_LIBRARY_SEARCH_SYSTEM32
        target_link_options(deckard PRIVATE /DEPENDENTLOADFLAG:0x800)
            
        # Undocumented options
        target_link_options(deckard PRIVATE  /emittoolversioninfo:no /emitpogophaseinfo)

    endif()
endif()

set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
configure_file(	
        $ENV{VCToolsInstallDir}modules/std.ixx
        ${CMAKE_CURRENT_LIST_DIR}/std.ixx
        COPYONLY
)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)

set(DECKARD_SRCS
		deckard.ixx
         $<$<CONFIG:Release>:buildnumber.ixx>

		debug/debug.ixx
		debug/assert.ixx
		file/File.ixx
		file/FileMonitor.ixx

        # Math
        math/matrix.ixx
        math/math_utility.ixx
        math/vec.ixx

        graphs/binarytree.ixx

        utils/base64.ixx
        utils/scope_exit.ixx
        utils/helpers.ixx
        utils/generator.ixx     # Remove when MSVC has it
        utils/uuid.ixx
        utils/sha2.ixx
        utils/cpuid.ixx
        utils/utf8.ixx

        $<$<PLATFORM_ID:Windows>:utils/system_win32.ixx>
        

        # Scripting
        scripting/tokenizer.ixx


		types/types.ixx
        types/as.ixx
        types/enum_flags.ixx
        types/helpertypes.ixx

         $<$<PLATFORM_ID:Windows>:win32/win32.ixx>
		#win32/DTE.ixx

        archive/archive.ixx

        std.ixx
)

target_sources(deckard
	PUBLIC
    FILE_SET CXX_MODULES 
	FILES ${DECKARD_SRCS}
)
 find_package(SQLite3 REQUIRED)

 
set(THIRD_PARTY_LIBS
    dxgi.lib
)
 target_link_libraries(deckard PRIVATE SQLite::SQLite3 ${THIRD_PARTY_LIBS})



#
target_compile_options(deckard PRIVATE 
       
    #/wd5039 # pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc.
    #
    #/wd5262 # implicit fall-through occurs here; are you missing a break statement? Use [[fallthrough]] when a break 
    #        # statement is intentionally omitted between cases
    #/wd4710 # function not inlined
    #/wd4711 # function selected for automatic inline expansion
    #/wd5045 # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
    #/wd5050 # Possible incompatible environment while importing module
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
    /wd5205 # delete of an abstract class '??' that has a non-virtual destructor results in undefined behavior
    #
    #/wd4686 # possible change in behavior, change in UDT return calling convention
    #
    /wd4324 # structure was padded due to alignment specifier
    #/wd5246 # the initialization of a subobject should be wrapped in braces
    #/wd4273 # inconsistent dll linkage


)

###################################
if(BUILDINC)
    find_program(BUILDINC_TOOL "buildinc" NO_CACHE)
    if(BUILDINC_TOOL)
    add_custom_command(TARGET deckard
                       PRE_BUILD
                       COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/buildnumber.ixx deckardBuild -q -m 
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                       COMMENT "Increase build number"
    )
    endif()
endif()


#####################################
#####################################
add_subdirectory(sc)

if(DECKARD_TESTS)
enable_testing()
add_subdirectory(tests)
endif()



