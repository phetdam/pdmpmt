:: C++ build script for pdmpmt.
::
:: Author: Derek Huang
:: Copyright: MIT License
::

@echo off
setlocal EnableDelayedExpansion

:: program name, as any label `call`ed uses label name as %0
set PROGNAME=%0
:: active build configuration, defaults to Debug
set BUILD_CONFIG=Debug
:: current action to take, argument parsing mode
set BUILD_ACTION=
set PARSE_ACTION=

call :Main %*
exit /b !ERRORLEVEL!

::::
:: Print help output.
::
:: Note: ^ is used to escape some characters with special meaning, and can be
:: also used for line continuation. ! must be double-escaped, however.
::
:PrintHelp
    echo Usage: %PROGNAME% [-h] [-Ca CMAKE_ARGS] [-Cb CMAKE_BUILD_ARGS]
    echo.
    echo Build the pdmpmt C++ binaries.
    echo.
    echo Uses the default Visual Studio generator and toolset.
    echo.
    echo Arguments of the form KEY^=VALUE must be double quoted, otherwise the
    echo KEY and VALUE will be split into separate arguments.
    echo.
    echo Options:
    echo   -h,  --help               Print this usage
    echo   -c,  --config CONFIG      Build configuration, default %BUILD_CONFIG%
    echo   -Ca, --cmake-args CMAKE_ARGS
    echo                             Args to pass to cmake config command
    echo.
    echo   -Cb, --cmake-build-args CMAKE_BUILD_ARGS
    echo                             Args to pass to cmake build command
exit /b 0

::::
:: Parse incoming command-line arguments.
::
:: Sets the CMAKE_ARGS and CMAKE_BUILD_ARGS global variables.
::
:: Arguments:
::  List of command-line arguments
::
:ParseArgs
:: note that using "::" as comment can cause cmd to misinterpret tokens as
:: drive names when you have "::" inside the if statements. use rem instead.
:: however, rem tends to be much slower.
::
:: see https://stackoverflow.com/a/12407934/14227825 and other answers.
::
:: we break early if the help flag is encountered.
::
for %%A in (%*) do (
    if %%A==-h (
        set BUILD_ACTION=print_help
        exit /b 0
    ) else (
        if %%A==--help (
            set BUILD_ACTION=print_help
            exit /b 0
        ) else (
            if %%A==-c (
                set PARSE_ACTION=parse_config
            ) else (
                if %%A==--config (
                    set PARSE_ACTION=parse_config
                ) else (
                    if %%A==-Ca (
                        set PARSE_ACTION=parse_cmake_args
                    ) else (
                        if %%A==--cmake-args (
                            set PARSE_ACTION=parse_cmake_args
                        ) else (
                            if %%A==-Cb (
                                set PARSE_ACTION=parse_cmake_build_args
                            ) else (
                                if %%A==--cmake-build-args (
                                    set PARSE_ACTION=parse_cmake_build_args
                                ) else (
                                    if !PARSE_ACTION!==parse_config (
                                        set "BUILD_CONFIG=%%A"
                                    ) else (
    if !PARSE_ACTION!==parse_cmake_args (
        if not defined CMAKE_ARGS (
            set "CMAKE_ARGS=%%A"
        ) else (
            set "CMAKE_ARGS=!CMAKE_ARGS! %%A"
        )
    ) else (
        if !PARSE_ACTION!==parse_cmake_build_args (
            if not defined CMAKE_BUILD_ARGS (
                set "CMAKE_BUILD_ARGS=%%A"
            ) else (
                set "CMAKE_BUILD_ARGS=!CMAKE_BUILD_ARGS! %%A"
            )
        ) else (
            echo Error: Unknown arg %%A, try %PROGNAME% --help for usage
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
        )
    )
)
exit /b 0

::::
:: Main function for the build script.
::
:: Arguments:
::  Array of command-line arguments
::
:Main
:: separate incoming args into those for cmake, cmake --build. note that the
:: only way to preserve literal "=" is to just accept all the args.
call :ParseArgs %*
:: print help and exit if BUILD_ACTION is print_help
if !BUILD_ACTION!==print_help (
    call :PrintHelp
    exit /b !ERRORLEVEL!
)
:: otherwise, proceed with our build. must use x64 to support CUDA
:: TODO: allow user specification of build prefix
cmake -S . -B build_windows -A x64 !CMAKE_ARGS!
cmake --build build_windows --config !BUILD_CONFIG! -j !CMAKE_BUILD_ARGS!
exit /b !ERRORLEVEL!
