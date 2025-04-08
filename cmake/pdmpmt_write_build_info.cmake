cmake_minimum_required(VERSION 3.15)

##
# pdmpmt_write_build_info.cmake
#
# Write a build info string based off of a Git hash.
#

##
# Writes the build info string to the given output variable.
#
# If Git is available, the Git build hash is used, otherwise DEV is used.
#
# Arguments:
#   var     Name of variable to write build info to
#
function(pdmpmt_write_build_info var)
    # Git not found, use DEV
    if(NOT Git_FOUND)
        set(${var} DEV PARENT_SCOPE)
        return()
    endif()
    # otherwise, use Git
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        RESULT_VARIABLE rev_parse_res
        ERROR_VARIABLE rev_parse_errout
        OUTPUT_VARIABLE rev_parse_out
        ERROR_STRIP_TRAILING_WHITESPACE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # failed
    if(rev_parse_res)
        message(FATAL_ERROR "git rev-parse failed:\n${rev_parse_errout}")
    endif()
    # otherwise, done
    set(${var} ${rev_parse_out} PARENT_SCOPE)
endfunction()
