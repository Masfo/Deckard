cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES  ON)

find_package(doctest CONFIG REQUIRED)

project(tests VERSION 0.0.1 LANGUAGES CXX)

add_executable(tests)


set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
configure_file(	
        $ENV{VCToolsInstallDir}modules/std.ixx
        ${CMAKE_CURRENT_LIST_DIR}/std.ixx
        COPYONLY
)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)

set(TEST_SOURCES
    std.ixx
)

target_sources(tests
	PUBLIC
    FILE_SET CXX_MODULES 
	FILES ${TEST_SOURCES}

)


target_sources(tests PUBLIC

    main.cpp
)


target_link_libraries(tests PRIVATE doctest::doctest)

    set_target_properties(tests PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
        PDB_NAME tests
        PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/pdb"
    )

