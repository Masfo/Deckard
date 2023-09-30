
cmake_minimum_required (VERSION 3.27)

macro(add_deckard TARGET DIR)

	target_sources(${TARGET}
			PRIVATE
			FILE_SET modules 
			TYPE CXX_MODULES
			FILES
			${DIR}/deckard.ixx
			${DIR}/debug.ixx
			${DIR}/assert.ixx
			${DIR}/file.ixx
			${DIR}/types.ixx


			
)

endmacro()
