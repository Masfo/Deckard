
cmake_minimum_required (VERSION 3.28)

add_library(deckard)


set(DECKARD_SRCS
		${DIR}/Deckard.ixx
		${DIR}/debug.ixx
		${DIR}/assert.ixx
		${DIR}/File.ixx
		${DIR}/FileMonitor.ixx
		${DIR}/types.ixx
		${DIR}/win32.ixx
		#${DIR}/DTE.ixx
)

target_sources(${TARGET}
		PRIVATE
		FILE_SET modules 
		TYPE CXX_MODULES
		FILES ${DECKARD_SRCS}
)

