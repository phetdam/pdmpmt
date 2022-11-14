#!/usr/bin/env bash
#
# Simple CMake wrapper script to build the CMake targets in the project.
# Any arguments before --build-args are passed directly to cmake --build.

# arguments passed to cmake command directly
CMAKE_ARGS=()
# arguments passed to cmake --build command directly
CMAKE_BUILD_ARGS=()
# indicate current argument parsing mode
PARSE_MODE=cmake_args

##
# Collect incoming arguments, separating them for cmake, cmake --build.
#
# Sets the CMAKE_ARGS and CMAKE_BUILD_ARGS global variables.
#
# Arguments:
#   Array of command-line arguments
#       Any arguments after --build-args are passed to cmake --build while the
#       rest preceding will be passed to the cmake configure command.
#
collect_args() {
    for ARG in "$@"
    do
        # break early if -h or --help flag is introduced
        if [ "$ARG" = -h ] || [ "$ARG" = --help ]
        then
            PARSE_MODE=print_help
            return 0
        elif [ "$ARG" = --build-args ]
        then
            PARSE_MODE=cmake_build_args
        elif [ $PARSE_MODE = cmake_build_args ]
        then
            CMAKE_BUILD_ARGS+=("$ARG")
        else
            CMAKE_ARGS+=("$ARG")
        fi
    done
}

##
# Print help output for this script.
#
print_help() {
    echo "Usage: $0 [ARG ...] [--build-args ARG [ARG ...]]"
    echo
    echo "Build the C++ library and examples by using CMake."
    echo
    echo "Arguments:"
    echo "  ARG ...                     args passed to cmake command"
    echo "  --build-args ARG [ARG ...]  args passed to cmake --build command"
    echo "  -h,--help                   show this help message and exit"
    echo
}

##
# Main function for the build script.
#
main() {
    # separate incoming args into those for cmake, cmake --build
    collect_args "$@"
    # print help and exit if PARSE_MODE is print_help
    if [ $PARSE_MODE = print_help ]
    then
        print_help
        return 0
    fi
    cmake -S . -B build ${CMAKE_ARGS[@]}
    # the last -j specification will override
    cmake --build build -j ${CMAKE_BUILD_ARGS[@]}
}

set -e
main "$@"
