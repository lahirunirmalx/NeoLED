# NeoLED ESP-IDF Component CMakeLists.txt
# Compatible with ESP-IDF 4.x and 5.x

# Component source files
set(COMPONENT_SRCS 
    "neoled.cpp"
)

# Include directories
set(COMPONENT_ADD_INCLUDEDIRS 
    "include"
)

# Required ESP-IDF components
set(COMPONENT_PRIV_REQUIRES 
    driver 
    freertos
    esp_log
)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Register the component
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    PRIV_REQUIRES ${COMPONENT_PRIV_REQUIRES}
)

# Optional: Enable extra warnings for development
if(CONFIG_COMPILER_WARN_WRITE_STRINGS)
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra -Wpedantic)
endif()