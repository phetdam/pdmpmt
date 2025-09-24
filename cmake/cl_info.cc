/**
 * @file cl_info.cc
 * @author Derek Huang
 * @brief C++ program to print OpenCL platform names + versions
 * @copyright MIT License
 *
 * This is a much simplified version of `opencl_info` that is run during CMake
 * build configuration to spit out some platform information. It therefore does
 * not use the useful abstractions from `opencl.hh` that are preferred.
 */

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <CL/opencl.h>

namespace {

/**
 * OpenCL error handler.
 *
 * This is used with an `operator<<` overload to report and exit on error.
 */
struct opencl_error_handler {};

// error handling global
constexpr opencl_error_handler errh;

/**
 * Check the return code of an OpenCL function call and exit on error.
 */
void operator<<(opencl_error_handler, cl_int res)
{
// error case helper macro
#define ERROR_CASE(err) \
  case err: \
    std::cerr << "Error: " << #err << std::endl; \
    std::exit(EXIT_FAILURE);
  // handle return values
  switch (res) {
  case CL_SUCCESS:
    break;
  ERROR_CASE(CL_OUT_OF_HOST_MEMORY)
  ERROR_CASE(CL_INVALID_VALUE)
  ERROR_CASE(CL_INVALID_PLATFORM)
#ifdef cl_khr_icd
  ERROR_CASE(CL_PLATFORM_NOT_FOUND_KHR)
#endif  // cl_khr_icd
  default:
    std::cerr << "Error: Unknown OpenCL error " << res << std::endl;
    std::exit(EXIT_FAILURE);
  }
#undef ERROR_CASE
}

}  // namespace

int main()
{
  // get platform ID count
  cl_uint n_plats;
  errh << clGetPlatformIDs(0u, nullptr, &n_plats);
  // get platform IDs
  std::vector<cl_platform_id> plats(n_plats);
  errh << clGetPlatformIDs(n_plats, plats.data(), nullptr);
  // enumerate platforms
  std::cout << "OpenCL platforms:\n";
  for (auto plat : plats) {
    // get platform name
    auto name = [plat]
    {
      // platform name length including null terminator
      std::size_t len;
      errh << clGetPlatformInfo(plat, CL_PLATFORM_NAME, 0u, nullptr, &len);
      // allocate string of NULs
      std::string str(len - 1u, '\0');
      // write null-terminated characters + return
      // note: defined behavior in C++11. see cppreference for details:
      // https://en.cppreference.com/w/cpp/string/basic_string.html
      errh << clGetPlatformInfo(plat, CL_PLATFORM_NAME, len, &str[0], nullptr);
      return str;
    }();
    // get platform OpenCL version
    auto version = [plat]
    {
      // platform version length including null terminator
      std::size_t len;
      errh << clGetPlatformInfo(plat, CL_PLATFORM_VERSION, 0u, nullptr, &len);
      // allocate string of NULs
      std::string str(len - 1u, '\0');
      // write null-terminated characters + return
      errh << clGetPlatformInfo(plat, CL_PLATFORM_VERSION, len, &str[0], nullptr);
      return str;
    }();
    // print
    std::cout << "  " << name << " w/ " << version << "\n";
  }
  // ensure flush and return
  std::cout << std::flush;
  return EXIT_SUCCESS;
}
