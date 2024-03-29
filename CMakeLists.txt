cmake_minimum_required(VERSION 3.21)

project(
    pdmpmt
    VERSION 0.0.1
    DESCRIPTION "C/C++ experiments on mixed-hardware computing"
    HOMEPAGE_URL "https://github.com/phetdam/pdmpmt"
    LANGUAGES C CXX
)

# TODO: consider only allowing building as shared
option(BUILD_SHARED_LIBS "Build libraries as shared instead of static" ON)
# this option is mostly important on Windows
option(PDMPMT_GSL_SHARED "Use GNU GSL shared libraries" ON)

# build debug by default; we set -DCMAKE_BUILD_TYPE=Release for release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# C/C++ standards are all _reasonably_ recent
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# eliminate per-configuration subdirs from being appended to build artifact
# output dirs for multi-config generators, e.g. Visual Studio
set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}$<$<CONFIG:Debug>:>
)
set(
    CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}$<$<CONFIG:Debug>:>
)
set(
    CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}$<$<CONFIG:Debug>:>
)

# update module search path to find our CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH})

# set compiler-specific configuration options
include(ConfigCompile)
# check for optional CUDA Toolkit and configure
include(ConfigCUDA)
# check compiler OpenMP support and add correct OpenMP compile options
include(ConfigOpenMP)
# check for GSL and include extra GSL configuration
include(ConfigGSL)

find_package(GTest 1.10 REQUIRED)
find_package(CUDAToolkit 11.7)

# base include for this repo (since "include" is reserved)
set(PDMPMT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# must add include directories if they are installed in non-standard locations
# that were specified by BOOST_ROOT, GTEST_ROOT, etc.
include_directories(
    ${PDMPMT_INCLUDE}
    SYSTEM ${Boost_INCLUDE_DIRS}
    SYSTEM ${GSL_INCLUDE_DIRS}
    SYSTEM ${GTEST_INCLUDE_DIRS}
)

add_subdirectory(src)

# need to call this to enable test discovery!
enable_testing()
add_subdirectory(test)
