
cmake_minimum_required (VERSION 3.25)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES  ON)


project(Deckard CXX)
add_library(Deckard)


target_compile_features(Deckard PRIVATE cxx_std_23) 

target_compile_definitions(Deckard PRIVATE UNICODE)
target_compile_definitions(Deckard PRIVATE _UNICODE)

target_compile_definitions(Deckard PRIVATE NOMINMAX)
target_compile_definitions(Deckard PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(Deckard PRIVATE WIN32_EXTRA_LEAN)
target_compile_options(Deckard PRIVATE /nologo)
target_compile_options(Deckard PRIVATE -Zc:__cplusplus /Zc:alignedNew)
target_compile_options(Deckard PRIVATE /utf-8)
target_compile_options(Deckard PRIVATE /EHsc)
#target_compile_options(Deckard PRIVATE /Za)

target_compile_options(Deckard  PRIVATE /fp:precise)
target_compile_options(Deckard PRIVATE /diagnostics:caret)


#target_compile_options(Deckard PRIVATE /experimental:module)
target_compile_options(Deckard PRIVATE /Zc:preprocessor)
target_compile_options(Deckard PRIVATE /permissive-)
target_compile_options(Deckard PRIVATE /std:c++latest)
target_compile_options(Deckard PRIVATE /Zc:__cplusplus)

target_compile_options(Deckard PRIVATE /W4)

set_target_properties(Deckard PROPERTIES DEBUG_POSTFIX "d")

if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")

    set_target_properties(Deckard PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Debug
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")

        target_compile_options(Deckard  PRIVATE /JMC)    # Just my debugging
        target_compile_definitions(Deckard PRIVATE -DDEBUG)

        target_compile_options(Deckard  PRIVATE /Od)

        target_compile_options(Deckard  PRIVATE /RTC1)
        target_compile_options(Deckard  PRIVATE /GS)
        target_compile_options(Deckard  PRIVATE /Zi)     # /ZI edit/continue

        target_link_options(Deckard PRIVATE /DEBUG)
        #target_link_options(Deckard PRIVATE /ALIGN:16)
    endif()

    # Release
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
        #set_target_properties(Deckard PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

        target_compile_definitions(Deckard PRIVATE -DNDEBUG)
        
        target_compile_options(Deckard  PRIVATE /O2  /Os)
        target_compile_options(Deckard  PRIVATE /GS-)

        target_link_options(Deckard PRIVATE /Release)
        target_link_options(Deckard PRIVATE /INCREMENTAL:NO)


        target_link_options(Deckard PRIVATE /OPT:REF /OPT:ICF)

        # 0x800: LOAD_LIBRARY_SEARCH_SYSTEM32
        target_link_options(Deckard PRIVATE /DEPENDENTLOADFLAG:0x800)
            
        # Undocumented options
        target_link_options(Deckard PRIVATE  /emittoolversioninfo:no /emitpogophaseinfo)

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
		Deckard.ixx
         $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/buildnumber.ixx>

		debug/debug.ixx
		debug/assert.ixx
		file/File.ixx
		file/FileMonitor.ixx

        math/matrix.ixx
        math/math_utility.ixx
        math/math.ixx
        math/vec.ixx

        graphs/binarytree.ixx

        utils/base64.ixx
        utils/scope_exit.ixx
        utils/helpers.ixx
        utils/generator.ixx     # Remove when MSVC has it
        utils/uuid.ixx
        utils/sha256.ixx
        
        # Scripting
        scripting/tokenizer.ixx


		types/types.ixx
        types/as.ixx
        types/enum_flags.ixx
        types/helpertypes.ixx

		win32/win32.ixx
		#win32/DTE.ixx

        std.ixx
)

target_sources(Deckard
	PUBLIC
    FILE_SET CXX_MODULES 
	FILES ${DECKARD_SRCS}
)


#
target_compile_options(Deckard PRIVATE 
       
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
    add_custom_command(TARGET Deckard
                       PRE_BUILD
                       COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/buildnumber.ixx DeckardBuild -q -m 
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                       COMMENT "Increase build number"
    )
    endif()
endif()


#####################################
#####################################
add_subdirectory(sc)

enable_testing()
add_subdirectory(tests)



######## 3rd party
# FetchContent_Declare(SQLite
#     URL https://www.sqlite.org/2023/sqlite-amalgamation-3430200.zip
# )
# FetchContent_Populate(SQLite)
# target_include_directories(Deckard PUBLIC "${sqlite_SOURCE_DIR}")

 # FetchContent_Declare(
 #     glm
 #     GIT_REPOSITORY https://github.com/g-truc/glm.git
 #     GIT_TAG        0.9.9.8
 #     SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm/
 # )