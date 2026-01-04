cmake_minimum_required(VERSION 3.20)

##
# pdmpmt_find_pip_packages.cmake
#
# Provides a function to locate pip packages given requirements specifiers.
#
# This requires that Python has already been found.
#

##
# Match a package spec against a list of pip freeze version requirements.
#
# On completion the output variables for the package name and whether or not
# the package was found are updated. The package version output variable is
# updated only if the package spec was matched against the pip packages.
#
# Arguments:
#                               pdmpmt_find_pip_packages()
#   spec                        Package spec for pip as described in
#
#   [REQUIRED]                  Emit fatal error if package spec is not matched
#   PIP_PACKAGES pkgs...        List of package==version pip freeze entries
#   PACKAGE_NAME name           Output variable with package name as in spec
#   PACKAGE_FOUND found         Output variable with TRUE/FALSE find result
#   PACKAGE_VERSION version     Output variable with package version
#
function(pdmpmt_match_pip_requirement spec)
    # single and multi-argument keywords
    set(single_args PACKAGE_NAME PACKAGE_FOUND PACKAGE_VERSION)
    set(multi_args PIP_PACKAGES)
    # parse arguments
    cmake_parse_arguments(ARG "REQUIRED" "${single_args}" "${multi_args}" ${ARGN})
    # check variables
    foreach(var ${single_args} ${multi_args})
        if(NOT DEFINED ARG_${var})
            message(FATAL_ERROR "{var} required")
        endif()
    endforeach()
    # variable to be STATUS or FATAL_ERROR as necessary
    if(ARG_REQUIRED)
        set(status_or_error FATAL_ERROR)
    else()
        set(status_or_error STATUS)
    endif()
    # get package name from spec
    string(REGEX REPLACE "^([0-9a-zA-Z_-]+).*" "\\1" pkg_name "${spec}")
    if(NOT pkg_name)
        message(FATAL_ERROR "Unable to get package name from spec ${spec}")
    endif()
    # get version requirements list separate from package name.
    string(REGEX REPLACE "^[0-9a-zA-Z_-]+(.*)" "\\1" version_reqs "${spec}")
    # if we can't get version requirements, only an error if spec and package
    # name are equal (spec only has package name, no version reqs)
    if(NOT version_reqs AND NOT pkg_name STREQUAL spec)
        message(
            FATAL_ERROR
            "Unable to get version requirements from spec ${spec}"
        )
    endif()
    # convert to list properly
    string(REPLACE "," ";" version_reqs "${version_reqs}")
    # search for package name in pip package list
    set(pkg_search "${ARG_PIP_PACKAGES}")
    list(FILTER pkg_search INCLUDE REGEX "^${pkg_name}==.+")
    if(NOT pkg_search)
        message(
            ${status_or_error}
            "Could NOT find ${pkg_name} in list of package requirements"
        )
        # print, set, return
        message(STATUS "${pkg_name} version: None")
        set(${ARG_PACKAGE_NAME} "${pkg_name}" PARENT_SCOPE)
        set(${ARG_PACKAGE_FOUND} FALSE PARENT_SCOPE)
        return()
    endif()
    # get package version from pkg_search
    string(REGEX REPLACE "^${pkg_name}==(.+)" "\\1" pkg_version "${pkg_search}")
    if(NOT pkg_version)
        message(
            FATAL_ERROR
            "Unable to get package version from ${pkg_name} search"
        )
    endif()
    # ok, managed to find package. parse version specs and check against each
    foreach(req ${version_reqs})
        # get operator
        string(REGEX REPLACE "([>=<]+).+" "\\1" ver_op "${req}")
        # get CMake if() version comparison operator + reverse operator
        if(ver_op STREQUAL ">")
            set(ver_cmp VERSION_GREATER)
            set(ver_op_neg "<=")
        elseif(ver_op STREQUAL ">=")
            set(ver_cmp VERSION_GREATER_EQUAL)
            set(ver_op_neg "<")
        elseif(ver_op STREQUAL "<")
            set(ver_cmp VERSION_LESS)
            set(ver_op_neg ">=")
        elseif(ver_op STREQUAL "<=")
            set(ver_cmp VERSION_LESS_EQUAL)
            set(ver_op_neg ">")
        elseif(ver_op STREQUAL "==")
            set(ver_cmp VERSION_EQUAL)
            set(ver_op_neg "!=")
        else()
            message(
                FATAL_ERROR
                "Invalid version requirement ${req} for spec ${spec}"
            )
        endif()
        # get actual version in question
        string(REGEX REPLACE "[>=<]+(.+)" "\\1" version "${req}")
        if(NOT version)
            message(FATAL_ERROR "Missing version in package spec ${spec}")
        endif()
        # check again pkg_version that was found. if failed, handle
        if(NOT pkg_version ${ver_cmp} version)
            message(
                ${status_or_error}
                "Could NOT find ${pkg_name} (required ${req} but found "
"${pkg_version})"
            )
            # print, set, return
            message(STATUS "${pkg_name} version: None")
            set(${ARG_PACKAGE_NAME} "${pkg_name}" PARENT_SCOPE)
            set(${ARG_PACKAGE_FOUND} FALSE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    # if all matched, print, set variables, and return
    message(STATUS "${pkg_name} version: ${pkg_version}")
    set(${ARG_PACKAGE_NAME} "${pkg_name}" PARENT_SCOPE)
    set(${ARG_PACKAGE_FOUND} TRUE PARENT_SCOPE)
    set(${ARG_PACKAGE_VERSION} "${pkg_version}" PARENT_SCOPE)
    return()
endfunction()

##
# Check that the specified packages have been installed with pip.
#
# On completion, the function sets ${pkg}_PIP_FOUND to TRUE for each package
# ${pkg} found, FALSE otherwise. On success, the version is defined:
#
#   ${pkg}_PIP_VERSION          ${pkg} version reported by pip
#
# Each package specifier should be a subset of the Python requirements specs:
#
#   spec = package-name | package-name version-specs
#   package-name = [0-9a-zA-Z_-]+
#   version-specs = version-spec | version-specs "," version-spec
#   version-spec = ( ">" | ">=" | "<" | "<=" | "==" ) version
#   version = [0-9.]+ | [0-9.]+ "+" .+
#
# For example, on valid invocation could look like this:
#
#   pdmpmt_find_pip_packages(
#       REQUIRED
#           dask>=2025.2
#           distributed>=2025.2
#           pytest>=8
#       OPTIONAL
#           ray-cpp>2.30
#           PyYAML               # note: any version allowed
#   )
#
# This function supersedes the original pdmpmt_find_pip_package() function
# which linearly increases the CMake configuration time per invocation due to
# invoking pip one time for each package being looked up.
#
# Arguments:
#   [REQUIRED spec...]      Package specs for required packages
#   [OPTIONAL spec...]      Package specs for optional packages
#
function(pdmpmt_find_pip_packages)
    # parse arguments
    # note: allow both lists to be undefined or empty
    cmake_parse_arguments(ARG "" "" "REQUIRED;OPTIONAL" ${ARGN})
    # get binary directory of Python executable (pip in same directory)
    cmake_path(GET Python3_EXECUTABLE PARENT_PATH py_binary_dir)
    # invoke pip freeze to get list of requirements
    execute_process(
        COMMAND "${py_binary_dir}/pip" freeze
        RESULT_VARIABLE pip_freeze_res
        ERROR_VARIABLE pip_freeze_err
        OUTPUT_VARIABLE pip_packages
        ERROR_STRIP_TRAILING_WHITESPACE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # failed
    if(pip_freeze_res)
        message(
            WARNING
            "${py_binary_dir}/pip freeze failed:\n${pip_freeze_err}\n"
"No pip package detection will be done."
        )
        return()
    endif()
    # clean up execute_process variables
    unset(pip_freeze_res)
    unset(pip_freeze_err)
    # convert output into list and keep only package==version lines
    string(REPLACE "\n" ";" pip_packages "${pip_packages}")
    list(FILTER pip_packages INCLUDE REGEX "^[0-9a-zA-Z_-]+==.+")
    # for each required package
    foreach(spec ${ARG_REQUIRED})
        # check package spec
        pdmpmt_match_pip_requirement(
            ${spec} REQUIRED
            PIP_PACKAGES ${pip_packages}
            PACKAGE_NAME pkg_name
            PACKAGE_FOUND pkg_found
            PACKAGE_VERSION pkg_version
        )
        # push variables to parent scope
        set(${pkg_name}_PIP_FOUND ${pkg_found} PARENT_SCOPE)
        set(${pkg_name}_PIP_VERSION ${pkg_version} PARENT_SCOPE)
    endforeach()
    # for each optional package
    foreach(spec ${ARG_OPTIONAL})
        # check package spec
        pdmpmt_match_pip_requirement(
            ${spec}
            PIP_PACKAGES ${pip_packages}
            PACKAGE_NAME pkg_name
            PACKAGE_FOUND pkg_found
            PACKAGE_VERSION pkg_version
        )
        # push variables to parent scope
        set(${pkg_name}_PIP_FOUND ${pkg_found} PARENT_SCOPE)
        if(pkg_found)
            set(${pkg_name}_PIP_VERSION ${pkg_version} PARENT_SCOPE)
        endif()
    endforeach()
endfunction()
