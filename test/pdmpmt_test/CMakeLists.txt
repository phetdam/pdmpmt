cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

if(GTest_FOUND)
    # pdmpmt_test: C++ unit test program
    add_executable(pdmpmt_test block_test.cc)
    target_link_libraries(pdmpmt_test PRIVATE GTest::gtest_main pdmpmt pdmpmt++)
    # register Google Tests tests with CTest
    gtest_discover_tests(pdmpmt_test)
else()
    message(STATUS "Skipping pdmpmt_test (Google Test required)")
endif()
