cmake_minimum_required(VERSION 3.21)

if(MSVC)
    # check OpenMP support. MSVC OpenMP support lags behind GCC's by a lot
    find_package(OpenMP 2.0)
    if(OpenMP_FOUND)
        add_compile_options(
            $<$<COMPILE_LANGUAGE:C>:${OpenMP_C_FLAGS}>
            $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_CXX_FLAGS}>
        )
    else()
        message(
            WARNING "OpenMP 2.0 not supported by MSVC version ${MSVC_VERSION}"
        )
    endif()
# options are also accepted by clang
else()
    # check OpenMP support, GCC 9.x and clang 3.9+ implement 4.5
    find_package(OpenMP 4.5)
    if(OpenMP_FOUND)
        add_compile_options(
            $<$<COMPILE_LANGUAGE:C>:${OpenMP_C_FLAGS}>
            $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_CXX_FLAGS}>
        )
    else()
        message(
            WARNING "OpenMP not supported by
  C compiler ${CMAKE_C_COMPILER} ${CMAKE_C_COMPILER_VERSION}
C++ compiler ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}"
        )
    endif()
endif()
