# Set the component to use C++ instead of C
set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "neoled.cpp")

# Specify C++ as the language and set C++11 standard
set(COMPONENT_PRIV_REQUIRES driver freertos)
set(CMAKE_CXX_STANDARD 11)

idf_component_register(SRCS ${COMPONENT_SRCS} INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS})

