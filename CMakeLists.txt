cmake_minimum_required(VERSION 3.21)

project(
    pdmpmt
    VERSION 0.1.0
    DESCRIPTION "C/C++ experiments on mixed-hardware computing"
    HOMEPAGE_URL "https://github.com/phetdam/pdmpmt"
    LANGUAGES C CXX
)

# TODO: consider only allowing building as shared
option(BUILD_SHARED_LIBS "Build libraries as shared instead of static" ON)
# this option is mostly important on Windows
# TODO: remove, we don't want to use GPL'd code
option(PDMPMT_GSL_SHARED "Use GNU GSL shared libraries" ON)
# build tests
option(PDMPMT_BUILD_TESTS "Build test programs" ON)

# build debug by default; we set -DCMAKE_BUILD_TYPE=Release for release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# C/C++ standards are all _reasonably_ recent
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# status messages
if(BUILD_SHARED_LIBS)
    message(STATUS "Build libraries: Shared")
else()
    message(STATUS "Build libraries: Static")
endif()
if(PDMPMT_BUILD_TESTS)
    message(STATUS "Build tests: Yes")
else()
    message(STATUS "Build tests: No")
endif()

# make all artifacts output in top-level. multi-config generators like Visual
# Studio will have per-config subdirectories (this is intentional)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# update module search path to find our CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# set compiler-specific configuration options
include(ConfigCompile)
# check for optional CUDA Toolkit and configure
include(ConfigCUDA)
# check compiler OpenMP support and add correct OpenMP compile options
include(ConfigOpenMP)
# check for GSL and include extra GSL configuration
# include(ConfigGSL)

# TODO: maybe make Google Test not required?
find_package(GTest 1.10 REQUIRED)
# find Python (note: may add Development component later)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    message(STATUS "Python version: ${Python3_VERSION}")
else()
    message(STATUS "Python version: None")
endif()

# base include for this repo (since "include" is reserved)
set(PDMPMT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# must add include directories if they are installed in non-standard locations
# that were specified by BOOST_ROOT, GTEST_ROOT, etc.
include_directories(${PDMPMT_INCLUDE} SYSTEM ${GTEST_INCLUDE_DIRS})

include(CTest)

# ensure BUILD_TESTING matches PDMPMT_BUILD_TESTS
if(NOT BUILD_TESTING AND PDMPMT_BUILD_TESTS)
    set(BUILD_TESTING ON)
endif()

add_subdirectory(src)

if(PDMPMT_BUILD_TESTS)
    add_subdirectory(test)
endif()
