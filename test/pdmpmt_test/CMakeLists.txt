cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

if(GTest_FOUND)
    # pdmpmt_test: C++ unit test program
    add_executable(
        pdmpmt_test
        block_test.cc
        file_mutex_test.cc
        paths_test.cc
        scope_exit_test.cc
        span_test.cc
        thread_pool_test.cc
    )
    # paths_test.cc requires PDMPMT_CONFIG_BUILD_PATH definition
    set_property(
        SOURCE paths_test.cc APPEND PROPERTY
        COMPILE_DEFINITIONS
        "PDMPMT_CONFIG_BUILD_PATH=\"$<TARGET_FILE:pdmpmt_config>\""
    )
    # span_test.cc includes CCCL headers when CUDA is available due to the
    # transitive usage requirements from CUDA::cudart and for newer versions of
    # CUDA the standards-compliant MSVC preprocessor is necessary
    if(PDMPMT_CUDA_ENABLED AND MSVC)
        set_property(
            SOURCE span_test.cc APPEND PROPERTY
            COMPILE_OPTIONS "/Zc:preprocessor"
        )
    endif()
    # if CUDA enabled for project add CUDA compile group
    if(PDMPMT_CUDA_ENABLED)
        pdmpmt_cuda_compile_group(pdmpmt_test_cu span_test.cu)
        # pdmpmt_cuda_compile_options(pdmpmt_test_cu PRIVATE -diag-suppress 177)
        pdmpmt_cuda_link_libraries(
            pdmpmt_test_cu PRIVATE
            CUDA::cudart GTest::gtest cuda_compile_options pdmpmt++
        )
        if(TARGET GTest::gmock)
            pdmpmt_cuda_link_libraries(pdmpmt_test_cu PRIVATE GTest::gmock)
        endif()
        pdmpmt_cuda_add_groups(pdmpmt_test pdmpmt_test_cu)
    endif()
    target_link_libraries(pdmpmt_test PRIVATE pdmpmt pdmpmt_config pdmpmt++)
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
