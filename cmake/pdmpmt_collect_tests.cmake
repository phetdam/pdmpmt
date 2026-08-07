cmake_minimum_required(VERSION 3.20)

##
# pdmpmt_collect_tests.cmake
#
# Provides a function for registering custom tests with CTest from targets.
#
# This module provides similar functionality to gtest_discover_tests() for
# custom and executable targets that meet certain loose requirements, enabling
# their individual tests to be registered with CTest.
#

##
# Collect the custom tests and register them as CTest tests.
#
# This runs the test command with the test listing option and then writes
# add_test() commands to invoke each test individually in the correct working
# directory and with the properties specified, if any.
#
# Required variables:
#
#   TARGET_NAME             Name of the associated CMake target
#   TEST_COMMAND            List of strings forming the test command line
#   TEST_WORKING_DIR        Working directory to use when listing/running tests
#   TEST_LIST_OPTION        Option used to list all the test names
#   TEST_LIST_TIMEOUT       Timeout in seconds to use when listing tests
#   TEST_RUN_OPTION         Option used with test name to run a specified test
#   TEST_SCRIPT_PATH        Absolute path to write the resulting CMake script
#
# Optional variables:
#
#   TEST_PROPERTIES     List of test properties to set on each test
#
function(pdmpmt_collect_tests_impl)
    # check required variables
    if(NOT TARGET_NAME)
        message(FATAL_ERROR "Missing required CMake target name")
    endif()
    if(NOT TEST_COMMAND)
        message(FATAL_ERROR "Missing required test command line")
    endif()
    if(NOT TEST_WORKING_DIR)
        message(FATAL_ERROR "Missing required test working directory")
    endif()
    if(NOT TEST_LIST_OPTION)
        message(FATAL_ERROR "Missing required test list option")
    endif()
    if(NOT TEST_LIST_TIMEOUT)
        message(FATAL_ERROR "Missing required test listing timeout")
    endif()
    if(NOT TEST_RUN_OPTION)
        message(FATAL_ERROR "Missing required test run option")
    endif()
    if(NOT TEST_SCRIPT_PATH)
        message(FATAL_ERROR "Missing rquired final CMake script path")
    endif()
    # list all the tests
    execute_process(
        COMMAND ${TEST_COMMAND} ${TEST_LIST_OPTION}
        WORKING_DIRECTORY "${TEST_WORKING_DIR}"
        TIMEOUT ${TEST_LIST_TIMEOUT}
        RESULT_VARIABLE test_list_res
        OUTPUT_VARIABLE test_list
        ERROR_VARIABLE test_list
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )
    if(test_list_res)
        message(FATAL_ERROR "Error collecting tests: ${test_list}")
    endif()
    # convert test_list to a list
    string(REPLACE "\n" ";" test_list "${test_list}")
    # add tests
    # note: CTest implements minimal add_tests() and set_tests_properties()
    foreach(test_name ${test_list})
        string(
            APPEND test_script_content
"add_test(\n"
"    \"${TARGET_NAME}.${test_name}\"\n"
"    \"${CMAKE_COMMAND}\"\n"
"        -E chdir \"${TEST_WORKING_DIR}\"\n"
"        ${TEST_COMMAND} ${TEST_RUN_OPTION} ${test_name}\n"
")\n"
        )
        # set any properties
        if(TEST_PROPERTIES)
            string(
                APPEND test_script_content
"set_tests_properties(\n"
"    \"${TARGET_NAME}.${test_name}\" PROPERTIES\n"
"    ${TEST_PROPERTIES}\n"
")\n"
            )
        endif()
    endforeach()
    # write test list to file
    file(WRITE "${TEST_SCRIPT_PATH}" "${test_script_content}")
endfunction()

