:: Run all tests registered with CMake using ctest.

@echo off
setlocal

call :Main %*
exit /b %ERRORLEVEL%

::::
:: Collect command-line arguments.
::
:: Arguments:
::  Array of command-line arguments
:CollectArgs
exit /b 0

::::
:: Main method for the script.
::
:: Note: In CMD GTEST_COLOR does not work with ctest. However, color output is
:: still produced if the Google Test runner is executed directly.
::
:: Arguments:
::  Array of command-line arguments
::
:Main
:: use half the number of processors. only final -j<n_procs> takes effect.
set /a N_PROCS=%NUMBER_OF_PROCESSORS% / 2
ctest --test-dir build_windows -j%N_PROCS% %*
exit /b %ERRORLEVEL%

endlocal
echo on
