cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# build pdmpmt libraries
add_subdirectory(pdmpmt)

# CUDA Thrust toy program
if(CUDAToolkit_FOUND)
    add_executable(thrust_demo thrust_demo.cu)
    add_test(NAME thrust_demo COMMAND thrust_demo)
endif()
