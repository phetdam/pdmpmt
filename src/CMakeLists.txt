cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# build pdmpmt libraries
add_subdirectory(pdmpmt)

if(CUDAToolkit_FOUND)
    # thrust_demo: Thrust (NVIDIA CCCL) demo program
    # TODO: see if we can use host C++ compiler only + use shared CUDA runtime
    add_executable(thrust_demo thrust_demo.cu)
    add_test(NAME thrust_demo COMMAND thrust_demo)
    # cuda_device_info: print info on available CUDA devices
    add_executable(cuda_device_info cuda_device_info.cc)
    target_link_libraries(cuda_device_info PRIVATE CUDA::cudart)
    add_test(NAME cuda_device_info COMMAND cuda_device_info)
    # test command-line options
    add_test(NAME cuda_device_info_h COMMAND cuda_device_info -h)
    add_test(NAME cuda_device_info_help COMMAND cuda_device_info --help)
    set_tests_properties(
        cuda_device_info_h cuda_device_info_help
        PROPERTIES PASS_REGULAR_EXPRESSION "Usage:"
    )
    add_test(NAME cuda_device_info_badopt COMMAND cuda_device_info --illegal-opt)
    set_tests_properties(
        cuda_device_info_badopt PROPERTIES
        PASS_REGULAR_EXPRESSION "Unknown argument"
    )
endif()
