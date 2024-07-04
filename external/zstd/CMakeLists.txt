cmake_minimum_required (VERSION 3.23)

set(ZSTD_ZIP_URL https://github.com/facebook/zstd/archive/refs/tags/v1.5.6.zip)
get_filename_component(ZSTD_ZIP_NAME ${ZSTD_ZIP_URL} NAME)
set(ZSTD_ZIP_NAME_NOEXT zstd-1.5.6)

 
if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME})
 
     file(DOWNLOAD ${ZSTD_ZIP_URL} ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME})
 
     # Extract to temp
     file(ARCHIVE_EXTRACT 
         INPUT ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME}
         DESTINATION ${CMAKE_CURRENT_LIST_DIR}
     )

     # Files to copy
     set(ZSTD_FILES
        common/allocations.h
        common/bits.h
        common/bitstream.h
        common/compiler.h
        common/cpu.h
        common/debug.c
        common/debug.h
        common/entropy_common.c
        common/error_private.c
        common/error_private.h
        common/fse.h
        common/fse_decompress.c
        common/huf.h
        common/mem.h
        common/pool.c
        common/pool.h
        common/portability_macros.h
        common/threading.c
        common/threading.h
        common/xxhash.c
        common/xxhash.h
        common/zstd_common.c
        common/zstd_deps.h
        common/zstd_internal.h
        common/zstd_trace.h

        compress/clevels.h
        compress/fse_compress.c
        compress/hist.c
        compress/hist.h
        compress/huf_compress.c
        compress/zstd_compress.c
        compress/zstd_compress_internal.h
        compress/zstd_compress_literals.c
        compress/zstd_compress_literals.h
        compress/zstd_compress_sequences.c
        compress/zstd_compress_sequences.h
        compress/zstd_compress_superblock.c
        compress/zstd_compress_superblock.h
        compress/zstd_cwksp.h
        compress/zstd_double_fast.c
        compress/zstd_double_fast.h
        compress/zstd_fast.c
        compress/zstd_fast.h
        compress/zstd_lazy.c
        compress/zstd_lazy.h
        compress/zstd_ldm.c
        compress/zstd_ldm.h
        compress/zstd_ldm_geartab.h
        compress/zstd_opt.c
        compress/zstd_opt.h
        compress/zstdmt_compress.c
        compress/zstdmt_compress.h

        decompress/huf_decompress.c
        decompress/zstd_ddict.c
        decompress/zstd_ddict.h
        decompress/zstd_decompress.c
        decompress/zstd_decompress_block.c
        decompress/zstd_decompress_block.h
        decompress/zstd_decompress_internal.h

        zstd.h
        zdict.h
        zstd_errors.h
     )


    set(CMAKE_DISABLE_SOURCE_CHANGES OFF)

    # Copy the files, create folder 
    foreach(f IN LISTS ZSTD_FILES)
       get_filename_component(ZSTD_DIR_NAME ${f} DIRECTORY)
       file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_DIR_NAME})
       file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME_NOEXT}/lib/${f} ${CMAKE_CURRENT_LIST_DIR}/${f} ONLY_IF_DIFFERENT)
    endforeach()

    set(CMAKE_DISABLE_SOURCE_CHANGES ON)

    # Remove temp
    file(REMOVE_RECURSE ${CMAKE_CURRENT_LIST_DIR}/${ZSTD_ZIP_NAME_NOEXT})

endif()

project(ZSTD)
add_library(ZSTD STATIC
        common/debug.c
        common/entropy_common.c
        common/error_private.c
        common/fse_decompress.c
        common/pool.c
        common/threading.c
        common/xxhash.c
        common/zstd_common.c

        compress/fse_compress.c
        compress/hist.c
        compress/huf_compress.c
        compress/zstd_compress.c
        compress/zstd_compress_literals.c
        compress/zstd_compress_sequences.c
        compress/zstd_compress_superblock.c
        compress/zstd_double_fast.c
        compress/zstd_fast.c
        compress/zstd_lazy.c
        compress/zstd_ldm.c
        compress/zstd_opt.c
        compress/zstdmt_compress.c

        decompress/huf_decompress.c
        decompress/zstd_ddict.c
        decompress/zstd_decompress.c
        decompress/zstd_decompress_block.c
)

if(MSVC)
    set_target_properties(ZSTD PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug,RelWithDebInfo>:Debug>")
endif()

set_target_properties(ZSTD PROPERTIES LINKER_LANGUAGE C)

target_include_directories(ZSTD PUBLIC ${CMAKE_CURRENT_LIST_DIR})



