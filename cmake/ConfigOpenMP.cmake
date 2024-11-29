cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# FIXME: reduce some of the repetition here

if(MSVC)
    # check OpenMP support. MSVC OpenMP support lags behind GCC's by a lot
    find_package(OpenMP 2.0 COMPONENTS C CXX)
    if(OpenMP_FOUND)
        message(STATUS "OpenMP C version: ${OpenMP_C_VERSION}")
        message(STATUS "OpenMP C++ version: ${OpenMP_CXX_VERSION}")
    else()
        message(
            WARNING "OpenMP 2.0 not supported by MSVC version ${MSVC_VERSION}"
        )
    endif()
else()
    # check OpenMP support, GCC 9.x and clang 3.9+ implement 4.5
    find_package(OpenMP 4.5 COMPONENTS C CXX)
    if(OpenMP_FOUND)
        message(STATUS "OpenMP C version: ${OpenMP_C_VERSION}")
        message(STATUS "OpenMP C++ version: ${OpenMP_CXX_VERSION}")
    else()
        message(
            WARNING "OpenMP not supported by
  C compiler ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_VERSION}
C++ compiler ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}"
        )
        message(STATUS "OpenMP version: None")
    endif()
endif()
