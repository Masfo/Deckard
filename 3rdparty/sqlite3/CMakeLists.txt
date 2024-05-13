cmake_minimum_required (VERSION 3.23)


project(sqlite3)
add_library(sqlite3 STATIC)

set_target_properties(sqlite3 PROPERTIES 
       LINKER_LANGUAGE CXX
)

target_sources(sqlite3 PRIVATE sqlite3.c)


target_include_directories(sqlite3 INTERFACE ${CMAKE_CURRENT_LIST_DIR})

