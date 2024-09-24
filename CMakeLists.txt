
cmake_minimum_required (VERSION 3.29.0)

project(deckard LANGUAGES CXX)
add_library(deckard STATIC)

set_target_properties(deckard PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_SCAN_FOR_MODULES ON
    # COMPILE_WARNING_AS_ERROR ON
)


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

    #target_compile_options(deckard PRIVATE /await:strict)


    target_compile_options(deckard PRIVATE /Zc:preprocessor /experimental:module)
    target_compile_options(deckard PRIVATE /std:c++latest)
    target_compile_options(deckard PRIVATE /Zc:__cplusplus /Zc:__STDC__)
    target_compile_options(deckard PRIVATE /utf-8)


    #target_compile_options(deckard PRIVATE /fp:fast)
    #target_compile_options(deckard PRIVATE /fp:contract)

    target_compile_options(deckard PRIVATE /diagnostics:caret)

    #target_compile_options(deckard PRIVATE /arch:AVX2)

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
    
        set_target_properties(deckard PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
        set_target_properties(deckard PROPERTIES POSITION_INDEPENDENT_CODE ON)
        set_target_properties(deckard PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

        target_compile_definitions(deckard PRIVATE -DNDEBUG)

        target_compile_options(deckard PRIVATE /O2 /Os)
        target_compile_options(deckard PRIVATE /GS-)
        target_compile_options(deckard PRIVATE /Gw /MP)
        target_compile_options(deckard PRIVATE /EHsc)
        target_compile_options(deckard PRIVATE /GF)


        
        target_link_options(deckard PRIVATE /RELEASE)
        target_link_options(deckard PRIVATE /MERGE:.pdata=.text /MERGE:.rdata=.text)






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

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(deckard PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(deckard INTERFACE Vulkan::Vulkan)



set(DECKARD_SRCS
		deckard.ixx
         $<$<CONFIG:RelWithDebInfo,Release>:buildnumber.ixx>

		debug/debug.ixx
		debug/assert.ixx

        # File
		file/file.ixx
		file/fileMonitor.ixx
        file/qoi.ixx

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
        net/types.ixx
        net/socket.ixx
        net/address.ixx
        net/client.ixx
        net/server.ixx
       
        # Taskpool
        taskpool/taskpool.ixx


        graphs/binarytree.ixx

        # Utils
        utils/base64.ixx
        utils/scope_exit.ixx
        utils/hash.ixx
        utils/helpers.ixx
        utils/generator.ixx     # Remove when MSVC has it
        utils/uuid.ixx
        utils/sha2.ixx
        utils/cpuid.ixx
        utils/ringbuffer.ixx
        utils/serializer.ixx
        utils/random.ixx
        utils/function_ref.ixx
        utils/grid.ixx

        # Unicode
        unicode/utf8.ixx
        unicode/utf8_codepoints.ixx
        unicode/utf8_xid.ixx
        unicode/utf8_xid_tables.ixx
        unicode/utf8_ascii.ixx
        unicode/utf8file.ixx
        unicode/utf8string.ixx

        # Graph
        graphs/graph.ixx
        graphs/avltree.ixx
        graphs/binarytree.ixx
        graphs/undirected.ixx


        $<$<PLATFORM_ID:Windows>:utils/system_win32.ixx>
        

        # Scripting
        scripting/lexer/lexer.ixx
        scripting/parser/parser.ixx

        # App
        app/app.ixx
        app/controller.ixx
        app/inputs.ixx

        # Vulkan
        app/vulkan.ixx

        app/vulkan/helpers.ixx
        app/vulkan/instance.ixx
        app/vulkan/device.ixx
        app/vulkan/surface.ixx
        app/vulkan/swapchain.ixx
        app/vulkan/command_buffer.ixx
        app/vulkan/fence.ixx
        app/vulkan/semaphore.ixx
        app/vulkan/images.ixx
        app/vulkan/debug.ixx


        # Types
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

target_include_directories(deckard PUBLIC ${CMAKE_CURRENT_LIST_DIR}/external/gl)

add_subdirectory(external/sqlite3)
add_subdirectory(external/zstd)

add_dependencies(deckard sqlite3 ZSTD)
 
set(THIRD_PARTY_LIBS
    dxgi.lib
    sqlite3
    ZSTD
    opengl32.lib
    ws2_32
    xinput
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
    /wd5205 # delete of an abstract class '??' that has a non-virtual destructor results in undefined behavior
    #
    #/wd4686 # possible change in behavior, change in UDT return calling convention
    #
    /wd4324 # structure was padded due to alignment specifier
    #/wd5246 # the initialization of a subobject should be wrapped in braces
    #/wd4273 # inconsistent dll linkage


)

###################################


#####################################
#####################################
if(PROJECT_IS_TOP_LEVEL)

    find_program(BUILDINC_TOOL "buildinc")
    if(BUILDINC_TOOL)
    add_custom_command(TARGET deckard
                       PRE_BUILD
                       COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/buildnumber.ixx deckard_build -q -m 
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                       COMMENT "Increase build number"
    )
    endif()

    add_subdirectory(tools/dbc)
    add_subdirectory(tools/utftables)

    
    
    enable_testing()
    add_subdirectory(tests)
    
    #add_subdirectory(example/empty)
    add_subdirectory(example/window)

endif()


