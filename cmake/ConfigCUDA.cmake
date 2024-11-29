cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# minimum CUDA Toolkit version to use
set(PDMPMT_CUDA_TOOLKIT_VERSION 11.7)
find_package(CUDAToolkit ${PDMPMT_CUDA_TOOLKIT_VERSION})
# if found, configure, else just warn that no CUDA files will be compiled
if(CUDAToolkit_FOUND)
    # same C++ standard and requirement
    set(CMAKE_CUDA_STANDARD ${CMAKE_CXX_STANDARD})
    set(CMAKE_CUDA_STANDARD_REQUIRED ${CMAKE_CXX_STANDARD_REQUIRED})
    # not necessarily the same as CUDAToolkit_NVCC_EXECUTABLE, set manually
    if(NOT CMAKE_CUDA_COMPILER)
        set(CMAKE_CUDA_COMPILER ${CUDAToolkit_NVCC_EXECUTABLE})
    endif()
    message(
        STATUS
        "ConfigCUDA: Using CMAKE_CUDA_COMPILER ${CMAKE_CUDA_COMPILER}"
    )
    message(STATUS "CUDA version: ${CUDAToolkit_VERSION}")
    enable_language(CUDA)
else()
    message(STATUS "CUDA version: None")
endif()
