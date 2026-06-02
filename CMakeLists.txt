# NeoLED ESP-IDF Component
# Compatible with ESP-IDF 4.x and 5.x

idf_component_register(
    SRCS "neoled.cpp"
         "neoled_c.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES driver freertos esp_rom esp_log
)

# The library targets C++11.
set_target_properties(${COMPONENT_LIB} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

# Extra warnings for development builds.
if(CONFIG_COMPILER_WARN_WRITE_STRINGS)
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wextra)
endif()
