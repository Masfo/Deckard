

cmake_minimum_required (VERSION 3.29)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)


project(dbc LANGUAGES CXX)

set(PROJECT_NAME example01)

add_executable(${PROJECT_NAME} WIN32)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_SCAN_FOR_MODULES ON
    # COMPILE_WARNING_AS_ERROR ON
)


set_target_properties(${PROJECT_NAME} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib

    PDB_NAME ${PROJECT_NAME}
    PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/pdb"
)

if(MSVC)
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_EXTRA_LEAN)
    
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor)
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive-)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++latest)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)


    target_compile_options(${PROJECT_NAME} PRIVATE /fp:precise)
    target_compile_options(${PROJECT_NAME} PRIVATE /diagnostics:caret)

    set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

    target_compile_options(${PROJECT_NAME} PRIVATE /arch:SSE2)

     # Debug
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    
        #set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d")


        set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")

        set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")


        target_compile_definitions(${PROJECT_NAME} PRIVATE -DDEBUG)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4)

        target_compile_options(${PROJECT_NAME} PRIVATE /JMC)    # Just my debugging
        target_compile_options(${PROJECT_NAME} PRIVATE /Od)
        target_compile_options(${PROJECT_NAME} PRIVATE /RTC1)
        target_compile_options(${PROJECT_NAME} PRIVATE /GS)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zi)     # /ZI edit/continue


        target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
        target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL)
        target_link_options(${PROJECT_NAME} PRIVATE /ILK:${CMAKE_SOURCE_DIR}/bin/pdb/${PROJECT_NAME}.ilk)

    endif()
    
    # Release
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    
        set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")

        target_compile_definitions(${PROJECT_NAME} PRIVATE -DNDEBUG)

        target_compile_options(${PROJECT_NAME} PRIVATE /W4)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2 /Os)
        target_compile_options(${PROJECT_NAME} PRIVATE /GS-)
        target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
        target_compile_options(${PROJECT_NAME} PRIVATE /Gw /MP)
        
        target_link_options(${PROJECT_NAME} PRIVATE /RELEASE)
        target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL:NO)
        target_link_options(${PROJECT_NAME} PRIVATE /MERGE:.pdata=.text /MERGE:.rdata=.text)
        target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE:NO)

        # Undocumented options
        target_link_options(${PROJECT_NAME} PRIVATE  /emittoolversioninfo:no /emitpogophaseinfo)


    endif()


    # Manifest
    if(EXISTS "${CMAKE_SOURCE_DIR}/manifest.txt")
        target_link_options(${PROJECT_NAME} PRIVATE /MANIFEST:EMBED)
        target_link_options(${PROJECT_NAME} PRIVATE /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/manifest.txt)
    endif()



endif()


# Modules
target_sources(${PROJECT_NAME}
	PUBLIC
    FILE_SET CXX_MODULES 
    BASE_DIRS
    ${CMAKE_SOURCE_DIR}

	FILES 
    $<$<CONFIG:RelWithDebInfo,Release>:src/buildnumber.ixx>
)

# Sources
target_sources(${PROJECT_NAME} PUBLIC
	src/main.cpp
)

# Libraries
add_dependencies(${PROJECT_NAME} deckard)
 
set(THIRD_PARTY_LIBS
    deckard
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_LIBS})

find_program(BUILDINC_TOOL "buildinc" NO_CACHE)
if(BUILDINC_TOOL)
    add_custom_command(TARGET ${PROJECT_NAME}
                    PRE_BUILD
                    COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/src/buildnumber.ixx ${PROJECT_NAME} -q -m
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
                    COMMENT "Increase build number (module)")
endif()


# Warnings
target_compile_options(${PROJECT_NAME} PRIVATE 
       
    #/wd5039 # pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc.
    #/wd5262 # implicit fall-through occurs here; are you missing a break statement? Use [[fallthrough]] when a break 
             # statement is intentionally omitted between cases

    #/wd4710 # function not inlined
    #/wd4711 # function selected for automatic inline expansion
    #/wd5045 # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
    #/wd5050 # Possible incompatible environment while importing module
    #/wd4820 # bytes padding added after data member
    #/wd4626 # assignment operator was implicitly defined as deleted
    #/wd5027 # move assignment operator was implicitly defined as deleted
    #/wd5026 # move constructor was implicitly defined as deleted
    #/wd4061 # switch of enum is not explicitly handled by a case label
    #/wd4355 # 'this': used in base member initializer list
    #/wd4625 # copy constructor was implicitly defined as deleted

    #/wd5220 # a non-static data member with a volatile qualified type no longer implies
             # that compiler generated copy/move constructors and copy/move assignment operators are not trivial

    #/wd5204 # class has virtual functions, but its trivial destructor is not virtual; instances of 
             # objects derived from this class may not be destructed correctly

    #/wd5205 # delete of an abstract class '??' that has a non-virtual destructor results in undefined behavior
    #/wd4686 # possible change in behavior, change in UDT return calling convention
   
    /wd4324 # structure was padded due to alignment specifier
    #/wd5246 # the initialization of a subobject should be wrapped in braces
    #/wd4273 # inconsistent dll linkage
)
