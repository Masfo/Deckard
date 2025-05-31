cmake_minimum_required (VERSION 3.23)

set(MONOCYPHER_ZIP_URL https://github.com/LoupVaillant/Monocypher/archive/refs/tags/4.0.2.zip)
set(MONOCYPHER_ZIP_NAME_NO_EXT Monocypher-4.0.2)

get_filename_component(MONOCYPHER_ZIP_NAME ${MONOCYPHER_ZIP_URL} NAME)


 
if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/${MONOCYPHER_ZIP_NAME})
 
	 file(DOWNLOAD ${MONOCYPHER_ZIP_URL} ${CMAKE_CURRENT_LIST_DIR}/${MONOCYPHER_ZIP_NAME})
 
	 # Extract to temp
	 file(ARCHIVE_EXTRACT 
		 INPUT ${CMAKE_CURRENT_LIST_DIR}/${MONOCYPHER_ZIP_NAME}
		 DESTINATION ${CMAKE_CURRENT_LIST_DIR}
	 )

	 # Files to copy
	 set(MONOCYPHER_FILES
	   monocypher.c
	   monocypher.h

	   optional/monocypher-ed25519.c
	   optional/monocypher-ed25519.h

	 )


	set(CMAKE_DISABLE_SOURCE_CHANGES OFF)

	# Copy the files, create folder 
	foreach(f IN LISTS MONOCYPHER_FILES)
		message(STATUS "file: ${f}")

	   get_filename_component(MONOCYPHER_DIR_NAME ${f} DIRECTORY)
		message(STATUS "filename comp: ${MONOCYPHER_DIR_NAME}")

	   file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${MONOCYPHER_DIR_NAME})
	   file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/${MONOCYPHER_ZIP_NAME_NO_EXT}/src/${f} ${CMAKE_CURRENT_LIST_DIR}/${f})
	endforeach()

	set(CMAKE_DISABLE_SOURCE_CHANGES ON)
	message(STATUS "Monocypher files copied to ${CMAKE_CURRENT_LIST_DIR}/${MONOCYPHER_ZIP_NAME_NO_EXT}")


	# Remove temp
	file(REMOVE_RECURSE ${CMAKE_CURRENT_LIST_DIR}/${MONOCYPHER_ZIP_NAME_NO_EXT})

endif()

project(monocypher)
add_library(monocypher STATIC
	monocypher.c
	monocypher.h

	optional/monocypher-ed25519.c
	optional/monocypher-ed25519.h
)

if(MSVC)
	set_target_properties(monocypher PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug,RelWithDebInfo>:Debug>")
endif()

set_target_properties(monocypher PROPERTIES LINKER_LANGUAGE C)

target_include_directories(monocypher PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(monocypher PUBLIC ${CMAKE_CURRENT_LIST_DIR}/optional)




