cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

if(MSVC)
    # check OpenMP support. MSVC OpenMP support lags behind GCC's by a lot
    find_package(OpenMP 2.0 COMPONENTS C CXX)
    if(NOT OpenMP_FOUND)
        message(
            WARNING "OpenMP 2.0 not supported by MSVC version ${MSVC_VERSION}"
        )
    endif()
else()
    # check OpenMP support, GCC 9.x and clang 3.9+ implement 4.5
    find_package(OpenMP 4.5 COMPONENTS C CXX)
    if(NOT OpenMP_FOUND)
        message(
            WARNING "OpenMP not supported by
  C compiler ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_VERSION}
C++ compiler ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}"
        )
    endif()
endif()
