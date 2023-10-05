
cmake_minimum_required (VERSION 3.27)

add_library(Deckard)



target_sources(Deckard
		PRIVATE
		FILE_SET modules 
		TYPE CXX_MODULES
		FILES

		${CMAKE_CDeckard.ixx
		debug.ixx
		assert.ixx
		File.ixx
		FileMonitor.ixx
		types.ixx
		win32.ixx
		#DTE.ixx

)

