cmake_minimum_required(VERSION 3.21)

project(
    pdmpmt
    VERSION 0.0.1
    DESCRIPTION "C/C++ experiments on mixed-hardware computing"
    HOMEPAGE_URL "https://github.com/phetdam/pdmpmt"
    LANGUAGES C CXX
)

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

# update module search path to find our CMake scripts
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH})

# set compiler-specific configuration options
include(ConfigCompile)
# check compiler OpenMP support and add correct OpenMP compile option
include(ConfigOpenMP)

# already checked for OpenMP on a per-compiler basis
find_package(GSL 2.7 REQUIRED)
find_package(GTest 1.10 REQUIRED)

# include extra GSL configuration
include(ConfigGSL)

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

# on Windows, export all DLL symbols
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_subdirectory(src)

# need to call this to enable test discovery!
enable_testing()
add_subdirectory(test)
