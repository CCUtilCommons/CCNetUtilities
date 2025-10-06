cmake_minimum_required(VERSION 3.25)

# C And C++ Settings for the Memory Settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(pretty_banner)
include(easy_build)
project(CCNetworkUtilities VERSION 0.1.0 LANGUAGES C CXX)

print_banner("Configuring the pre-configs...")
add_subdirectory(config)
print_banner("Configuring the pre-configs done!")

print_banner("Configuring the library, so entering the library folder...")
add_subdirectory(platform)
add_subdirectory(library)
print_banner("Configuring the library done")

print_banner("Configuring the test and example...")
add_subdirectory(example)
add_subdirectory(test)
print_banner("Configuring the test and example done")
