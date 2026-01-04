cmake_minimum_required(VERSION 3.21)

project(
    pdmpmt
    VERSION 0.1.0
    DESCRIPTION "C/C++ experiments on mixed-hardware computing"
    HOMEPAGE_URL "https://github.com/phetdam/pdmpmt"
    LANGUAGES C CXX
)

# TODO: consider only allowing building as shared
option(BUILD_SHARED_LIBS "Build libraries as shared instead of static" ON)
# build tests
option(PDMPMT_BUILD_TESTS "Build test programs" ON)
# build as release version without build suffix
option(PDMPMT_RELEASE_BUILD "Build as an official release" OFF)

# find Git
find_package(Git)
if(Git_FOUND)
    message(STATUS "Git version: ${GIT_VERSION_STRING}")
else()
    message(STATUS "Git version: None")
endif()

# check if generator is multi-config
get_property(PDMPMT_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(PDMPMT_MULTI_CONFIG)
    message(STATUS "Generator config: Multi")
else()
    message(STATUS "Generator config: Single")
endif()

# build debug by default; we set -DCMAKE_BUILD_TYPE=Release for release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
if(PDMPMT_MULTI_CONFIG)
    message(STATUS "Build config: Multi")
else()
    message(STATUS "Build config: ${CMAKE_BUILD_TYPE}")
endif()

# C/C++ standards are all _reasonably_ recent
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# status messages
if(BUILD_SHARED_LIBS)
    message(STATUS "Build libraries: Shared")
else()
    message(STATUS "Build libraries: Static")
endif()
if(PDMPMT_BUILD_TESTS)
    message(STATUS "Build tests: Yes")
else()
    message(STATUS "Build tests: No")
endif()

# make all artifacts output in top-level. multi-config generators like Visual
# Studio will have per-config subdirectories (this is intentional)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# update module search path to find our CMake scripts
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(pdmpmt_config_compile)
include(pdmpmt_find_pip_packages)
include(pdmpmt_write_build_info)

# set major minor patch version (because we want all-caps prefix)
set(PDMPMT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(PDMPMT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(PDMPMT_VERSION_PATCH ${PROJECT_VERSION_PATCH})
# determine build info (empty for actual release) + set build version
if(PDMPMT_RELEASE_BUILD)
    set(PDMPMT_BUILD_INFO "")
    # PDMPMT_VERSION implicitly set
    # pip version for Python package
    set(PDMPMT_PY_VERSION ${PDMPMT_VERSION})
else()
    pdmpmt_write_build_info(PDMPMT_BUILD_INFO)
    # update to include build info
    set(PDMPMT_VERSION ${PROJECT_VERSION}-${PDMPMT_BUILD_INFO})
    # note: since __init__.py is being generated we don't want WSL and Windows
    # to keep overwriting each other's version so no extra build info
    set(PDMPMT_PY_VERSION ${PROJECT_VERSION}.dev0)
endif()
message(STATUS "Build version: ${PDMPMT_VERSION}")

# find CUDA Toolkit
# note: we choose CUDA Toolkit 12 as then NVCC can support C++20
# note: bumped minimum CUDA Toolkit version from 12.2 to 12.4 due to upgrading
# to VS2022 17.10 which causes enable_language(CUDA) to fail due to an elevated
# CUDA Toolkit version requirement. _ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
# being defined for nvcc doesn't help because we can't force this as part of
# the enable_language(CUDA) call unfortunately. for more details see the
# following: https://stackoverflow.com/a/78576792/14227825
find_package(CUDAToolkit 12.4)
# if found, configure
if(CUDAToolkit_FOUND)
    message(STATUS "CUDA version: ${CUDAToolkit_VERSION}")
    # enable CUDA for project
    enable_language(CUDA)
    # same C++ standard and requirement
    set(CMAKE_CUDA_STANDARD ${CMAKE_CXX_STANDARD})
    set(CMAKE_CUDA_STANDARD_REQUIRED ${CMAKE_CXX_STANDARD_REQUIRED})
else()
    message(STATUS "CUDA version: None")
endif()

# OpenMP target version depends on the compiler. MSVC OpenMP support lags
# behind GCC's a lot (only 2.0) while GCC 9.x and Clang 3.9+ implement 4.5
if(MSVC)
    set(_omp_find_version 2.0)
else()
    set(_omp_find_version 4.5)
endif()
# find OpenMP
find_package(OpenMP ${_omp_find_version} COMPONENTS C CXX)
if(OpenMP_FOUND)
    message(STATUS "OpenMP C version: ${OpenMP_C_VERSION}")
    message(STATUS "OpenMP C++ version: ${OpenMP_CXX_VERSION}")
else()
    message(STATUS "OpenMP C version: None")
    message(STATUS "OpenMP C++ version: None")
endif()
unset(_omp_find_version)

# for check_symbol_exists. we use this for version detection via macro checks
include(CheckSymbolExists)

# find OpenCL
# note: OpenCL 3.0 only detected starting from CMake 3.24
find_package(OpenCL 2.0)
if(OpenCL_FOUND)
    message(STATUS "OpenCL version: ${OpenCL_VERSION_STRING}")
    # if CL_TARGET_OPENCL_VERSION is not defined we get a compiler warning
    # TODO: should we try parsing it out of cl_version.h? e.g. try_compile
    set(PDMPMT_OPENCL_TARGET ${OpenCL_VERSION_MAJOR}${OpenCL_VERSION_MINOR}0)
    message(STATUS "OpenCL CL_TARGET_OPENCL_VERSION: ${PDMPMT_OPENCL_TARGET}")
    # pdmpmt_opencl_target_version: interface target for defining the OpenCL
    # target version via CL_TARGET_OPENCL_VERSION
    add_library(pdmpmt_opencl_target_version INTERFACE)
    add_library(pdmpmt::opencl_target_version ALIAS pdmpmt_opencl_target_version)
    target_compile_definitions(
        pdmpmt_opencl_target_version INTERFACE
        CL_TARGET_OPENCL_VERSION=${PDMPMT_OPENCL_TARGET}
    )
else()
    message(STATUS "OpenCL version: None")
endif()

# find OpenGL
# will assume use of GLX on POSIX systems (also update COMPONENTS)
find_package(OpenGL COMPONENTS OpenGL OPTIONAL_COMPONENTS GLX)
# known OpenGL versions: https://www.khronos.org/opengl/wiki/History_of_OpenGL
set(
    gl_versions
    1.1 1.2 1.3 1.4 1.5 2.0 2.1 3.0 3.1 3.2 3.3 4.0 4.1 4.2 4.3 4.4 4.5 4.6
)
# check for OpenGL core
if(OpenGL_FOUND)
    # bug on Windows: OPENGL_INCLUDE_DIR is never set
    if(WIN32 AND NOT OPENGL_INCLUDE_DIR)
        find_path(OPENGL_INCLUDE_DIR Gl/gl.h NO_CACHE)
    endif()
    # if found, attempt to determine OpenGL header version. default is 1.0.
    # this determines the highest statically available OpenGL version
    set(PDMPMT_OPENGL_GL_VERSION 1.0)
    # note: gl.h usually doesn't provide anything beyond GL_VERSION_1_3.
    # however, modern Linux distributions typically have gl.h include glext.h
    # unless GL_GLEXT_LEGACY is defined, so transitively you get GL_VERSION_4_6
    foreach(gl_ver ${gl_versions})
        # convert 1.1 -> 1_1 for macro checking
        string(REPLACE "." "_" gl_ver_suffix "${gl_ver}")
        # for Windows, check_symbol_exists seems to fail regardless if we use
        # CMAKE_REQUIRED_DIRS or OPENGL_INCLUDE_DIR, even with conversion to a
        # CMake-style path. that's why for Windows we instead use try_compile
        if(WIN32)
            # first, configure gl_check (use native newlines)
            # variables consumed: gl_ver_suffix
            configure_file(cmake/gl_check.cc.in gl_check.cc @ONLY)
            # now attempt to compile
            message(CHECK_START "Looking for GL_VERSION_${gl_ver_suffix}")
            try_compile(
                gl_version_found
                ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/gl_check.cc
                # for consistency, use same C++ standard
                CXX_STANDARD ${CMAKE_CXX_STANDARD}
                # note: use OUTPUT_VARIABLE if we need compiler output
            )
            # report success/fail. we short-circuit if necessary
            if(gl_version_found)
                message(CHECK_PASS "found")
                set(PDMPMT_OPENGL_GL_VERSION ${gl_ver})
            else()
                message(CHECK_FAIL "not found")
                break()
            endif()
        # otherwise, use check_symbol_exists
        else()
            check_symbol_exists(
                GL_VERSION_${gl_ver_suffix}
                ${OPENGL_INCLUDE_DIR}/GL/gl.h
                gl_version_found_${gl_ver_suffix}
            )
            # if found, update OpenGL version
            if(gl_version_found_${gl_ver_suffix})
                set(PDMPMT_OPENGL_GL_VERSION ${gl_ver})
            # otherwise, short circuit
            else()
                break()
            endif()
        endif()
    endforeach()
    # TODO: on success, compile and run a test program to create an OpenGL
    # context and call glGetString(GL_VERSION) to report the OpenGL version.
    # we probably also need to print out the device context, etc.
    message(STATUS "OpenGL version: ${PDMPMT_OPENGL_GL_VERSION}")
else()
    message(STATUS "OpenGL version: None")
endif()
# GLU
if(OPENGL_GLU_FOUND)
    # if found, attempt to determine GLU version for each known minor version
    set(PDMPMT_GLU_VERSION 1.0)
    foreach(glu_minor 1 2 3)
        # same issue with checking OpenGL header version
        if(WIN32)
            # first, configure glu_check (use native newlines)
            # variables consumed: glu_minor
            configure_file(cmake/glu_check.cc.in glu_check.cc @ONLY)
            # now attempt to compile
            message(CHECK_START "Looking for GLU_VERSION_1_${glu_minor}")
            try_compile(
                glu_version_found
                ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/glu_check.cc
                # for consistency, use same C++ standard
                CXX_STANDARD ${CMAKE_CXX_STANDARD}
                # note: use OUTPUT_VARIABLE if we need compiler output
            )
            # report success/fail. we short-circuit if necessary
            if(glu_version_found)
                message(CHECK_PASS "found")
                set(PDMPMT_GLU_VERSION 1.${glu_minor})
            else()
                message(CHECK_FAIL "not found")
                break()
            endif()
        else()
            check_symbol_exists(
                GLU_VERSION_1_${glu_minor}
                ${OPENGL_INCLUDE_DIR}/GL/glu.h
                glu_version_found_${glu_minor}
            )
            # if found, update GLU version
            if(glu_version_found_${glu_minor})
                set(PDMPMT_GLU_VERSION 1.${glu_minor})
            # otherwise, short circuit
            else()
                break()
            endif()
        endif()
    endforeach()
    # report
    message(STATUS "GLU version: ${PDMPMT_GLU_VERSION}")
else()
    message(STATUS "GLU version: None")
endif()
# GLX
if(OpenGL_GLX_FOUND)
    # if found, attempt to determine GLX version for each of the GLX minor
    # versions that are known. assumes GLX version is 1.0 otherwise
    set(PDMPMT_GLX_VERSION 1.0)
    foreach(glx_minor 1 2 3 4)
        check_symbol_exists(
            GLX_VERSION_1_${glx_minor}
            ${OPENGL_INCLUDE_DIR}/GL/glx.h
            glx_version_found_${glx_minor}
        )
        # if found, update GLX version
        if(glx_version_found_${glx_minor})
            set(PDMPMT_GLX_VERSION 1.${glx_minor})
        endif()
    endforeach()
    # report
    message(STATUS "GLX version: ${PDMPMT_GLX_VERSION}")
else()
    message(STATUS "GLX version: None")
endif()

# find X11 (for use with GLX)
# TODO: do we have a way to get the version without using pkg-config?
message(CHECK_START "Looking for X11")
find_package(X11)
if(X11_FOUND)
    message(CHECK_PASS "found")
else()
    message(CHECK_FAIL "not found")
endif()

# Google Test for unit test runners
find_package(GTest 1.10)
if(GTest_FOUND)
    message(STATUS "Google Test version: ${GTest_VERSION}")
    include(GoogleTest)  # enable test discovery
else()
    message(STATUS "Google Test version: None")
endif()

# find Python (note: may add Development component later)
find_package(Python3 COMPONENTS Interpreter NumPy)
if(Python3_FOUND)
    message(STATUS "Python version: ${Python3_VERSION}")
else()
    message(STATUS "Python version: None")
endif()
if(Python3_NumPy_FOUND)
    message(STATUS "NumPy version: ${Python3_NumPy_VERSION}")
else()
    message(STATUS "NumPy version: None")
endif()

# if we have Python 3, find relevant packages
if(Python3_FOUND)
    pdmpmt_find_pip_packages(
        OPTIONAL
            dask>=2025.2
            distributed>=2025.3
            pytest>=8
            # note: ray-cpp must be separately installed from ray
            ray-cpp>=2.40
    )
endif()
# if ray-cpp is found create an imported target to link against
# note: not supported on Windows. we also would have to use find_file to locate
# the library because the DLL is named libray_api.so. it's not possible to use
# it because MSVC complains that the DLL is corrupt and there's also probably
# not enough symbols that are exported from the DLL (also no import lib)
if(NOT WIN32 AND ray-cpp_PIP_FOUND)
    # Ray C++ root, include dir, library dir
    # TODO: Ray has vendored dependencies. use find_package on them?
    set(ray-cpp_ROOT_DIR ${Python3_SITEARCH}/ray/cpp)
    # need to normalize to CMake path because Python3_SITEARCH contains the raw
    # Windows path that is returned from Python
    cmake_path(CONVERT ${ray-cpp_ROOT_DIR} TO_CMAKE_PATH_LIST ray-cpp_ROOT_DIR)
    set(ray-cpp_INCLUDE_DIRS ${ray-cpp_ROOT_DIR}/include)
    # find the main Ray API library
    find_library(
        ray-cpp_LIBRARY
        NAMES ray_api
        HINTS ${ray-cpp_ROOT_DIR}
        PATH_SUFFIXES lib
        NO_CACHE
        REQUIRED
    )
    # create shared IMPORTED target for ray_api library
    add_library(pdmpmt::ray_api SHARED IMPORTED)
    set_target_properties(
        pdmpmt::ray_api PROPERTIES
        IMPORTED_LOCATION "${ray-cpp_LIBRARY}"
    )
    target_include_directories(pdmpmt::ray_api INTERFACE ${ray-cpp_INCLUDE_DIRS})
    # note: binary Python wheels for Linux still use the pre-C++11 ABI. another
    # example would be PyTorch; this is also a required compile definition. see
    # https://github.com/ray-project/ray/issues/26031
    # TODO: may not need this?
    target_compile_definitions(pdmpmt::ray_api INTERFACE _GLIBCXX_USE_CXX11_ABI=0)
    # build Ray example source as shared library. this is necessary to properly
    # have the Plus, Counter::* symbols available for export. we use MODULE
    # instead of SHARED since there are no headers and Ray will dlopen() this
    add_library(ray_example_lib MODULE ${ray-cpp_ROOT_DIR}/example/example.cc)
    target_link_libraries(ray_example_lib PRIVATE pdmpmt::ray_api)
    # build Ray example source as executable. yes this is somewhat redundant
    # in the case of the example, but in a real-life setting, you would
    # probably have extern "C" symbols exported from shared objects separate
    # from the actual application logic that is creating a ray::Task.
    # note: has issues with SO_REUSEPORT on WSL1 (although it still runs). see
    # https://github.com/grpc/grpc/issues/28936 and others
    # note: by default, Ray will look for dynamic libraries to load in the
    # directory that the running application resides in
    add_executable(ray_example ${ray-cpp_ROOT_DIR}/example/example.cc)
    target_link_libraries(ray_example PRIVATE pdmpmt::ray_api)
    # smoke test for the Ray example
    add_test(NAME ray_example COMMAND ray_example)
    # the same default Ray runtime is started + stopped
    # TODO: can we have some nanny process that calls ray::Init() and
    # ray::Shutdown() and just have the tests connect to the existing runtime?
    set_tests_properties(ray_example PROPERTIES RESOURCE_LOCK ray_runtime)
else()
    message(STATUS "Skipping ray_example (Ray C++ API not supported on Windows)")
    message(
        STATUS
        "Skipping ray_example_lib (Ray C++ API not supported on Windows)"
    )
endif()

# set compiler-specific configuration options. this is done after finding all
# dependencies so there is no possible interference with upstream compile flags
pdmpmt_config_compile()

# base include for this repo
set(PDMPMT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# generate version.h header
set(PDMPMT_VERSION_HEADER include/pdmpmt/version.h)
# variables consumed:
#
#   PDMPMT_VERSION
#   PDMPMT_VERSION_MAJOR
#   PDMPMT_VERSION_MINOR
#   PDMPMT_VERSION_PATCH
#   PDMPMT_BUILD_INFO
#
configure_file(
    ${PDMPMT_VERSION_HEADER}.in ${PDMPMT_VERSION_HEADER}
    NEWLINE_STYLE LF @ONLY
)
message(STATUS "Generated ${PDMPMT_VERSION_HEADER}")

# generate __init__.py
set(PDMPMT_INIT_PY pdmpmt/__init__.py)
# variables consumed:
#
#   PDMPMT_PY_VERSION
#
configure_file(
    ${PDMPMT_INIT_PY}.in ${CMAKE_CURRENT_SOURCE_DIR}/${PDMPMT_INIT_PY}
    NEWLINE_STYLE LF @ONLY
)
message(STATUS "Generated ${PDMPMT_INIT_PY}")

# pdmpmt++: C++ header-only library implementation
add_library(pdmpmt++ INTERFACE)
add_library(pdmpmt::pdmpmt++ ALIAS pdmpmt++)
target_include_directories(
    pdmpmt++ INTERFACE
    $<BUILD_INTERFACE:${PDMPMT_INCLUDE_DIR}>
    # config-independent generated headers
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

include(CTest)

# ensure BUILD_TESTING matches PDMPMT_BUILD_TESTS
if(NOT BUILD_TESTING AND PDMPMT_BUILD_TESTS)
    set(BUILD_TESTING ON)
endif()

add_subdirectory(src)

if(PDMPMT_BUILD_TESTS)
    add_subdirectory(test)
endif()

# also build third-party dependencies
add_subdirectory(third_party)
