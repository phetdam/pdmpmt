cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

if(GTest_FOUND)
    # mcpi_test: C++ Monte Carlo pi estimation test program
    add_executable(mcpi_test mcpi_test.cc)
    target_link_libraries(mcpi_test PRIVATE GTest::gtest_main pdmpmt pdmpmt++)
    # link OpenMP if OpenMP is available (only need C++ target)
    if(OpenMP_FOUND)
        target_link_libraries(mcpi_test PRIVATE OpenMP::OpenMP_CXX)
    endif()
    # register Google Tests tests with CTest
    gtest_discover_tests(mcpi_test)
else()
    message(STATUS "Skipping mcpi_test (Google Test required)")
endif()

# add unit test program subdirectory
add_subdirectory(pdmpmt_test)

# file_mutex_mt_test: test file_mutex inter-thread synchronization
add_executable(file_mutex_mt_test file_mutex_mt_test.cc)
target_link_libraries(file_mutex_mt_test PRIVATE pdmpmt++)
# -h, --help
add_test(NAME file_mutex_mt_test_h COMMAND file_mutex_mt_test -h)
add_test(NAME file_mutex_mt_test_help COMMAND file_mutex_mt_test --help)
set_tests_properties(
    file_mutex_mt_test_h
    file_mutex_mt_test_help PROPERTIES
    PASS_REGULAR_EXPRESSION "Usage:"
)
# run for different thread counts
add_test(NAME file_mutex_mt_test_n100 COMMAND file_mutex_mt_test -n 100)
add_test(NAME file_mutex_mt_test_n250 COMMAND file_mutex_mt_test -n 250)
add_test(NAME file_mutex_mt_test_n400 COMMAND file_mutex_mt_test -n 400)
# note: must run serially since tests share the same lockfile
set_tests_properties(
    file_mutex_mt_test_n100
    file_mutex_mt_test_n250
    file_mutex_mt_test_n400 PROPERTIES
    RESOURCE_LOCK file_mutex_mt_test_lock
)

# file_mutex_mp_test: file_mutex inter-process synchronization test master
add_executable(file_mutex_mp_test file_mutex_mp_test.cc)
target_link_libraries(file_mutex_mp_test PRIVATE pdmpmt++ pdmpmt_config)
if(WIN32)
    target_link_libraries(file_mutex_mp_test PRIVATE kernel32)
endif()
# -h, --help
add_test(NAME file_mutex_mp_test_h COMMAND file_mutex_mp_test -h)
add_test(NAME file_mutex_mp_test_help COMMAND file_mutex_mp_test --help)
set_tests_properties(
    file_mutex_mp_test_h
    file_mutex_mp_test_help PROPERTIES
    PASS_REGULAR_EXPRESSION "Usage: "
)
# invalid -n value
add_test(NAME file_mutex_mp_test_n0 COMMAND file_mutex_mp_test -n 0)
set_tests_properties(
    file_mutex_mp_test_n0 PROPERTIES
    PASS_REGULAR_EXPRESSION "workers cannot be zero"
)

# file_mutex_mp_test_worker: file_mutex inter-process sync test worker
add_executable(file_mutex_mp_test_worker file_mutex_mp_test_worker.cc)
target_link_libraries(file_mutex_mp_test_worker PRIVATE pdmpmt++ pdmpmt_config)

# file_mutex multiprocess synchronization tests
add_test(NAME file_mutex_mp_test_n100 COMMAND file_mutex_mp_test -n 100)
add_test(NAME file_mutex_mp_test_n250 COMMAND file_mutex_mp_test -n 250)
add_test(NAME file_mutex_mp_test_n400 COMMAND file_mutex_mp_test -n 400)
# note: must run serially since tests share the same lock + counter files
set_tests_properties(
    file_mutex_mp_test_n100
    file_mutex_mp_test_n250
    file_mutex_mp_test_n400 PROPERTIES
    RESOURCE_LOCK file_mutex_mp_test_lock
)
