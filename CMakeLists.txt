
cmake_minimum_required (VERSION 3.28)

macro(add_deckard TARGET DIR)

	target_sources(${TARGET}
			PRIVATE
			FILE_SET modules 
			TYPE CXX_MODULES
			FILES
			${DIR}/Deckard.ixx
			${DIR}/debug.ixx
			${DIR}/assert.ixx
			${DIR}/File.ixx
			${DIR}/FileMonitor.ixx
			${DIR}/types.ixx
			${DIR}/win32.ixx
			#${DIR}/DTE.ixx




			
)

endmacro()
