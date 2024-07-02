cmake_minimum_required (VERSION 3.23)

project(zstd)
add_library(zstd)

 
set_target_properties(zstd PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib
)
 
if(NOT EXISTS  ${CMAKE_CURRENT_LIST_DIR}/zstd.zip)
   # file(DOWNLOAD https://github.com/facebook/zstd/archive/refs/heads/dev.zip  ${CMAKE_CURRENT_LIST_DIR}/zstd.zip)
    file(DOWNLOAD  https://github.com/facebook/zstd/releases/download/v1.5.6/zstd-v1.5.6-win64.zip ${CMAKE_CURRENT_LIST_DIR}/zstd-v156.zip)

    file(ARCHIVE_EXTRACT 
        INPUT ${CMAKE_CURRENT_LIST_DIR}/zstd-v156.zip 
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}
    )
endif()


if(MSVC)
    set_target_properties(zstd PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug,RelWithDebInfo>:Debug>")
endif()

 
target_include_directories(zstd INTERFACE ${CMAKE_CURRENT_LIST_DIR}/zstd-v1.5.6-win64/include)
target_link_libraries(zstd INTERFACE ${CMAKE_CURRENT_LIST_DIR}/zstd-v1.5.6-win64/static/libzstd_static.lib)

