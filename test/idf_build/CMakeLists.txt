# Minimal ESP-IDF project that builds the NeoLED component, used by CI to
# verify the component compiles and links across ESP-IDF versions and targets.
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(neoled_build_test)
