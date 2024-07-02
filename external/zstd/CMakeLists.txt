cmake_minimum_required (VERSION 3.20)

 
if(NOT EXISTS  ${CMAKE_CURRENT_LIST_DIR}/zstd.zip)
   # file(DOWNLOAD https://github.com/facebook/zstd/archive/refs/heads/dev.zip  ${CMAKE_CURRENT_LIST_DIR}/zstd.zip)
    file(DOWNLOAD  https://github.com/facebook/zstd/releases/download/v1.5.6/zstd-v1.5.6-win64.zip ${CMAKE_CURRENT_LIST_DIR}/zstd-v156.zip)

    file(ARCHIVE_EXTRACT 
        INPUT ${CMAKE_CURRENT_LIST_DIR}/zstd-v156.zip 
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}
    )
endif()



# Copy header and lib to here to can just have ${CMAKE_CURRENT_LIST_DIR}
 target_link_directories(deckard PUBLIC ${CMAKE_CURRENT_LIST_DIR}/zstd-v1.5.6-win64/static)
target_include_directories(deckard PUBLIC ${CMAKE_CURRENT_LIST_DIR}/zstd-v1.5.6-win64/include)


