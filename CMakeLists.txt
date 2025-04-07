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
# build tests
option(PDMPMT_BUILD_TESTS "Build test programs" ON)

# check if generator is multi-config
get_property(PDMPMT_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(PDMPMT_MULTI_CONFIG)
    message(STATUS "Generator config: Multi")
else()
    message(STATUS "Generator config: Single")
endif()

# build debug by default; we set -DCMAKE_BUILD_TYPE=Release for release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
if(PDMPMT_MULTI_CONFIG)
    message(STATUS "Build config: Multi")
else()
    message(STATUS "Build config: ${CMAKE_BUILD_TYPE}")
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

# check for optional CUDA Toolkit and configure
include(ConfigCUDA)
# check compiler OpenMP support and add correct OpenMP compile options
include(ConfigOpenMP)

# Google Test for unit test runners
find_package(GTest 1.10)
if(GTest_FOUND)
    message(STATUS "Google Test version: ${GTest_VERSION}")
else()
    message(STATUS "Google Test version: None")
endif()

# find Python (note: may add Development component later)
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    message(STATUS "Python version: ${Python3_VERSION}")
else()
    message(STATUS "Python version: None")
endif()

# if we have Python 3, find Dask
if(Python3_FOUND)
    execute_process(
        COMMAND pip show dask
        RESULT_VARIABLE _show_dask_res
        OUTPUT_VARIABLE _show_dask_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # failed
    if(_show_dask_res)
        set(PDMPMT_DASK_FOUND FALSE)
    else()
        # Dask found
        set(PDMPMT_DASK_FOUND TRUE)
        # convert _show_dask_output to list and keep only version info
        string(REPLACE "\n" ";" _show_dask_output "${_show_dask_output}")
        list(FILTER _show_dask_output INCLUDE REGEX "Version:")
        # convert into version
        string(
            REGEX REPLACE "Version:[ ]+" ""
            PDMPMT_DASK_VERSION "${_show_dask_output}"
        )
    endif()
    unset(_show_dask_res)
    unset(_show_dask_output)
endif()
# Dask found?
if(PDMPMT_DASK_FOUND)
    message(STATUS "Dask version: ${PDMPMT_DASK_VERSION}")
else()
    message(STATUS "Dask version: None")
endif()

# set compiler-specific configuration options. this is done after finding all
# dependencies so there is no possible interference with upstream compile flags
include(ConfigCompile)

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

# also build third-party dependencies
add_subdirectory(third_party)
