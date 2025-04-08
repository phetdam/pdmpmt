cmake_minimum_required(VERSION 3.15)

##
# pdmpmt_find_pip_package.cmake
#
# Provides a function to locate a pip package.
#
# This requires that Python has already been found.
#

##
# Check that the given Python package has been installed with pip.
#
# On completion, the function sets $[pkg}_PIP_FOUND to TRUE if the package was
# found, FALSE otherwise. On success, the version is defined:
#
#   ${pkg}_PIP_VERSION          ${pkg} version reported by pip
#
# Arguments:
#   pkg                         Package name for pip, e.g. dask
#   [REQUIRED]                  Error if package is not found
#   [MIN_VERSION min_ver]       Minimum package version
#   [MAX_VERSION max_ver]       Maximum package version
#
function(pdmpmt_find_pip_package pkg)
    cmake_parse_arguments(ARG "REQUIRED" "MIN_VERSION;MAX_VERSION" "" ${ARGN})
    # if Python 3 not found, not found
    execute_process(
        COMMAND pip show ${pkg}
        RESULT_VARIABLE show_${pkg}_res
        ERROR_VARIABLE show_${pkg}_errout
        OUTPUT_VARIABLE show_${pkg}_output
        ERROR_STRIP_TRAILING_WHITESPACE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # failed
    if(show_${pkg}_res)
        set(${pkg}_PIP_FOUND FALSE PARENT_SCOPE)
        if(ARG_REQUIRED)
            message(
                FATAL_ERROR
                "Could NOT find ${pkg} using pip:\n${show_${pkg}_errout}"
            )
        endif()
        message(STATUS "${show_${pkg}_errout}")
        message(STATUS "${pkg} version: None")
        return()
    endif()
    # otherwise, ${pkg} has been located by pip
    # convert show_${pkg}_output to list and keep only version info
    string(REPLACE "\n" ";" show_${pkg}_output "${show_${pkg}_output}")
    list(FILTER show_${pkg}_output INCLUDE REGEX "Version:")
    # convert into version
    string(
        REGEX REPLACE "Version:[ ]+" ""
        ${pkg}_version "${show_${pkg}_output}"
    )
    # ensure min version is ok
    if(ARG_MIN_VERSION AND ARG_MIN_VERSION VERSION_GREATER ${pkg}_version)
        set(_msg "${pkg} version ${${pkg}_version} < minimum ${ARG_MIN_VERSION}")
        if(ARG_REQUIRED)
            message(FATAL_ERROR "${_msg}")
        endif()
        message(STATUS "${_msg}")
        set(${pkg}_PIP_FOUND FALSE PARENT_SCOPE)
        message(STATUS "${pkg} version: None")
        return()
    endif()
    # ensure max version is ok
    if(ARG_MAX_VERSION AND ARG_MAX_VERSION VERSION_LESS ${pkg}_version)
        set(_msg "${pkg} version ${${pkg}_version} > maximum ${ARG_MAX_VERSION}")
        if(ARG_REQUIRED)
            message(FATAL_ERROR "${_msg}")
        endif()
        message(STATUS "${_msg}")
        set(${pkg}_PIP_FOUND FALSE PARENT_SCOPE)
        message(STATUS "${pkg} version: None")
        return()
    endif()
    # set variables in parent scope to finish
    set(${pkg}_PIP_FOUND TRUE PARENT_SCOPE)
    set(${pkg}_PIP_VERSION ${${pkg}_version} PARENT_SCOPE)
    message(STATUS "${pkg} version: ${${pkg}_version}")
endfunction()
