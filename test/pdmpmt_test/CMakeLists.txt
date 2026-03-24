cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

if(GTest_FOUND)
    # pdmpmt_test: C++ unit test program
    add_executable(
        pdmpmt_test
        block_test.cc file_mutex_test.cc scope_exit_test.cc
    )
    target_link_libraries(pdmpmt_test PRIVATE pdmpmt pdmpmt++)
    # if GoogleMock is available we want to use its main instead
    if(TARGET GTest::gmock_main)
        target_link_libraries(pdmpmt_test PRIVATE GTest::gmock_main)
    # otherwise just use the GoogleTest main
    else()
        target_link_libraries(pdmpmt_test PRIVATE GTest::gtest_main)
    endif()
    # register Google Tests tests with CTest
    gtest_discover_tests(pdmpmt_test)
else()
    message(STATUS "Skipping pdmpmt_test (Google Test required)")
endif()
