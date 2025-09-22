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

# cuda_info: print info on available CUDA devices
if(CUDAToolkit_FOUND)
    add_executable(cuda_info cuda_info.cc)
    target_link_libraries(cuda_info PRIVATE CUDA::cudart pdmpmt++)
    # print standard output
    add_test(NAME cuda_info COMMAND cuda_info)
    # test command-line options
    add_test(NAME cuda_info_e COMMAND cuda_info -e)
    add_test(NAME cuda_info_extended COMMAND cuda_info --extended)
    add_test(NAME cuda_info_h COMMAND cuda_info -h)
    add_test(NAME cuda_info_help COMMAND cuda_info --help)
    set_tests_properties(
        cuda_info_h cuda_info_help
        PROPERTIES PASS_REGULAR_EXPRESSION "Usage:"
    )
    add_test(NAME cuda_info_badopt COMMAND cuda_info --illegal-opt)
    set_tests_properties(
        cuda_info_badopt PROPERTIES
        PASS_REGULAR_EXPRESSION "Unknown argument"
    )
else()
    message(STATUS "Skipping cuda_info (requires CUDA Toolkit)")
endif()

# opencl_info: print OpenCL platform + device info
if(OpenCL_FOUND)
    add_executable(opencl_info opencl_info.cc)
    target_link_libraries(
        opencl_info PRIVATE
        OpenCL::OpenCL pdmpmt++ pdmpmt_opencl_target_version
    )
    # plain run
    add_test(NAME opencl_info COMMAND opencl_info)
    # -h, --help
    add_test(NAME opencl_info_h COMMAND opencl_info -h)
    add_test(NAME opencl_info_help COMMAND opencl_info --help)
    set_tests_properties(
        opencl_info_h
        opencl_info_help PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:"
    )
    # --no-extensions
    add_test(NAME opencl_info_no_ext COMMAND opencl_info --no-ext)
    set_tests_properties(
        opencl_info_no_ext PROPERTIES
        FAIL_REGULAR_EXPRESSION "Extensions:"
    )
    # invalid argument
    add_test(NAME opencl_info_badopt COMMAND opencl_info --badopt)
    set_tests_properties(
        opencl_info_badopt PROPERTIES
        PASS_REGULAR_EXPRESSION "Error: Unknown option"
    )
else()
    message(STATUS "Skipping opencl_info (requires OpenCL)")
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

# opengl_info: print OpenGL platform + version info
if(OpenGL_FOUND AND WIN32 OR (X11_FOUND AND OpenGL_GLX_FOUND))
    add_executable(opengl_info opengl_info.cc)
    target_link_libraries(opengl_info PRIVATE OpenGL::GL pdmpmt++)
    # on Windows, link to relevant system libraries like GDI
    if(WIN32)
        target_link_libraries(opengl_info PRIVATE gdi32 user32)
    # otherwise, using GLX + X11
    else()
        target_link_libraries(opengl_info PRIVATE OpenGL::GLX X11::X11)
    endif()
    # smoke test
    add_test(NAME opengl_info COMMAND opengl_info)
    # -h, --help option
    add_test(NAME opengl_info_h COMMAND opengl_info -h)
    add_test(NAME opengl_info_help COMMAND opengl_info --help)
    set_tests_properties(
        opengl_info_h opengl_info_help PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage:"
    )
    # -nx-gl option (no OpenGL extensions)
    add_test(NAME opengl_info_nx_gl COMMAND opengl_info -nx-gl)
    set_tests_properties(
        opengl_info_nx_gl PROPERTIES
        FAIL_REGULAR_EXPRESSION "OpenGL extensions"
    )
    # -nx-glx option (no GLX extensions)
    if(X11_FOUND AND OpenGL_GLX_FOUND)
        add_test(NAME opengl_info_nx_glx COMMAND opengl_info -nx-glx)
        set_tests_properties(
            opengl_info_nx_glx PROPERTIES
            FAIL_REGULAR_EXPRESSION "GLX (client|server) extensions"
        )
    endif()
    # when built with X11 and GLX, if there is no display, skip the test
    if(X11_FOUND)
        set_tests_properties(
            opengl_info
            opengl_info_nx_gl
            opengl_info_nx_glx PROPERTIES
            SKIP_REGULAR_EXPRESSION "Unable to open default display"
        )
    endif()
else()
    set(opengl_info_requires "OpenGL")
    # if not on Windows, need X11 + GLX
    if(NOT WIN32)
        string(APPEND opengl_info_requires " + GLX + X11")
    endif()
    message(STATUS "Skipping opengl_info (requires ${opengl_info_requires})")
    unset(opengl_info_requires)
endif()

# ray_mcpi_remote: DSO for ray_mcpi
# we need this because Ray resolves function names registered at load-time with
# RAY_REMOTE by scanning the dynamic symbol tables of the DSOs it loads. the
# load locations are determined via search directories
if(TARGET pdmpmt::ray_api)
    add_library(ray_mcpi_remote MODULE ray_mcpi.cc)
    target_link_libraries(ray_mcpi_remote PRIVATE pdmpmt++ pdmpmt::ray_api)
    set_target_properties(
        ray_mcpi_remote PROPERTIES
        DEFINE_SYMBOL PDMPMT_BUILD_RAY_MCPI_REMOTE
    )
else()
    message(STATUS "Skipping ray_mcpi_remote (requires Ray C++ API)")
endif()

# ray_mcpi: compute pi using Monte Carlo via Ray's C++ API
if(TARGET pdmpmt::ray_api)
    add_executable(ray_mcpi ray_mcpi.cc)
    target_link_libraries(ray_mcpi PRIVATE pdmpmt++ pdmpmt::ray_api)
    add_test(NAME ray_mcpi COMMAND ray_mcpi)
    # the same default Ray runtime is started + stopped
    set_tests_properties(ray_mcpi PROPERTIES RESOURCE_LOCK ray_runtime)
else()
    message(STATUS "Skipping ray_mcpi (requires Ray C++ API)")
endif()
