 							   

set(MODULENAME Deckard)

add_library(${MODULENAME} STATIC)
set_property(TARGET ${MODULENAME} PROPERTY CXX_STANDARD          23)
set_property(TARGET ${MODULENAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${MODULENAME} PROPERTY CXX_EXTENSIONS        OFF)


target_sources(${MODULENAME} PRIVATE 
               FILE_SET cxx_modules 
               TYPE CXX_MODULES 
               FILES 

               module.ixx
)

											
