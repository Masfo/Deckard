
add_executable(sc)


target_compile_definitions(sc PRIVATE UNICODE)
target_compile_definitions(sc PRIVATE _UNICODE)

target_compile_definitions(sc PRIVATE NOMINMAX)
target_compile_definitions(sc PRIVATE WIN32_LEAN_AND_MEAN)
target_compile_definitions(sc PRIVATE WIN32_EXTRA_LEAN)

set_property(TARGET sc PROPERTY CXX_STANDARD          23)
set_property(TARGET sc PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET sc PROPERTY CXX_EXTENSIONS        OFF)



set_target_properties(sc PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
    PDB_NAME sc
    PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/pdb"
)

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

if (${CMAKE_BUILD_TYPE} MATCHES "Release")

    
    target_link_options(sc PRIVATE /MERGE:.pdata=.text /MERGE:.rdata=.text)
    target_link_options(sc PRIVATE /DYNAMICBASE:NO)
    set_target_properties(sc PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

    target_compile_definitions(sc PRIVATE -DNDEBUG)
 
    target_compile_options(sc  PRIVATE /O2 /Os)
    target_compile_options(sc  PRIVATE /GS-)
    target_compile_options(sc  PRIVATE /GF)

    target_compile_options(sc  PRIVATE /Gw /MP)

    target_link_options(sc PRIVATE /RELEASE)
    target_link_options(sc PRIVATE /INCREMENTAL:NO)
    target_link_options(sc PRIVATE /OPT:REF /OPT:ICF)

elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug" OR ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")

    target_compile_definitions(sc PRIVATE -DDEBUG)

    target_compile_options(sc  PRIVATE /JMC)    # Just my debugging
    target_compile_options(sc  PRIVATE /RTC1)
    target_compile_options(sc  PRIVATE /Od)
    target_compile_options(sc  PRIVATE /GS)
    #target_compile_options(sc  PRIVATE /Ob1)

    if (POLICY CMP0141)
        cmake_policy(SET CMP0141 NEW)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()


    target_link_libraries(sc PRIVATE ${DEBUG_LIBS})

    target_link_options(sc PRIVATE /DEBUG  /NOVCFEATURE /NOCOFFGRPINFO)

endif()

target_compile_features(sc PRIVATE cxx_std_23) 

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