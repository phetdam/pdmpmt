cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

if(GTest_FOUND)
    # pdmpmt_test: C++ unit test program
    # TODO: move mcpi tests out into separate programs
    add_executable(pdmpmt_test block_test.cc mcpi_test.cc)
    # link OpenMP if OpenMP is available (only need C++ target)
    if(OpenMP_FOUND)
        target_link_libraries(pdmpmt_test PRIVATE OpenMP::OpenMP_CXX)
    endif()
    target_link_libraries(pdmpmt_test PRIVATE GTest::gtest_main pdmpmt)
endif()

# on Windows, copy all DLLs in the project pdmpmt_test depends on is if they
# have changed. however, note that this DOES NOT copy in DLLs of targets that
# are unaware of their status as SHARED libraries!
# FIXME: remove, no longer have DLLs that pdmpmt_test depends on
# if(WIN32)
#     add_custom_command(
#         TARGET pdmpmt_test POST_BUILD
#         COMMAND
#         ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_RUNTIME_DLLS:pdmpmt_test> $<TARGET_FILE_DIR:pdmpmt_test>
#         COMMAND_EXPAND_LISTS
#     )
# endif()

# add custom post-build action copying GSL DLLs to where test runner is. no
# effect on non-Windows systems and emits warning if using static GSL libs
# copy_gsl_dlls_post_build(pdmpmt_test)

# enable test discovery for Google Test
if(GTest_FOUND)
    include(GoogleTest)
    gtest_discover_tests(pdmpmt_test)
endif()

# criteria for running Dask tests
if(
    Python3_NumPy_FOUND AND
    dask_PIP_FOUND AND
    distributed_PIP_FOUND AND
    pytest_PIP_FOUND
)
    set(_run_dask_tests TRUE)
else()
    set(_run_dask_tests FALSE)
endif()

# run pytest test if possible
# TODO: find way to run tests individually
if(_run_dask_tests)
    add_test(
        NAME pdmpmt_pytest_tests
        COMMAND pytest WORKING_DIRECTORY
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
else()
    message(
        STATUS
        "Skipping pdmpmt_pytest_tests (requires Python 3, NumPy, Dask, pytest)"
    )
endif()
