cmake_minimum_required(VERSION 3.21)

add_executable(pdmpmt_test mcpi_test.cc)
# link OpenMP if OpenMP is available (only need for C++)
if(OpenMP_FOUND)
    target_link_libraries(pdmpmt_test OpenMP::OpenMP_CXX)
endif()
target_link_libraries(pdmpmt_test pdmpmt_c GTest::gtest_main)

# on Windows, copy all DLLs in the project pdmpmt_test depends on is if they
# have changed. however, note that this DOES NOT copy in external DLLs!
if(WIN32)
    add_custom_command(
        TARGET pdmpmt_test POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:pdmpmt_test> $<TARGET_FILE_DIR:pdmpmt_test>
        COMMAND_EXPAND_LISTS
    )
endif()

# enable test discovery for Google Test
include(GoogleTest)

# TODO: disable on Windows until we figure out better GSL integration
if(WIN32)
    message(STATUS "Test discovery disabled on Windows temporarily")
else()
    gtest_discover_tests(pdmpmt_test)
endif()
