cmake_minimum_required(VERSION 3.20)

project(
    pdmpmt
    VERSION 0.0.1
    DESCRIPTION "C/C++ experiments on mixed-hardware computing"
    HOMEPAGE_URL "https://github.com/phetdam/pdmpmt"
    LANGUAGES C CXX
)

# build debug by default; we set -DCMAKE_BUILD_TYPE=Release for release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(MSVC)
    # CMake adds /O2 by default for release version
    if(NOT CMAKE_BUILD_TYPE STREQUAL Release)
        add_compile_options(/Od /DEBUG)
    endif()
# options are also accepted by clang
else()
    add_compile_options(-Wall)
    if(CMAKE_BUILD_TYPE STREQUAL Release)
        add_compile_options(-O3)
    else()
        add_compile_options(-O0 -ggdb)
    endif()
endif()

# base include for this repo (since "include" is reserved)
set(PDMPMT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(Boost 1.71 REQUIRED)
find_package(GTest 1.10 REQUIRED)

# must add include directories if they are installed in non-standard locations
# that were specified by BOOST_ROOT, GTEST_ROOT, etc.
include_directories(
    ${PDMPMT_INCLUDE}
    SYSTEM ${Boost_INCLUDE_DIRS}
    SYSTEM ${GTEST_INCLUDE_DIRS}
)

# need to call this to enable test discovery!
enable_testing()
add_subdirectory(test)
