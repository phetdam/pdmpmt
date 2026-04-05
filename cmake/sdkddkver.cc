/**
 * @file sdkddkver.cc
 * @author Derek Huang
 * @brief C++ program to print the default defined `_WIN32_WINNT` value
 * @copyright MIT License
 */

// note: no need to include Windows.h to use sdkddkver.h
#include <sdkddkver.h>

#include <cstdlib>
#include <iostream>

// stringification macros. we want to expand _WIN32_WINNT's hex value
#define STRINGIFY_I(x) #x
#define STRINGIFY(x) STRINGIFY_I(x)

int main()
{
  std::cout << STRINGIFY(_WIN32_WINNT) << std::endl;
  return EXIT_SUCCESS;
}
