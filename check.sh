#!/usr/bin/bash
#
# CTest driver script for pdmpmt.
#
# Author: Derek Huang
# Copyright: MIT License
#

##
# Main entry point.
#
# Arguments:
#   $@    Additional arguments to pass to CTest
#
main() {
    # GTEST_COLOR=yes allows color output from GoogleTest mains
    GTEST_COLOR=yes ctest --test-dir build -j$(nproc) "$@"
}

set -e
main "$@"
