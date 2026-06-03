# Minimal ESP-IDF project that builds the NeoLED component, used by CI to
# verify the component compiles and links across ESP-IDF versions and targets.
cmake_minimum_required(VERSION 3.16)
# The NeoLED component is provided by components/neoled/, a thin wrapper that
# compiles the library sources from the repo root by absolute path. This keeps
# the component name deterministic ("neoled") regardless of the repo's checkout
# directory name/case.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(neoled_build_test)
