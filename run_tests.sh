#!/usr/bin/env bash
#
# Run all tests registered with CMake using ctest.

##
# Main function.
#
main() {
    # GTEST_COLOR=yes allows color output from Google test mains. the nested
    # command substitution gets half the number of available threads on system.
    # note that only the final -j<n_procs> option takes effect.
    GTEST_COLOR=yes ctest --test-dir build -j$(expr $(nproc --all) / 2) "$@"
}

set -e
main "$@"
