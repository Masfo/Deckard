cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES  ON)

project(tests VERSION 0.0.1 LANGUAGES CXX)


add_executable(tests main.cpp)

find_package(doctest CONFIG REQUIRED)
target_link_libraries(tests PRIVATE doctest::doctest)

    set_target_properties(tests PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
        PDB_NAME tests
        PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/pdb"
    )


