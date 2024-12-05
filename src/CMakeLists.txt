cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# build pdmpmt libraries
add_subdirectory(pdmpmt)

# CUDA Thrust toy program
if(CUDAToolkit_FOUND)
    # TODO: see if we can use host C++ compiler only + use shared CUDA runtime
    add_executable(thrust_demo thrust_demo.cu)
    add_test(NAME thrust_demo COMMAND thrust_demo)
endif()

# add_executable(cuda_device_info cuda_device_info.cc)
# target_link_libraries(cuda_device_info PRIVATE CUDA::cudart)
