# Thin wrapper that exposes the repo-root NeoLED library as a component named
# "neoled" for the CI build-test project. Sources are referenced by absolute
# path so the component name does not depend on the repo's checkout directory.
get_filename_component(NEOLED_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)

idf_component_register(
    SRCS "${NEOLED_ROOT}/neoled.cpp"
         "${NEOLED_ROOT}/neoled_c.cpp"
    INCLUDE_DIRS "${NEOLED_ROOT}/include"
    # 'driver' covers I2S + GPIO; freertos/log/esp_rom are common components.
    PRIV_REQUIRES driver
)
