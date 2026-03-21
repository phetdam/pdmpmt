cmake_minimum_required(VERSION 3.20)

##
# pdmpmt_pytest_add_tests.cmake
#
# Module providing post-build pytest test collection for CTest registration.
#
# This operates similarly in principle to gtest_discover_tests(), hanging a
# post-build command on a target that will invoke pytest with --collect-only -q
# to get a list of tests that will be run, writing a CTest test script with
# add_test() commands, and setting TEST_INCLUDE_FILES.
#

include_guard(GLOBAL)

##
# Collect pytest tests and register them individually as CTest tests.
#
# This uses the --collect-only and -q pytest options to obtain the list of
# discovered tests which we can then have pytest individually run under CTest.
# Each add_test() command invokes pytest with the reported pytest test specs
# prepended with the test discovery directory to ensure that pytest can find
# the tests correctly even when running in a different build directory.
#
# Required variables:
#
#   Python3_EXECUTABLE      Absolute path to the Python 3 interpreter used
#   TEST_ROOT               Absolute path to start pytest test discovery in
#   TEST_SCRIPT_PATH        Absolute path to write the resulting CMake script
#
function(pdmpmt_pytest_add_tests_impl)
    # required variables
    if(NOT Python3_EXECUTABLE)
        message(FATAL_ERROR "No Python 3 executable specified")
    endif()
    if(NOT TEST_ROOT)
        message(FATAL_ERROR "No test discovery root specified")
    endif()
    if(NOT TEST_SCRIPT_PATH)
        message(FATAL_ERROR "No CTest test script path specified")
    endif()
    # run pytest to collect tests
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pytest --collect-only -q "${TEST_ROOT}"
        WORKING_DIRECTORY "${TEST_ROOT}"
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
    # note: use absolute path so pytest will automatically figure out what the
    # test root directory is supposed to be. this does not correctly the
    # working directory, however, so that's still an issue
    foreach(test_name ${test_list})
        string(
            APPEND test_script_content
            "add_test(\n"
            "    \"${test_name}\"\n"
            "    \"${Python3_EXECUTABLE}\" -m pytest \"${TEST_ROOT}/${test_name}\"\n"
            ")\n"
        )
    endforeach()
    # write test list to file
    file(WRITE "${TEST_SCRIPT_PATH}" "${test_script_content}")
endfunction()

##
# Add a target post-build command to register pytest tests as CTest tests.
#
# It operates similar to gtest_discover_tests() by hanging a post-build command
# on a target that generates a CTest script used with TEST_INCLUDE_FILES. The
# pytest --collect-only and -q pytest options are used to get the test list.
#
# Arguments:
#   target                  Target to use as connection to build graph. This
#                           could be any target, e.g. one added using
#                           add_custom_target(), that we want the test
#                           collection step to run after. The target could be
#                           one added with pdmpmt_add_pip_package().
#
#   [FROM file_or_dir]      File or directory to collect tests from. If not
#                           specified, ${CMAKE_CURRENT_SOURCE_DIR} is used. If
#                           the path is relative it will be resolved relative
#                           to ${CMAKE_CURRENT_SOURCE_DIR}. All pytest tests
#                           will be run using this as the working directory.
#
function(pdmpmt_pytest_add_tests target)
    # parse arguments
    cmake_parse_arguments(ARG "" "FROM" "" ${ARGN})
    # if FROM given, normalize into absolute path
    if(DEFINED ARG_FROM)
        cmake_path(ABSOLUTE_PATH ARG_FROM NORMALIZE)
    # default to CMAKE_CURRENT_SOURCE_DIR
    else()
        set(ARG_FROM "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    # base name for the CTest script to include with TEST_INCLUDE_FILES
    set(test_script_base "${CMAKE_CURRENT_BINARY_DIR}/${target}-pytest")
    # get generator expression for full test name
    set(
        test_script_gen_name
        "${test_script_base}-$<IF:${PDMPMT_MULTI_CONFIG},$<CONFIG>,impl>.cmake"
    )
    # add post-build command on target to collect tests
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -DPython3_EXECUTABLE=${Python3_EXECUTABLE}
                -DTEST_ROOT=${ARG_FROM}
                # note: can't use $<TARGET_FILE_DIR:${target}> because target
                # might be a add_custom_target() target with no location
                -DTEST_SCRIPT_PATH=${test_script_gen_name}
                -P ${PROJECT_SOURCE_DIR}/cmake/pdmpmt_pytest_add_tests.cmake
        COMMENT "Collecting ${target} pytest tests"
        VERBATIM
    )
    # add main CTest script to include. for multi-config, warn if no -C config
    if(PDMPMT_MULTI_CONFIG)
        file(
            WRITE "${test_script_base}.cmake"
            "if(NOT CTEST_CONFIGURATION_TYPE)\n"
            "    message(\n"
            "        WARNING\n"
            "        \"No -C <config> specified for multi-config generator \"\n"
            "\"${CMAKE_GENERATOR}. ${target} pytest tests will not be run.\"\n"
            "    )\n"
            "else()\n"
            "    include(\"${test_script_base}-\${CTEST_CONFIGURATION_TYPE}.cmake\")\n"
            "endif()\n"
        )
    else()
        file(
            WRITE "${test_script_base}.cmake"
            "include(\"${test_script_base}-impl.cmake\")\n"
        )
    endif()
    # set directory property to include ${test_script_base}.cmake
    set_property(
        DIRECTORY APPEND
        PROPERTY TEST_INCLUDE_FILES "${test_script_base}.cmake"
    )
endfunction()

# only run pdmpmt_pytest_add_tests_impl() in script mode
if(CMAKE_SCRIPT_MODE_FILE)
    pdmpmt_pytest_add_tests_impl()
endif()
