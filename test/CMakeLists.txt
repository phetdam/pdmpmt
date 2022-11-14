cmake_minimum_required(VERSION 3.20)

add_executable(pdmpmt_test mcpi_test.cc)
target_link_libraries(pdmpmt_test GTest::gtest_main)

# enable test discovery for Google Test
include(GoogleTest)
gtest_discover_tests(pdmpmt_test)
