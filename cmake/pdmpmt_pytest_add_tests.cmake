cmake_minimum_required(VERSION 3.20)

##
# pdmpmt_pytest_add_tests.cmake
#
# Provides a function to collect pytest tests for registration with CTest.
#

include_guard(GLOBAL)

##
# Collect pytest tests and register them individually as CTest tests.
#
# This uses the --collect-only and -q pytest options to obtain the list of
# discovered tests which we can then have pytest individually run under CTest.
#
# Arguments:
#   [FROM file_or_dir]      File or directory to collect tests from. If not
#                           specified, ${CMAKE_CURRENT_SOURCE_DIR} is used. If
#                           the path is relative it will be resolved relative
#                           to ${CMAKE_CURRENT_SOURCE_DIR}. All pytest tests
#                           will be run using this as the working directory.
#
function(pdmpmt_pytest_add_tests)
    # parse arguments
    cmake_parse_arguments(ARG "" "FROM" "" ${ARGN})
    # if FROM given, normalize into absolute path
    if(DEFINED ARG_FROM)
        cmake_path(ABSOLUTE_PATH ARG_FROM NORMALIZE)
    # default to CMAKE_CURRENT_SOURCE_DIR
    else()
        set(ARG_FROM "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    # run pytest to collect tests
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pytest --collect-only -q "${ARG_FROM}"
        WORKING_DIRECTORY "${ARG_FROM}"
        RESULT_VARIABLE collect_res
        OUTPUT_VARIABLE collect_out
        ERROR_VARIABLE collect_out
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )
    # error
    if(collect_res)
        message(FATAL_ERROR "pytest collection failure: ${collect_out}")
    endif()
    # otherwise, convert output into list. first remove last two newlines and
    # final summary line to get a real list of tests
    string(REGEX REPLACE "\n\n[0-9]+.+$" "" test_list "${collect_out}")
    string(REPLACE "\n" ";" test_list "${test_list}")
    # loop through items to add tests
    foreach(test_name ${test_list})
        add_test(
            NAME "${test_name}"
            COMMAND "${Python3_EXECUTABLE}" -m pytest "${test_name}"
            WORKING_DIRECTORY "${ARG_FROM}"
        )
    endforeach()
endfunction()