cmake_minimum_required(VERSION 3.16)
project(haystack VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_CLI "Build CLI application" ON)
#option(BUILD_WEB_BACKEND "Build web backend" ON)
#option(BUILD_TESTS "Build tests" ON)

# add third-party libaries
add_subdirectory(lib)

# add core search engine
add_subdirectory(core)

# built based on passed option
if(BUILD_CLI)
    add_subdirectory(cli)
endif()
