
cmake_minimum_required (VERSION 3.29)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES  ON)


project(deckard CXX)
add_library(deckard STATIC)

set_target_properties(deckard PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib

    PDB_NAME deckard
    PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/pdb"
)



#set_target_properties(deckard PROPERTIES DEBUG_POSTFIX "d")


if(MSVC)
    
    target_compile_options(deckard PRIVATE /nologo)

    target_compile_definitions(deckard PRIVATE UNICODE)
    target_compile_definitions(deckard PRIVATE _UNICODE)
    target_compile_definitions(deckard PRIVATE NOMINMAX)
    target_compile_definitions(deckard PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(deckard PRIVATE WIN32_EXTRA_LEAN)
    
    target_compile_options(deckard PRIVATE /W4)
    target_compile_options(deckard PRIVATE /EHsc)

    target_compile_options(deckard PRIVATE /Zc:preprocessor)
    target_compile_options(deckard PRIVATE /permissive-)
    target_compile_options(deckard PRIVATE /std:c++latest)
    target_compile_options(deckard PRIVATE /Zc:__cplusplus /Zc:alignedNew)
    target_compile_options(deckard PRIVATE /utf-8)
    target_compile_options(deckard PRIVATE /experimental:module)


    target_compile_options(deckard PRIVATE /fp:precise)
    target_compile_options(deckard PRIVATE /diagnostics:caret)

    target_compile_options(deckard PRIVATE /arch:SSE2)

        set_target_properties(
            deckard
            PROPERTIES
            POSITION_INDEPENDENT_CODE ON)

    #set_target_properties(deckard PROPERTIES MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

    # Enable Hot Reload for MSVC compilers if supported.



     # Debug
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    
        #set_target_properties(deckard PROPERTIES MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
        if (POLICY CMP0141)
          cmake_policy(SET CMP0141 NEW)
          set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
        endif()

        set_target_properties(deckard PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")


        target_compile_definitions(deckard PRIVATE -DDEBUG)

        target_compile_options(deckard PRIVATE /JMC)    # Just my debugging
        target_compile_options(deckard PRIVATE /Od)
        target_compile_options(deckard PRIVATE /DEBUG)

        target_compile_options(deckard PRIVATE /RTC1)
        target_compile_options(deckard PRIVATE /GS)
        target_link_options(deckard PRIVATE /INCREMENTAL)
        target_link_options(deckard PRIVATE /ILK:${CMAKE_SOURCE_DIR}/bin/pdb/deckard.ilk)


        target_compile_options(deckard PRIVATE /Zi)     # /ZI edit/continue
    

    endif()
    
    # Release
    if (${CMAKE_BUILD_TYPE} MATCHES "Release" OR ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
    
        target_link_options(deckard PRIVATE /DEBUG)
        set_target_properties(deckard PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
        set_target_properties(deckard PROPERTIES POSITION_INDEPENDENT_CODE ON)
        set_target_properties(deckard PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

        target_compile_definitions(deckard PRIVATE -DNDEBUG)

        target_compile_options(deckard PRIVATE /O2 /Os)
        target_compile_options(deckard PRIVATE /GS-)
        target_compile_options(deckard PRIVATE /Gw /MP)
        
        target_link_options(deckard PRIVATE /RELEASE)


        # Undocumented options
        target_link_options(deckard PRIVATE  /emittoolversioninfo:no /emitpogophaseinfo)

        # # Generate out stub
        # if(NOT EXISTS ${CMAKE_SOURCE_DIR}/stub.bin)
        #     set(CMAKE_DISABLE_SOURCE_CHANGES OFF)   
        #     file(WRITE newstub.txt "4D 5A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
        #     # CertUtil has been included since Windows XP
        #     execute_process( COMMAND certutil -f -decodehex newstub.txt stub.bin WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        #     file(REMOVE newstub.txt)
        #     set(CMAKE_DISABLE_SOURCE_CHANGES ON)   
        # endif()
        # 
        # # use our own stub
        # if(EXISTS ${CMAKE_SOURCE_DIR}/stub.bin)
        #     target_link_options(deckard PRIVATE /stub:${CMAKE_SOURCE_DIR}/stub.bin )
        # else()
        #     message(WARNING "Could not find stub.bin")
        # endif()


    endif()


    # STD hack
    set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
    configure_file(	
            $ENV{VCToolsInstallDir}modules/std.ixx
            ${CMAKE_CURRENT_LIST_DIR}/std.ixx
            COPYONLY
    )
    set(CMAKE_DISABLE_SOURCE_CHANGES ON)

endif()

set(DECKARD_SRCS
		deckard.ixx
         $<$<CONFIG:RelWithDebInfo,Release>:buildnumber.ixx>

		debug/debug.ixx
		debug/assert.ixx
		file/file.ixx
		file/fileMonitor.ixx

        # Config
        config/ini.ixx

        # Math
        math/math.ixx
        math/ease_functions.ixx
        math/math_formatter.ixx
        math/matrix.ixx
        math/math_utility.ixx
        math/vec_generic.ixx
        math/vec_sse_generic.ixx
        math/vec2_sse.ixx
        math/vec3_sse.ixx
        math/vec4_sse.ixx
        math/quaternion.ixx

        # Net
        net/net.ixx
       
        # Taskpool
        taskpool/taskpool.ixx


        graphs/binarytree.ixx

        utils/base64.ixx
        utils/scope_exit.ixx
        utils/hash.ixx
        utils/helpers.ixx
        utils/generator.ixx     # Remove when MSVC has it
        utils/uuid.ixx
        utils/sha2.ixx
        utils/cpuid.ixx

        unicode/utf8.ixx
        unicode/utf8_codepoints.ixx
        unicode/utf8_xid.ixx
        unicode/utf8_xid_tables.ixx
        unicode/utf8_ascii.ixx
        unicode/utf8file.ixx




        $<$<PLATFORM_ID:Windows>:utils/system_win32.ixx>
        

        # Scripting
        scripting/lexer/lexer.ixx
        scripting/parser/parser.ixx

        # App
        app/window.ixx
        app/app.ixx


		types/types.ixx
        types/as.ixx
        types/enum_flags.ixx
        types/helpertypes.ixx

        $<$<PLATFORM_ID:Windows>:win32/win32.ixx>
		# win32/DTE.ixx

        archive/archive.ixx
        compress/zstd.ixx

        std.ixx
)

# $<$<PLATFORM_ID:Linux,Windows>:OpenGL::GL>
# $<$      <PLATFORM_ID:Windows> :win32/win32.ixx>
# $<$<NOT:$<PLATFORM_ID:Windows>>:file.cpp>


target_sources(deckard PUBLIC
)

target_sources(deckard
	PUBLIC
    FILE_SET CXX_MODULES 
	FILES ${DECKARD_SRCS}
)

add_subdirectory(external/sqlite3)
add_subdirectory(external/zstd)



add_dependencies(deckard sqlite3 ZSTD)
 
set(THIRD_PARTY_LIBS
    dxgi.lib
    sqlite3
    ZSTD
    Shcore.lib      # SetProcessDpiAwareness
    Dwmapi.lib      # DwmSetWindowAttribute
   # libcpmtd.lib
   # libcmtd.lib
   # libconcrtd.lib
   # libvcruntimed.lib
   #libucrtd.lib
   #libvcruntimed.lib	
)

target_link_libraries(deckard PUBLIC ${THIRD_PARTY_LIBS})

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
 find_program(BUILDINC_TOOL "buildinc")
 if(BUILDINC_TOOL AND BUILDINC)
 add_custom_command(TARGET deckard
                    PRE_BUILD
                    COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/buildnumber.ixx deckard_build -q -m 
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    COMMENT "Increase build number"
 )
 endif()


#####################################
#####################################
if(DECKARD_SCRIPT)
add_subdirectory(dbc)
endif()


if(DECKARD_TESTS)
enable_testing()
add_subdirectory(tests)
endif()



