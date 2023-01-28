cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# get extensionless file names of the GSL and GSL CBLAS DLLs
get_filename_component(PDMPMT_GSL_STEM ${GSL_LIBRARY} NAME_WE)
get_filename_component(PDMPMT_GSL_CBLAS_STEM ${GSL_CBLAS_LIBRARY} NAME_WE)
# get list of GSL DLLs used for this particular project build
set(
    PDMPMT_GSL_DLL_LIST
    "${GSL_ROOT_DIR}/bin/${CMAKE_BUILD_TYPE}/${PDMPMT_GSL_STEM}.dll;\
${GSL_ROOT_DIR}/bin/${CMAKE_BUILD_TYPE}/${PDMPMT_GSL_CBLAS_STEM}.dll"
)

##
# Add a post-build action copying GSL DLLs to target output location.
#
# Has no effect on non-Windows platforms or if PDMPMT_GSL_SHARED is OFF/0. Uses
# copy_if_different to avoid repeated duplicate DLL copying.
#
# Arguments:
#   target_name - Name of CMake target to add this post-build action to
#
function(copy_gsl_dlls_post_build target_name)
    # warn if using static GSL libraries
    if(NOT WIN32)
        return()
    endif()
    if(NOT PDMPMT_GSL_SHARED)
        message(
            WARNING
            "${CMAKE_CURRENT_FUNCTION}(${target_name}) is a no-op when using \
static GSL libraries"
        )
        return()
    endif()
    # otherwise add post-build DLL copy command for GSL DLLs
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
            ${PDMPMT_GSL_DLL_LIST} $<TARGET_FILE_DIR:${target_name}>
        COMMAND_EXPAND_LISTS
    )
endfunction()
