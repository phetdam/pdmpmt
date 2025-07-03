/**
 * @file cl_platform_info.cc
 * @author Derek Huang
 * @brief C++ program for querying OpenCL platform info
 * @copyright MIT License
 */

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

// TODO: include CL/opencl.h instead?
#include <CL/cl.h>
// OpenCL extensions header
#ifdef __has_include
#if __has_include(<CL/cl_ext.h>)
#include <CL/cl_ext.h>
#endif  // !__has_include(<CL/cl_ext.h>)
#endif  // __has_include

namespace {

/**
 * Return the error identifier for a given OpenCL error code.
 *
 * @param err OpenCL error code
 */
constexpr auto cl_strerror(cl_int err) noexcept
{
  switch (err) {
#define ERR_CASE(v) case v: return #v;
#ifdef cl_khr_icd
    ERR_CASE(CL_PLATFORM_NOT_FOUND_KHR)
#endif  // cl_khr_icd
    ERR_CASE(CL_INVALID_VALUE)
    ERR_CASE(CL_OUT_OF_HOST_MEMORY)
    ERR_CASE(CL_INVALID_PLATFORM)
#undef ERR_CASE
  default:
    return "(unknown)";
  }
}

/**
 * Helper to print the OpenCL error identifier and exit on error.
 *
 * This is bit nicer compared to wrapping each OpenCL function in another
 * function call, e.g. `cl_error_exit(clGetPlatformIDs(...))`.
 */
struct opencl_error_handler {};

/**
 * Static OpenCL error handler helper.
 */
constexpr opencl_error_handler err_check;

/**
 * Print the OpenCL error identifier and exit on error.
 *
 * If the argument is `CL_SUCCESS` then nothing is done.
 *
 * @param err OpenCL error code
 */
void operator<<(opencl_error_handler /*handler*/, cl_int err)
{
  if (err == CL_SUCCESS)
    return;
  std::cerr << "OpenCL error: " << cl_strerror(err) << " (" << err << ")" <<
    std::endl;
  std::exit(EXIT_FAILURE);
}

}  // namespace

int main()
{
  // get the OpenCL platform IDs
  auto plat_ids = []
  {
    // get the number of platforms
    cl_uint n_plats;
    err_check << clGetPlatformIDs(0u, nullptr, &n_plats);
    // now get the platform IDs
    std::vector<cl_platform_id> ids(n_plats);
    err_check << clGetPlatformIDs(n_plats, ids.data(), nullptr);
    return ids;
  }();
  // lambda for getting null-terminated OpenCL platform information
  auto plat_strinfo = [](cl_platform_id id, cl_platform_info info)
  {
    // get length (including null terminator)
    std::size_t len;
    err_check << clGetPlatformInfo(id, info, 0u, nullptr, &len);
    // allocate buffer + get null-terminated name
    auto buf = std::make_unique<char[]>(len);
    err_check << clGetPlatformInfo(id, info, len, buf.get(), nullptr);
    return buf;
  };
  // get info for each platform
  std::cout << "OpenCL platforms: " << plat_ids.size() << std::endl;
  for (auto i = 0u; i < plat_ids.size(); i++) {
    // platform ID
    auto plat_id = plat_ids[i];
    std::cout << "  Platform " << i << ":\n";
    // print platform name + platform version string
    std::cout <<
      "    " << plat_strinfo(plat_id, CL_PLATFORM_NAME).get() << "\n" <<
      "    " << plat_strinfo(plat_id, CL_PLATFORM_VERSION).get() << "\n";
    // ensure flush
    std::cout << std::flush;
  }
  return EXIT_SUCCESS;
}
