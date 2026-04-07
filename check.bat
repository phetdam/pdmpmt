:: CTest test driver script for pdmpmt.
::
:: Author: Derek Huang
:: Copyright: MIT License
::

@echo off
setlocal EnableDelayedExpansion

:: program name (called labels use label names as %0)
set PROGNAME=%0
:: ensure that GoogleTest mains run with color
set GTEST_COLOR=yes
:: default build config
set BUILD_CONFIG=Debug
:: current action to take, argument parsing mode
set TEST_ACTION=
set PARSE_ACTION=

call :Main %*
exit /b !ERRORLEVEL!

:: Print script usage.
:PrintUsage
echo Usage: %PROGNAME% [-h] [-c CONFIG] [-Ct CTEST_ARGS]
echo.
echo CTest driver script for pdmpmt.
echo.
echo Currently supports only "build_windows" as the output prefix and x64 as
echo the architecture for a build directory "build_windows_x64".
echo.
echo Options:
echo   -h, --help               Print this usage
echo   -c, --config CONFIG      Build configuration, default %BUILD_CONFIG%
echo.
echo   -Ct, --ctest-args CTEST_ARGS
echo                            Additional arguments for CTest
exit /b 0

:: Parse incoming command-line arguments.
::
:: Arguments:
::  %*    Command-line arguments
::
:ParseArgs
:: loop arguments
for %%A in (%*) do (
    if %%A==-h (
        set TEST_ACTION=print_usage
        exit /b 0
    ) else (
        if %%A==--help (
            set TEST_ACTION=print_usage
            exit /b 0
        ) else (
            if %%A==-c (
                set PARSE_ACTION=parse_config
            ) else (
                if %%A==--config (
                    set PARSE_ACTION=parse_config
                ) else (
                    if %%A==-Ct (
                        set PARSE_ACTION=parse_ctest_args
                    ) else (
                        if %%A==--ctest-args (
                            set PARSE_ACTION=parse_ctest_args
                        ) else (
if !PARSE_ACTION!==parse_config (
    set "BUILD_CONFIG=%%A"
) else (
    if !PARSE_ACTION!==parse_ctest_args (
        if not defined CTEST_ARGS (
            set "CTEST_ARGS=%%A"
        ) else (
            set "CTEST_ARGS=!CTEST_ARGS! %%A"
        )
    ) else (
        echo Error: Unknown argument %%A. Try %PROGNAME% --help for usage.
        exit /b 1
    )
)
                        )
                    )
                )
            )
        )
    )
)
exit /b !ERRORLEVEL!

:: Main entry point.
::
:: Arguments:
::  %*    Additional arguments to pass to CTest
::
:Main
:: parse arguments
call :ParseArgs %*
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
:: print usage if specified and exit
if !TEST_ACTION!==print_usage (
    call :PrintUsage
    exit /b 0
)
:: run CTest tests
ctest --test-dir build_windows_x64 -C !BUILD_CONFIG! -j%NUMBER_OF_PROCESSORS% ^
    !CTEST_ARGS!
exit /b 0
