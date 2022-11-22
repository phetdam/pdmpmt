cmake_minimum_required(VERSION 3.20)

add_executable(pdmpmt_test mcpi_test.cc)
# link OpenMP if OpenMP is available (only need for C++)
if(OpenMP_FOUND)
    target_link_libraries(pdmpmt_test OpenMP::OpenMP_CXX)
endif()
# link C library + Google Test main for entry point
target_link_libraries(pdmpmt_test pdmpmt_c GTest::gtest_main)

# enable test discovery for Google Test
include(GoogleTest)
gtest_discover_tests(pdmpmt_test)
