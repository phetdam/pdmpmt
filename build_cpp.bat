:: Simple CMake wrapper script to build the CMake targets in the project.
:: Any arguments before --build-args are passed directly to cmake --build.
::
:: Note: any -D<var>=<value> arguments must be double-quoted!

@echo off
setlocal EnableDelayedExpansion

:: arguments passed to cmake command directly
set CMAKE_ARGS=
:: arguments passed to cmake --build command directly
set CMAKE_BUILD_ARGS=
:: program name, as any label `call`ed uses label name as %0
set PROGNAME=%0
:: indicate current argument parsing mode
set PARSE_MODE=cmake_args

call :Main %*
exit /b !ERRORLEVEL!

::::
:: Collect incoming arguments, separating them for cmake, cmake --build.
::
:: Sets the CMAKE_ARGS and CMAKE_BUILD_ARGS global variables.
::
:: Arguments:
::  Array of command-line arguments
::      Any arguments after --build-args are passed to cmake --build while the
::      rest preceding will be passed to the cmake configure command.
::
:CollectArgs
:: note that using "::" as comment can cause cmd to misinterpret tokens as
:: drive names when you have "::" inside the if statements. use rem instead.
:: however, rem tends to be much slower.
::
:: see https://stackoverflow.com/a/12407934/14227825 and other answers.
::
:: we break early if the help flag is encountered and set PARSE_MODE.
::
for %%A in (%*) do (
    if %%A==--help (
        set PARSE_MODE=print_help
        exit /b 0
    ) else (
        if %%A==--build-args (
            set PARSE_MODE=cmake_build_args
        ) else (
            if !PARSE_MODE!==cmake_build_args (
                if not defined CMAKE_BUILD_ARGS (
                    set CMAKE_BUILD_ARGS=%%A
                ) else (
                    set CMAKE_BUILD_ARGS=!CMAKE_BUILD_ARGS! %%A
                )
            ) else (
                if not defined CMAKE_ARGS (
                    set CMAKE_ARGS=%%A
                ) else (
                    set CMAKE_ARGS=!CMAKE_ARGS! %%A
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
:: print help blurb if necessary
:: separate incoming args into those for cmake, cmake --build. note that the
:: only way to preserve literal "=" is to just accept all the args.
call :CollectArgs %*
:: print help and exit if PARSE_MODE is print_help
if !PARSE_MODE!==print_help (
    call :PrintHelp
    exit /b 0
)
cmake -G Ninja -S . -B build_windows !CMAKE_ARGS!
cmake --build build_windows !CMAKE_BUILD_ARGS!
exit /b !ERRORLEVEL!

::::
:: Print help output.
::
:: Note: ^ is used to escape some characters with special meaning, and can be
:: also used for line continuation. ! must be double-escaped, however.
::
:PrintHelp
echo Usage: %PROGNAME% [ARG ...] [--build-args BUILD_ARG [BUILD_ARG ...]]
echo.
echo Build the C++ library and examples by using CMake.
echo.
echo The default generator used is Ninja, so to CMAKE_BUILD_TYPE should be used
echo to specify the build type, ex. Debug or Release.
echo.
echo Note that any -D^<var^>=^<value^> arguments must be double-quoted^^!
echo.
echo Arguments:
echo   ARG ...                     args passed to cmake command
echo   --build-args ARG [ARG ...]  args passed to cmake --build command
echo   --help                      show this help message and exit
exit /b 0

endlocal
echo on
