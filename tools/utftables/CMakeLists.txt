

cmake_minimum_required (VERSION 3.30)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)


project(utftables LANGUAGES CXX)

add_executable(utftables)

set_target_properties(utftables PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_SCAN_FOR_MODULES ON
    # COMPILE_WARNING_AS_ERROR ON
)

set_target_properties(utftables PROPERTIES DEBUG_POSTFIX "d")

set_target_properties(utftables PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_LIST_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_LIST_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/lib

    PDB_NAME utftables
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/pdb"
)

if(MSVC)
    
    target_compile_definitions(utftables PRIVATE UNICODE)
    target_compile_definitions(utftables PRIVATE _UNICODE)
    target_compile_definitions(utftables PRIVATE NOMINMAX)
    target_compile_definitions(utftables PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(utftables PRIVATE WIN32_EXTRA_LEAN)
    
    target_compile_options(utftables PRIVATE /Zc:preprocessor)
    target_compile_options(utftables PRIVATE /permissive-)
    target_compile_options(utftables PRIVATE /std:c++latest)
    target_compile_options(utftables PRIVATE /Zc:__cplusplus)
    target_compile_options(utftables PRIVATE /utf-8)


    target_compile_options(utftables PRIVATE /fp:precise)
    target_compile_options(utftables PRIVATE /diagnostics:caret)

    set_target_properties(utftables PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(utftables PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    
    set_target_properties(utftables PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_target_properties(utftables PROPERTIES MSVC_DEBUG_INFORMATION_FORMAT  "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")

     # Debug
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    

        target_compile_definitions(utftables PRIVATE -DDEBUG)

        target_compile_options(utftables PRIVATE /W4)
        target_compile_options(utftables  PRIVATE /JMC)    # Just my debugging
        target_compile_options(utftables  PRIVATE /Od)
        target_compile_options(utftables  PRIVATE /RTC1)
        target_compile_options(utftables  PRIVATE /EHsc)
        target_compile_options(utftables  PRIVATE /GS)
        target_compile_options(utftables  PRIVATE /Zi)     # /ZI edit/continue
    
        target_link_options(utftables PRIVATE /INCREMENTAL)
        target_link_options(utftables PRIVATE /ILK:${CMAKE_CURRENT_LIST_DIR}/bin/pdb/utftables.ilk)

        target_link_options(utftables PRIVATE /DEBUG)
        #target_link_options(utftables PRIVATE /NOVCFEATURE /NOCOFFGRPINFO)

    endif()
    
    # Release
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    

        target_compile_definitions(utftables PRIVATE -DNDEBUG)

        target_compile_options(utftables PRIVATE /W4)
        target_compile_options(utftables PRIVATE /O2 /Os)
        target_compile_options(utftables PRIVATE /GS-)
        target_compile_options(utftables PRIVATE /EHsc)
        target_compile_options(utftables PRIVATE /Gw /MP)
        
        target_link_options(utftables PRIVATE /RELEASE)
        target_link_options(utftables PRIVATE /INCREMENTAL:NO)
        target_link_options(utftables PRIVATE /MERGE:.pdata=.text /MERGE:.rdata=.text)
        target_link_options(utftables PRIVATE /DYNAMICBASE:NO)

        # Undocumented options
        target_link_options(utftables PRIVATE  /emittoolversioninfo:no /emitpogophaseinfo)

        #  # Generate out stub
        # if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/stub.bin)
        #     set(CMAKE_DISABLE_SOURCE_CHANGES OFF)   
        #     file(WRITE newstub.txt "4D 5A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
        #     # CertUtil has been included since Windows XP
        #     execute_process( COMMAND certutil -f -decodehex newstub.txt stub.bin WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
        #     file(REMOVE newstub.txt)
        #     set(CMAKE_DISABLE_SOURCE_CHANGES ON)   
        # endif()
        # 
        # # use our own stub
        # if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/stub.bin)
        #     target_link_options(utftables PRIVATE /stub:${CMAKE_CURRENT_LIST_DIR}/stub.bin )
        # else()
        #     message(WARNING "Could not find stub.bin")
        # endif()

        # Manifest
        if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/manifest.txt")
            target_link_options(utftables PRIVATE /MANIFEST:EMBED)
            target_link_options(utftables PRIVATE /MANIFESTINPUT:${CMAKE_CURRENT_LIST_DIR}/manifest.txt)
        endif()

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


# Modules
target_sources(utftables
	PUBLIC
    FILE_SET CXX_MODULES 
	FILES 

    $<$<CONFIG:Release>:src/buildnumber.ixx>
    ${CMAKE_CURRENT_LIST_DIR}/std.ixx
    
)

# Sources
target_sources(utftables PRIVATE
	src/main.cpp
)


# Libraries
target_link_libraries(utftables PUBLIC 
    # lib here
)

find_program(BUILDINC_TOOL "buildinc" NO_CACHE)
if(BUILDINC_TOOL)
    add_custom_command(TARGET utftables
                    PRE_BUILD
                    COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/src/buildnumber.ixx BuildInc -q -m
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
                    COMMENT "Increase build number (module)")
endif()


# Warnings
target_compile_options(utftables PRIVATE 
       
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

include(ExternalProject)

set(CMAKE_DISABLE_SOURCE_CHANGES OFF)

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/bin/DerivedCodeProperties.txt)
file(DOWNLOAD https://www.unicode.org/Public/UCD/latest/ucd/DerivedCoreProperties.txt
        ${CMAKE_CURRENT_LIST_DIR}/bin/DerivedCoreProperties.txt)
message(STATUS Downloaded latest DerivedCoreProperties.txt)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/bin/UnicodeData.txt)
file(DOWNLOAD https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
        ${CMAKE_CURRENT_LIST_DIR}/bin/UnicodeData.txt)
message(STATUS Downloaded latest UnicodeData.txt)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/bin/CaseFolding.txt)
file(DOWNLOAD https://www.unicode.org/Public/UCD/latest/ucd/CaseFolding.txt
        ${CMAKE_CURRENT_LIST_DIR}/bin/CaseFolding.txt)
message(STATUS Downloaded latest CaseFolding.txt)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/bin/PropList.txt)
file(DOWNLOAD https://www.unicode.org/Public/UCD/latest/ucd/PropList.txt
        ${CMAKE_CURRENT_LIST_DIR}/bin/PropList.txt)
message(STATUS Downloaded latest PropList.txt)
endif()

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
