cmake_minimum_required (VERSION 3.20)

set(ZSTD_ZIP_URL https://github.com/facebook/zstd/releases/download/v1.5.6/zstd-v1.5.6-win64.zip)

get_filename_component(ZSTD_ZIP_NAME ${ZSTD_ZIP_URL} NAME)
get_filename_component(ZSTD_ZIP_NAME_NOEXT ${ZSTD_ZIP_URL} NAME_WLE)

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME})

    file(DOWNLOAD ${ZSTD_ZIP_URL} ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME})

    # Extract to temp
    file(ARCHIVE_EXTRACT 
        INPUT ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME}
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}
    )

    file(COPY ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME_NOEXT}/static/libzstd_static.lib DESTINATION ${CMAKE_CURRENT_LIST_DIR})
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME_NOEXT}/include/zstd.h DESTINATION ${CMAKE_CURRENT_LIST_DIR})
    
    # Remove temp
    file(REMOVE_RECURSE   ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME_NOEXT})
endif()

target_include_directories(deckard PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_directories(deckard PUBLIC ${CMAKE_CURRENT_LIST_DIR})



