cmake_minimum_required (VERSION 3.23)

project(sqlite3)
add_library(sqlite3 STATIC sqlite3.c)

set_source_files_properties(sqlite3.c sqlite3.h PROPERTIES LANGUAGE C)

set_target_properties(sqlite3 PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib

)
if(MSVC)
    set_target_properties(sqlite3 PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug,RelWithDebInfo>:Debug>")
endif()


set_target_properties(sqlite3 PROPERTIES LINKER_LANGUAGE C)
target_include_directories(sqlite3 INTERFACE ${CMAKE_CURRENT_LIST_DIR})