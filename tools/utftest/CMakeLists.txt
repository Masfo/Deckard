

cmake_minimum_required (VERSION 4.2)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)


project(utftest LANGUAGES CXX)

add_executable(utftest)

set_target_properties(utftest PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_SCAN_FOR_MODULES ON
    # COMPILE_WARNING_AS_ERROR ON
)

set_target_properties(utftest PROPERTIES DEBUG_POSTFIX "d")

set_target_properties(utftest PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib

    PDB_NAME utftest
    PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/pdb"
)

if(MSVC)
    
    target_compile_definitions(utftest PRIVATE UNICODE)
    target_compile_definitions(utftest PRIVATE _UNICODE)
    target_compile_definitions(utftest PRIVATE NOMINMAX)
    target_compile_definitions(utftest PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(utftest PRIVATE WIN32_EXTRA_LEAN)
    
    target_compile_options(utftest PRIVATE /Zc:preprocessor)
    target_compile_options(utftest PRIVATE /permissive-)
    target_compile_options(utftest PRIVATE /std:c++latest)
    target_compile_options(utftest PRIVATE /Zc:__cplusplus)
    target_compile_options(utftest PRIVATE /utf-8)


    target_compile_options(utftest PRIVATE /fp:precise)
    target_compile_options(utftest PRIVATE /diagnostics:caret)

    set_target_properties(utftest PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(utftest PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

     # Debug
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    
        set_target_properties(utftest PROPERTIES MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")

        set_target_properties(utftest PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")

        string(REGEX REPLACE "/MDd" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REGEX REPLACE "/W[0-3]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

        target_compile_definitions(utftest PRIVATE -DDEBUG)

        target_compile_options(utftest PRIVATE /W4)
        target_compile_options(utftest  PRIVATE /JMC)    # Just my debugging
        target_compile_options(utftest  PRIVATE /Od)
        #target_compile_options(utftest  PRIVATE /RTC1)
        target_compile_options(utftest  PRIVATE /EHsc)
        target_compile_options(utftest  PRIVATE /GS)
        target_compile_options(utftest  PRIVATE /Zi)     # /ZI edit/continue
    
        target_link_options(utftest PRIVATE /INCREMENTAL)
        target_link_options(utftest PRIVATE /ILK:${CMAKE_SOURCE_DIR}/bin/pdb/utftest.ilk)

        target_link_options(utftest PRIVATE /DEBUG)
        target_link_options(utftest PRIVATE /NOVCFEATURE /NOCOFFGRPINFO)

    endif()
    
    # Release
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    
        set_target_properties(utftest PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")

        target_compile_definitions(utftest PRIVATE -DNDEBUG)

        target_compile_options(utftest PRIVATE /W4)
        target_compile_options(utftest PRIVATE /O2 /Os)
        target_compile_options(utftest PRIVATE /GS-)
        target_compile_options(utftest PRIVATE /EHsc)
        target_compile_options(utftest PRIVATE /Gw /MP)
        
        target_link_options(utftest PRIVATE /RELEASE)
        target_link_options(utftest PRIVATE /INCREMENTAL:NO)
        target_link_options(utftest PRIVATE /MERGE:.pdata=.text /MERGE:.rdata=.text)
        target_link_options(utftest PRIVATE /DYNAMICBASE:NO)

        # Undocumented options
        target_link_options(utftest PRIVATE  /emittoolversioninfo:no /emitpogophaseinfo)

         # Generate out stub
        if(NOT EXISTS ${CMAKE_SOURCE_DIR}/stub.bin)
            set(CMAKE_DISABLE_SOURCE_CHANGES OFF)   
            file(WRITE newstub.txt "4D 5A 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
            # CertUtil has been included since Windows XP
            execute_process( COMMAND certutil -f -decodehex newstub.txt stub.bin WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
            file(REMOVE newstub.txt)
            set(CMAKE_DISABLE_SOURCE_CHANGES ON)   
        endif()

        # use our own stub
        if(EXISTS ${CMAKE_SOURCE_DIR}/stub.bin)
            target_link_options(utftest PRIVATE /stub:${CMAKE_SOURCE_DIR}/stub.bin )
        else()
            message(WARNING "Could not find stub.bin")
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
target_sources(utftest
	PUBLIC
    FILE_SET CXX_MODULES 
	FILES 

    $<$<CONFIG:Release>:src/buildnumber.ixx>
)

# Sources
target_sources(utftest PRIVATE
	src/main.cpp
)
add_dependencies(utftest deckard)
 

# Libraries
target_link_libraries(utftest PUBLIC 
    # lib here
    deckard
)

find_program(BUILDINC_TOOL "buildinc" NO_CACHE)
if(BUILDINC_TOOL)
    add_custom_command(TARGET utftest
                    PRE_BUILD
                    COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/src/buildnumber.ixx BuildInc -q -m
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
                    COMMENT "Increase build number (module)")
endif()


# Warnings
target_compile_options(utftest PRIVATE 
       
    #/wd5039 # pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc.
    #/wd5262 # implicit fall-through occurs here; are you missing a break statement? Use [[fallthrough]] when a break 
             # statement is intentionally omitted between cases

    #/wd4710 # function not inlined
    #/wd4711 # function selected for automatic inline expansion
    #/wd5045 # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
    /wd5050 # Possible incompatible environment while importing module
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

set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
# TODO: function to download files

function(download_file)
    set(url ${ARGV0})
    set(output ${ARGV1})
    if(NOT EXISTS ${output})
        file(DOWNLOAD ${url} ${output})
        message(STATUS "Downloaded latest ${output}")
    endif()
endfunction()

# download_file(https://www.unicode.org/Public/UCD/latest/ucd/DerivedCoreProperties.txt 
#              ${CMAKE_CURRENT_LIST_DIR}/bin/utf/DerivedCoreProperties.txt)
# 
download_file(https://www.unicode.org/Public/UCD/latest/ucd/UnicodeData.txt
              ${CMAKE_CURRENT_LIST_DIR}/bin/utf/UnicodeData.txt)

download_file(https://www.unicode.org/Public/UCD/latest/ucd/NormalizationTest.txt
              ${CMAKE_CURRENT_LIST_DIR}/bin/utf/NormalizationTest.txt)
# 
# download_file(https://www.unicode.org/Public/UCD/latest/ucd/CaseFolding.txt
#              ${CMAKE_CURRENT_LIST_DIR}/bin/utf/CaseFolding.txt)
# 
# download_file(https://www.unicode.org/Public/UCD/latest/ucd/Blocks.txt
#               ${CMAKE_CURRENT_LIST_DIR}/bin/utf/Blocks.txt)
# 
# download_file(https://www.unicode.org/Public/UCD/latest/ucd/PropList.txt
#               ${CMAKE_CURRENT_LIST_DIR}/bin/utf/PropList.txt)
#               
# download_file(https://www.unicode.org/Public/UCD/latest/ucd/NamesList.txt
#               ${CMAKE_CURRENT_LIST_DIR}/bin/utf/NamesList.txt)



set(CMAKE_DISABLE_SOURCE_CHANGES ON)
