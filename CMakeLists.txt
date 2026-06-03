# NeoLED ESP-IDF Component
# Compatible with ESP-IDF 4.x and 5.x

idf_component_register(
    SRCS "neoled.cpp"
         "neoled_c.cpp"
    INCLUDE_DIRS "include"
    # 'driver' covers I2S + GPIO on both ESP-IDF 4.x and 5.x. freertos, log and
    # esp_rom (used for esp_rom_delay_us) are common components, automatically
    # available to every component, so they are not listed here.
    PRIV_REQUIRES driver
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
