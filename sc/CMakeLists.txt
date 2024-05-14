
add_executable(sc)



set_property(TARGET sc PROPERTY CXX_STANDARD          23)
set_property(TARGET sc PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET sc PROPERTY CXX_EXTENSIONS        OFF)




set_target_properties(sc PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib

    PDB_NAME sc
    PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/pdb"
)



if(MSVC)
    
    target_compile_options(sc PRIVATE /nologo)

    target_compile_definitions(sc PRIVATE UNICODE)
    target_compile_definitions(sc PRIVATE _UNICODE)
    target_compile_definitions(sc PRIVATE NOMINMAX)
    target_compile_definitions(sc PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(sc PRIVATE WIN32_EXTRA_LEAN)
    
    target_compile_options(sc PRIVATE /W4)
    target_compile_options(sc PRIVATE /EHsc)

    target_compile_options(sc PRIVATE /Zc:preprocessor)
    target_compile_options(sc PRIVATE /permissive-)
    target_compile_options(sc PRIVATE /std:c++latest)
    target_compile_options(sc PRIVATE /Zc:__cplusplus /Zc:alignedNew)
    target_compile_options(sc PRIVATE /utf-8)


    target_compile_options(sc PRIVATE /fp:precise)
    target_compile_options(sc PRIVATE /diagnostics:caret)

    set_target_properties(sc PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(sc PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

     # Debug
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    
        set_target_properties(sc PROPERTIES MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")

        set_target_properties(sc PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")


        target_compile_definitions(sc PRIVATE -DDEBUG)

        target_compile_options(sc PRIVATE /JMC)    # Just my debugging
        target_compile_options(sc PRIVATE /Od)
        target_compile_options(sc PRIVATE /RTC1)
        target_compile_options(sc PRIVATE /GS)

        target_compile_options(sc PRIVATE /Zi)     # /ZI edit/continue
    
        target_link_options(sc PRIVATE /DEBUG)
    endif()
    
    # Release
    if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    
        set_target_properties(sc PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")

        target_compile_definitions(sc PRIVATE -DNDEBUG)

        target_compile_options(sc PRIVATE /O2 /Os)
        target_compile_options(sc PRIVATE /GS-)
        target_compile_options(sc PRIVATE /Gw /MP)
        
        target_link_options(sc PRIVATE /RELEASE)
        target_link_options(sc PRIVATE /MERGE:.pdata=.text /MERGE:.rdata=.text)
        target_link_options(sc PRIVATE /DYNAMICBASE:NO)

        # Undocumented options
        target_link_options(sc PRIVATE  /emittoolversioninfo:no /emitpogophaseinfo)
    endif()

endif()



set_target_properties(sc PROPERTIES DEBUG_POSTFIX "d")


set(SCRIPTCOMPILER_SRCS 
	main.cpp
)

target_sources(sc PUBLIC ${SCRIPTCOMPILER_SRCS})

set(SC_MODULE_SOURCES
    $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/buildnumber.ixx>
)

target_sources(sc
	PUBLIC
    FILE_SET CXX_MODULES 
	FILES ${SC_MODULE_SOURCES}
)

target_link_libraries(sc deckard)




target_compile_options(sc PRIVATE 
       
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

find_program(BUILDINC_TOOL "buildinc")
if(BUILDINC_TOOL)
    add_custom_command(TARGET sc
                        PRE_BUILD
                        COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/buildnumber.ixx scbuild -q -m 
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        COMMENT "Increase build number"
    )
endif()