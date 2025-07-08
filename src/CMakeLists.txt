cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# build pdmpmt libraries
add_subdirectory(pdmpmt)

# thrust_demo: Thrust (NVIDIA CCCL) demo program
# TODO: see if we can use host C++ compiler only + use shared CUDA runtime
if(CUDAToolkit_FOUND)
    add_executable(thrust_demo thrust_demo.cu)
    target_link_libraries(thrust_demo PRIVATE pdmpmt++)
    add_test(NAME thrust_demo COMMAND thrust_demo)
else()
    message(STATUS "Skipping thrust_demo (requires CUDA Toolkit)")
endif()

# cuda_device_info: print info on available CUDA devices
if(CUDAToolkit_FOUND)
    add_executable(cuda_device_info cuda_device_info.cc)
    target_link_libraries(cuda_device_info PRIVATE CUDA::cudart pdmpmt++)
    # print standard output
    add_test(NAME cuda_device_info COMMAND cuda_device_info)
    # test command-line options
    add_test(NAME cuda_device_info_e COMMAND cuda_device_info -e)
    add_test(NAME cuda_device_info_extended COMMAND cuda_device_info --extended)
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
else()
    message(STATUS "Skipping cuda_device_info (requires CUDA Toolkit)")
endif()

# opencl_device_info: print OpenCL platform + device info
if(OpenCL_FOUND)
    add_executable(opencl_device_info opencl_device_info.cc)
    target_link_libraries(opencl_device_info PRIVATE OpenCL::OpenCL pdmpmt++)
    # plain run
    add_test(NAME opencl_device_info COMMAND opencl_device_info)
    # -h, --help
    add_test(NAME opencl_device_info_h COMMAND opencl_device_info -h)
    add_test(NAME opencl_device_info_help COMMAND opencl_device_info --help)
    set_tests_properties(
        opencl_device_info_h
        opencl_device_info_help PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage: opencl_device_info"
    )
else()
    message(STATUS "Skipping opencl_device_info (requires OpenCL)")
endif()

# curand_mcpi: compute pi using Monte Carlo using Thrust + cuRAND
# TODO: consider making this also a test in the Google Test runner
if(CUDAToolkit_FOUND)
    add_executable(curand_mcpi curand_mcpi.cu)
    target_link_libraries(curand_mcpi PRIVATE CUDA::curand pdmpmt++)
    add_test(NAME curand_mcpi COMMAND curand_mcpi)
else()
    message(STATUS "Skipping curand_mcpi (requires CUDA Toolkit)")
endif()
