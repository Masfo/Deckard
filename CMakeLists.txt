cmake_minimum_required (VERSION 4.3)

# set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
# if ( NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio.*" )
#   set(CMAKE_CXX_MODULE_STD 1)
# endif ()

project(deckard LANGUAGES CXX ASM_MASM)


add_library(deckard STATIC)

option(DECKARD_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(DECKARD_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(DECKARD_BUILD_TOOLS "Build tools" ${PROJECT_IS_TOP_LEVEL})
option(DECKARD_RUN_BUILDINC "Buildinc tool" ${PROJECT_IS_TOP_LEVEL})

#find_program(SCCACHE_PROGRAM sccache)

# if(SCCACHE_PROGRAM)
# 	message(STATUS "sccache found: ${SCCACHE_PROGRAM}")
# 	set(CMAKE_C_COMPILER_LAUNCHER   "${SCCACHE_PROGRAM}" CACHE STRING "" FORCE)
# 	set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}" CACHE STRING "" FORCE)
# else()
# 	message(STATUS "sccache not found — building without compiler cache")
# endif()





set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set_target_properties(deckard PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
	CXX_SCAN_FOR_MODULES ON
	RUNTIME_OUTPUT_DIRECTORY                ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_DIRECTORY_DEBUG          ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_DIRECTORY_RELEASE        ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin
	ARCHIVE_OUTPUT_DIRECTORY                ${CMAKE_SOURCE_DIR}/lib
	LIBRARY_OUTPUT_DIRECTORY_DEBUG          ${CMAKE_SOURCE_DIR}/lib
	LIBRARY_OUTPUT_DIRECTORY_RELEASE        ${CMAKE_SOURCE_DIR}/lib
	PDB_NAME deckard
	PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/pdb"
)



#set_target_properties(deckard PROPERTIES DEBUG_POSTFIX "d")


if(MSVC)
	
	if (POLICY CMP0141)
		cmake_policy(SET CMP0141 NEW)
		set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
	endif()


	target_compile_definitions(deckard PRIVATE 
		UNICODE 
		_UNICODE 
		NOMINMAX 
		WIN32_LEAN_AND_MEAN 
		WIN32_EXTRA_LEAN
	)
	
	# Global compile options for MSVC
	target_compile_options(deckard PRIVATE 
		$<$<COMPILE_LANGUAGE:CXX>:
			/nologo
			/Zc:preprocessor 
			/std:c++latest 
			/Zc:__cplusplus /Zc:__STDC__ 
			/utf-8
			/W4
			/EHsc
			/FS
			/diagnostics:caret
			/MP
			>
	)



	#target_compile_options(deckard PRIVATE /arch:AVX2)

	set_target_properties(deckard PROPERTIES POSITION_INDEPENDENT_CODE ON)

	#set_target_properties(deckard PROPERTIES MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

	target_compile_options(deckard PRIVATE
		$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:DEBUG>>:
			/JMC
			/Od
			/GS
			/Zi
			/FS
		>

		$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:
			/O2
			/Ob2
			/Oi
			/GL
			/Gy
			/GF
			/GT
		>
	)


	 # Debug
	if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	
		set_target_properties(deckard PROPERTIES
			MSVC_DEBUG_INFORMATION_FORMAT "ProgramDatabase"
			MSVC_RUNTIME_LIBRARY "MultiThreadedDebug"
		)
		target_compile_definitions(deckard PRIVATE DEBUG)
		target_link_options(deckard PRIVATE /INCREMENTAL /ILK:${CMAKE_SOURCE_DIR}/bin/pdb/deckard.ilk)
	endif()
	
	# Release
	if (${CMAKE_BUILD_TYPE} MATCHES "Release")
	
		set_target_properties(deckard PROPERTIES
			MSVC_RUNTIME_LIBRARY "MultiThreaded"
			INTERPROCEDURAL_OPTIMIZATION TRUE
		)

		target_compile_definitions(deckard PRIVATE NDEBUG)
		
		target_link_options(deckard PRIVATE /MERGE:.pdata=.text /MERGE:.rdata=.text)
		option(DECKARD_BUILD_TESTS "Build tests" FALSE)

	endif()


	# STD hack
	set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
	configure_file(	
			$ENV{VCToolsInstallDir}modules/std.ixx
			${CMAKE_CURRENT_LIST_DIR}/std.ixx
			COPYONLY
	)
	set(CMAKE_DISABLE_SOURCE_CHANGES ON)

endif()

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(deckard PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(deckard PRIVATE Vulkan::Vulkan)

# find_program(GLSLC_EXECUTABLE glslc HINTS $ENV{VULKAN_SDK}/Bin)
# if(NOT GLSLC_EXECUTABLE)
#     message(FATAL_ERROR "glslc not found! Please install the Vulkan SDK or set GLSLC_EXECUTABLE manually.")
# endif()


set(DECKARD_SRCS
		deckard.ixx
		$<$<CONFIG:RelWithDebInfo,Release>:buildnumber.ixx>

		$<$<CONFIG:Debug>:debug/debug.ixx>
		$<$<CONFIG:Debug>:debug/assert.ixx>
		$<$<CONFIG:Debug>:debug/log.ixx>

		$<$<CONFIG:RelWithDebInfo,Release>:debug/debug_null.ixx>
		$<$<CONFIG:RelWithDebInfo,Release>:debug/assert_null.ixx>
		$<$<CONFIG:RelWithDebInfo,Release>:debug/log_null.ixx>


		$<$<PLATFORM_ID:Windows>:debug/console.ixx>

		# File
		file/file.ixx
		file/fileMonitor.ixx
		

		# Config
		config/config.ixx

		# Math
		math/math.ixx
		math/ease_functions.ixx
		math/math_utility.ixx
		math/primitives.ixx
		math/curves.ixx
		
		math/matrix.ixx

		math/vec4_sse.ixx

		math/generic_vec.ixx
		math/generic_vec2.ixx
		math/generic_vec3.ixx
		math/generic_vec4.ixx

		math/quaternion.ixx

		# Net
		net/net.ixx
		net/types.ixx
		net/protocol.ixx
		net/auth.ixx
		net/socket.ixx
		
		net/server.ixx
		net/client.ixx
		net/ip.ixx
	   
		# threadpool
		threadpool/threadpool.ixx

		# memorypool
		memory/memorypool.ixx


		# Utils
		utils/arrays.ixx
		utils/base_n.ixx
		utils/commandline.ixx
		utils/cpuid.ixx
		utils/function_ref.ixx
		utils/grid.ixx
		utils/hash.ixx
		utils/helpers.ixx
		utils/hmac.ixx
		utils/random.ixx
		utils/ringbuffer.ixx
		utils/scope_exit.ixx
		utils/serializer.ixx
		utils/sha.ixx
		utils/smallbufferobject.ixx
		utils/stringhelper.ixx
		utils/threadutils.ixx
		utils/timers.ixx
		utils/uuid.ixx
		utils/logger.ixx
		utils/colors.ixx
		utils/circular_buffer.ixx
		utils/lru.ixx
		utils/bytepool.ixx
		utils/stringpool.ixx
		


		# Unicode
		unicode/utf8.ixx
		unicode/utf8_ascii.ixx
		unicode/utf8_basic_characters.ixx
		unicode/utf8_codepoints.ixx
		unicode/utf8_decode.ixx
		unicode/utf8_xid.ixx
		
		
		unicode/utf8_tables.ixx
		
		unicode/utf8_normalization.ixx
		unicode/utf8string.ixx
		unicode/utf8view.ixx
		unicode/utf8_scanner.ixx
		

		# Graph
		graphs/avltree.ixx
		graphs/binarytree.ixx
		
		graphs/graph.ixx
		



		

		# Scripting
		scripting/lexer/lexer.ixx
		scripting/parser/parser.ixx

		# App
		app/app.ixx
		app/controller.ixx
		app/inputs.ixx

		# Vulkan
		app/vulkan.ixx

		app/vulkan/command_buffer.ixx
		app/vulkan/core.ixx
		app/vulkan/debug.ixx
		app/vulkan/device.ixx
		app/vulkan/fence.ixx
		app/vulkan/helpers.ixx
		app/vulkan/images.ixx
		app/vulkan/instance.ixx
		app/vulkan/semaphore.ixx
		app/vulkan/shader.ixx
		app/vulkan/surface.ixx
		app/vulkan/swapchain.ixx
		app/vulkan/texture.ixx
		app/vulkan/shaders.ixx
		app/vulkan/buffer.ixx
		app/vulkan/pipeline.ixx


		# Types
		types/as.ixx
		types/bigint.ixx
		types/enum_flags.ixx
		types/helpertypes.ixx
		types/types.ixx
		types/int128.ixx
		types/allocator.ixx
		types/image.ixx

		# External helpers
		compress/zstd.ixx
		monocypher/monocypher.ixx

		archive/archive.ixx
		archive/db.ixx


		# win32
		$<$<PLATFORM_ID:Windows>:platform/platform_win32.ixx>
		$<$<PLATFORM_ID:Windows>:platform/win32.ixx>
		# $<$<PLATFORM_ID:Windows>:platform/DTE.ixx>


		std.ixx
)

# $<$<PLATFORM_ID:Linux,Windows>:OpenGL::GL>
# $<$      <PLATFORM_ID:Windows> :win32/win32.ixx>
# $<$<NOT:$<PLATFORM_ID:Windows>>:file.cpp>


target_sources(deckard PRIVATE
		utils/cpuid.asm
)




target_sources(deckard
	PUBLIC
	FILE_SET CXX_MODULES 
	FILES ${DECKARD_SRCS}
)

#target_include_directories(deckard PUBLIC ${CMAKE_CURRENT_LIST_DIR}/external/gl)



set(THIRD_PARTY_LIBS
	sqlite3
	ZSTD
	monocypher
	opengl32.lib
	ws2_32
	xinput
	Comctl32.lib
	#cpuid
   # libcpmtd.lib
   # libcmtd.lib
   # libconcrtd.lib
   # libvcruntimed.lib
   #libucrtd.lib
   #libvcruntimed.lib	
)

target_link_libraries(deckard PUBLIC ${THIRD_PARTY_LIBS})

#
target_compile_options(deckard PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:
		/wd5050  # Possible incompatible environment while importing module
		/wd5205  # delete of an abstract class with a non-virtual destructor
		/wd4324  # structure was padded due to alignment specifier

		#/wd5039  # pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc.
		#/wd5262  # implicit fall-through occurs here
		#/wd4710  # function not inlined
		#/wd4711  # function selected for automatic inline expansion
		#/wd5045  # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
		#/wd4820  # bytes padding added after data member
		#/wd4626  # assignment operator was implicitly defined as deleted
		#/wd5027  # move assignment operator was implicitly defined as deleted
		#/wd5026  # move constructor was implicitly defined as deleted
		#/wd4061  # switch of enum is not explicitly handled by a case label
		#/wd4355  # 'this': used in base member initializer list
		#/wd4625  # copy constructor was implicitly defined as deleted
		#/wd5220  # a non-static data member with a volatile qualified type
		#/wd5204  # class has virtual functions, but its trivial destructor is not virtual
		#/wd4686  # possible change in behavior, change in UDT return calling convention
		#/wd5246  # the initialization of a subobject should be wrapped in braces
		#/wd4273  # inconsistent dll linkage
	>
)

###################################


if(DECKARD_RUN_BUILDINC)
	find_program(BUILDINC_TOOL "buildinc")
	if(BUILDINC_TOOL)
	add_custom_command(TARGET deckard
					   PRE_BUILD
					   COMMAND ${BUILDINC_TOOL} ${CMAKE_CURRENT_SOURCE_DIR}/buildnumber.ixx deckard_build -q -m 
					   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
					   COMMENT "Increase build number"
	)
	endif()
endif()


if(DECKARD_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()


if(DECKARD_BUILD_EXAMPLES)
	#add_subdirectory(example/empty)
	add_subdirectory(example/window)
	add_subdirectory(example/threader)
endif()

if(DECKARD_BUILD_TOOLS)
	add_subdirectory(tools/dbc)
	add_subdirectory(tools/utftables)
	add_subdirectory(tools/normalization)
endif()



add_subdirectory(external/sqlite3)
add_subdirectory(external/zstd)
add_subdirectory(external/monocypher)
