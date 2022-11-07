cmake_minimum_required(VERSION 3.20)

project(
    pdmpmt
    VERSION 0.0.1.dev0
    DESCRIPTION "C/C++ experiments on mixed-hardware computing"
    HOMEPAGE_URL "https://github.com/phetdam/pdmpmt"
    LANGUAGES C CXX
)

if(MSVC)
    # CMake adds /O2 by default for release version
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /DEBUG)
    endif()
# options are also accepted by clang
else()
    add_compile_options(-Wall)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-O0 -ggdb)
    else()
        add_compile_options(-O3)
    endif()
endif()

add_subdirectory(src)
add_subdirectory(test)
