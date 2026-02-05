# https://www.sqlite.org

cmake_minimum_required (VERSION 3.23)


set(SQLITE_ZIP_URL https://www.sqlite.org/2026/sqlite-amalgamation-3510200.zip)
get_filename_component(SQLITE_ZIP_NAME ${SQLITE_ZIP_URL} NAME)
get_filename_component(SQLITE_ZIP_NAME_NOEXT ${SQLITE_ZIP_URL} NAME_WLE)


if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/${SQLITE_ZIP_NAME})

    file(DOWNLOAD ${SQLITE_ZIP_URL} ${CMAKE_CURRENT_LIST_DIR}/${SQLITE_ZIP_NAME})
    
    # extract to temp
    file(ARCHIVE_EXTRACT 
         INPUT ${CMAKE_CURRENT_LIST_DIR}/${SQLITE_ZIP_NAME}
         DESTINATION ${CMAKE_CURRENT_LIST_DIR}
    )

    set(SQLITE_FILES
    sqlite3.h
    sqlite3.c
    )

    # copy files
    
    set(CMAKE_DISABLE_SOURCE_CHANGES OFF)

    # Copy the files, create folder 
    foreach(f IN LISTS SQLITE_FILES)
       file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/${SQLITE_ZIP_NAME_NOEXT}/${f} ${CMAKE_CURRENT_LIST_DIR}/${f} ONLY_IF_DIFFERENT)
    endforeach()

    set(CMAKE_DISABLE_SOURCE_CHANGES ON)

        # Remove temp
    file(REMOVE_RECURSE ${CMAKE_CURRENT_LIST_DIR}/${SQLITE_ZIP_NAME_NOEXT})

endif()


project(sqlite3 C)
add_library(sqlite3 OBJECT sqlite3.c)

set_target_properties(sqlite3 PROPERTIES
	C_STANDARD 17
)

# target_compile_definitions(sqlite3 PRIVATE -DSQLITE_ENABLE_COLUMN_METADATA)

set_source_files_properties(sqlite3.c sqlite3.h PROPERTIES LANGUAGE C)

set_target_properties(sqlite3 PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib

)
if(MSVC)
    set_target_properties(sqlite3 PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug,RelWithDebInfo>:Debug>")
endif()


set_target_properties(sqlite3 PROPERTIES LINKER_LANGUAGE C)
target_include_directories(sqlite3 INTERFACE ${CMAKE_CURRENT_LIST_DIR})