##
# Add a post-build command to collect custom tests from a target.
#
# This function requires an executable or custom target that can be queried for
# its list of tests and also supports running each test individually. Any
# compatible target's command must accept command-line options list_opt and
# run_opt that can be used in the following manner:
#
#   list_opt        Prints the list of test names one name per line
#   run_opt TEST    Runs the specified test name
#
# By default, list_opt is -l and run_opt is -t, but these can be respectively
# changed using the LIST_OPTION and RUN_OPTION arguments. Furthermore, if the
# target is executable, the default test command will use the target file name,
# but custom targets must specify their own command line.
#
# Arguments:
#   target                  Custom or executable target
#
#   [COMMAND cmd...]        Command to invoke the test driver. If not specified
#                           then ${target} is used, so if the target a custom
#                           target, then COMMAND is required.
#
#   [LIST_OPTION opt]       Command-line option for printing the list of tests
#   [RUN_OPTION opt]        Command-line option used to specify a single test
#
#   [PROPERTIES prop1 value1...]
#                           List of properties to set on each test
#
#   [WORKING_DIRECTORY dir]
#                           Working directory to use when listing tests. By
#                           default ${CMAKE_CURRENT_BINARY_DIR} is used.
#
#   [TIMEOUT timeout]       Timeout in seconds when listing tests (default 10)
#
function(pdmpmt_collect_tests target)
    # target must exist
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Target ${target} does not exist")
    endif()
    # parse arguments
    cmake_parse_arguments(
        ARG
        ""
        "LIST_OPTION;RUN_OPTION;WORKING_DIRECTORY;TIMEOUT"
        "COMMAND;PROPERTIES"
        ${ARGN}
    )
    # if target is executable COMMAND defaults to target file else required
    get_target_property(target_type ${target} TYPE)
    if(NOT ARG_COMMAND)
        if(target_type STREQUAL "EXECUTABLE")
            set(ARG_COMMAND "$<TARGET_FILE:${target}>")
        else()
            message(FATAL_ERROR "Non-executable target requires COMMAND")
        endif()
    endif()
    # option defaults
    if(NOT ARG_LIST_OPTION)
        set(ARG_LIST_OPTION -l)
    endif()
    if(NOT ARG_RUN_OPTION)
        set(ARG_RUN_OPTION -t)
    endif()
    if(NOT ARG_WORKING_DIRECTORY)
        set(ARG_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    if(NOT ARG_TIMEOUT)
        set(ARG_TIMEOUT 10)
    endif()
    # if properties are provided the list must be of even length
    if(ARG_PROPERTIES)
        list(LENGTH ARG_PROPERTIES props_len)
        math(EXPR props_trunc_len "2 * (${props_len} / 2)")  # truncated
        if(NOT props_len EQUAL props_trunc_len)
            message(FATAL_ERROR "PROPERTIES must have even length")
        endif()
    endif()
    # base name for CTest script to include with TEST_INCLUDE_FILES
    set(test_script_base "${CMAKE_CURRENT_BINARY_DIR}/${target}_tests")
    # indicate if multi-config generator
    get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    # create full name using generator expression based on generator type
    set(
        test_script_gen_name
        "${test_script_base}-$<IF:${multi_config},$<CONFIG>,impl>.cmake"
    )
    # add post-build command to run collection
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -DTARGET_NAME=${target}
                -DTEST_COMMAND=${ARG_COMMAND}
                -DTEST_WORKING_DIR=${ARG_WORKING_DIRECTORY}
                -DTEST_LIST_OPTION=${ARG_LIST_OPTION}
                -DTEST_LIST_TIMEOUT=${ARG_TIMEOUT}
                -DTEST_RUN_OPTION=${ARG_RUN_OPTION}
                -DTEST_PROPERTIES=${ARG_PROPERTIES}
                -DTEST_SCRIPT_PATH=${test_script_gen_name}
                -P ${PROJECT_SOURCE_DIR}/cmake/pdmpmt_collect_tests.cmake
        VERBATIM
    )
    # main CTest script to include
    if(multi_config)
        file(
            WRITE "${test_script_base}.cmake"
"if(NOT CTEST_CONFIGURATION_TYPE)\n"
"    message(\n"
"        FATAL_ERROR\n"
"        \"-C <config> required for multi-config generator ${CMAKE_GENERATOR}\"\n"
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
    # set directory property to ensure ${test_script_base}.cmake is included
    set_property(
        DIRECTORY APPEND
        PROPERTY TEST_INCLUDE_FILES "${test_script_base}.cmake"
    )
endfunction()

# run implementation in script mode
if(CMAKE_SCRIPT_MODE_FILE)
    pdmpmt_collect_tests_impl()
endif()
