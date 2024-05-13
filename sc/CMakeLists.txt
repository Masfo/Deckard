
project(sc)
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