cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# libprand. no install rules since this is a private dependency
add_library(prand STATIC mrg32k3a.c mt19937.c mt19937_poly.c prand.c)
# headers are in src/header
target_include_directories(prand PUBLIC header)
# if requested, build with position-independent code even for static builds
if(BUILD_SHARED_LIBS)
    set_target_properties(prand PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
# use "d" suffix on Windows for debug config to note use of debug C runtime
if(MSVC)
    set_target_properties(prand PROPERTIES OUTPUT_NAME_DEBUG prandd)
endif()
