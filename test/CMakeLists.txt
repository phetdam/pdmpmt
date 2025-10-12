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

# add unit test program subdirectory
add_subdirectory(pdmpmt_test)
