cmake_minimum_required (VERSION 3.20)

set(ZSTD_NAME zstd-v1.5.6-win64)
 
if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_NAME}.zip)

    file(DOWNLOAD  https://github.com/facebook/zstd/releases/download/v1.5.6/${ZSTD_NAME}.zip ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_NAME}.zip)

    # Extract to temp
    file(ARCHIVE_EXTRACT 
        INPUT ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_NAME}.zip
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}
    )

    file(COPY ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_NAME}/static/libzstd_static.lib DESTINATION ${CMAKE_CURRENT_LIST_DIR})
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_NAME}/include/zstd.h DESTINATION ${CMAKE_CURRENT_LIST_DIR})
    
    # Remove temp
    file(REMOVE_RECURSE   ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_NAME})

endif()

target_include_directories(deckard PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_directories(deckard PUBLIC ${CMAKE_CURRENT_LIST_DIR})



