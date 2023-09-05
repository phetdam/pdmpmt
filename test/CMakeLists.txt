cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

add_executable(pdmpmt_test mcpi_test.cc)
# link OpenMP if OpenMP is available (only need C++ target)
if(OpenMP_FOUND)
    target_link_libraries(pdmpmt_test PRIVATE OpenMP::OpenMP_CXX)
endif()
target_link_libraries(pdmpmt_test PRIVATE pdmpmt GTest::gtest_main)

# on Windows, copy all DLLs in the project pdmpmt_test depends on is if they
# have changed. however, note that this DOES NOT copy in DLLs of targets that
# are unaware of their status as SHARED libraries (e.g. GSL)!
if(WIN32)
    add_custom_command(
        TARGET pdmpmt_test POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:pdmpmt_test> $<TARGET_FILE_DIR:pdmpmt_test>
        COMMAND_EXPAND_LISTS
    )
endif()

# add custom post-build action copying GSL DLLs to where test runner is. no
# effect on non-Windows systems and emits warning if using static GSL libs
copy_gsl_dlls_post_build(pdmpmt_test)

# enable test discovery for Google Test
include(GoogleTest)

gtest_discover_tests(pdmpmt_test)
