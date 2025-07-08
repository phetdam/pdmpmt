/**
 * @file opencl.hh
 * @author Derek HUang
 * @brief C++ header for OpenCL helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_OPENCL_HH_
#define PDMPMT_OPENCL_HH_

#include <sstream>
#include <stdexcept>
#include <vector>

#include <CL/opencl.h>

namespace pdmpmt {
namespace opencl {

/**
 * Return the error identifier for a given OpenCL error code.
 *
 * @param err OpenCL error code
 */
constexpr auto strerror(cl_int err) noexcept
{
  switch (err) {
#define PDMPMT_OPENCL_ERR_CASE(v) case v: return #v
#ifdef cl_khr_icd
    PDMPMT_OPENCL_ERR_CASE(CL_PLATFORM_NOT_FOUND_KHR);
#endif  // cl_khr_icd
    PDMPMT_OPENCL_ERR_CASE(CL_INVALID_VALUE);
    PDMPMT_OPENCL_ERR_CASE(CL_OUT_OF_HOST_MEMORY);
    PDMPMT_OPENCL_ERR_CASE(CL_INVALID_PLATFORM);
#undef PDMPMT_OPENCL_ERR_CASE
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
struct error_handler {};

/**
 * Static OpenCL error handler helper.
 */
constexpr error_handler errh;

/**
 * Exception type for an OpenCL error.
 */
class exception : public std::runtime_error {
public:
  /**
   * Ctor.
   *
   * @param err OpenCL error code
   */
  exception(cl_int err)
    : std::runtime_error{
        [err]
        {
          std::stringstream ss;
          ss << "OpenCL error: " << strerror(err) << " (" << err << ")";
          return ss.str();
        }()
      }
  {}

  /**
   * Return the associated OpenCL error code.
   */
  auto err() const noexcept { return err_; }

private:
  cl_int err_;
};

/**
 * Throw an OpenCL runtime exception on error.
 *
 * If the argument is `CL_SUCCESS` then nothing is done.
 *
 * @param err OpenCL error code
 */
inline void operator<<(error_handler /*handler*/, cl_int err)
{
  if (err != CL_SUCCESS)
    throw exception{err};
}

/**
 * Helper to get a vector of OpenCL platform IDs.
 *
 * This enumerates the OpenCL platforms that are available.
 */
inline auto platform_ids()
{
  // get the number of platforms
  cl_uint n_plats;
  errh << clGetPlatformIDs(0u, nullptr, &n_plats);
  // now get the platform IDs
  std::vector<cl_platform_id> ids(n_plats);
  errh << clGetPlatformIDs(n_plats, ids.data(), nullptr);
  return ids;
}

/**
 * OpenCL platform info converter.
 *
 * This takes the `cl_platform_info` value and converts it into a C++ type.
 * Each valid specialization implements the following:
 *
 * @code{.cc}
 * T operator()(cl_platform_id) const;
 * @endcode
 *
 * The operator may be `noexcept` and the `T` is defined by the specialization.
 *
 * @tparam I OpenCL platform info value
 */
template <cl_platform_info I, typename = void>
struct platform_info_converter {};

/**
 * Retrieve the specified OpenCL platform info.
 *
 * @note This is intended to be namespaced at some point.
 *
 * @tparam I OpenCL platform info value
 */
template <cl_platform_info I>
constexpr platform_info_converter<I> platform_info;

/**
 * Partial specialization for the types that return a string.
 *
 * @tparam I OpenCL platform info value
 */
template <cl_platform_info I>
struct platform_info_converter<
  I,
  std::enable_if_t<
    I == CL_PLATFORM_PROFILE ||
    I == CL_PLATFORM_VERSION ||
    I == CL_PLATFORM_NAME ||
    I == CL_PLATFORM_VENDOR ||
#ifdef cl_khr_icd
    I == CL_PLATFORM_ICD_SUFFIX_KHR ||
#endif  // cl_khr_icd
    I == CL_PLATFORM_EXTENSIONS
  > > {
  /**
   * Write the returned null-terminated character array as a `std::string`.
   *
   * @param id OpenCL platform ID
   */
  std::string operator()(cl_platform_id id) const
  {
    // get length (including null terminator)
    std::size_t len;
    errh << clGetPlatformInfo(id, I, 0u, nullptr, &len);
    // allocate buffer + get null-terminated name
    auto buf = std::make_unique<char[]>(len);
    errh << clGetPlatformInfo(id, I, len, buf.get(), nullptr);
    // return std::string from characters (minus null terminator)
    return {buf.get(), len - 1};
  }
};

/**
 * Get a vector of OpenCL device IDs from the given platform ID.
 *
 * This enumerates the OpenCL devices available for the platform.
 *
 * @param id OpenCL platform ID
 * @param type OpenCL device type
 */
auto device_ids(cl_platform_id id, cl_device_type type)
{
  // get the number of devices
  cl_uint n_dev;
  errh << clGetDeviceIDs(id, type, 0u, nullptr, &n_dev);
  // now get the device IDs
  std::vector<cl_device_id> ids(n_dev);
  errh << clGetDeviceIDs(id, type, n_dev, ids.data(), nullptr);
  return ids;
}

}  // namespace opencl
}  // namespace pdmpmt

#endif  // PDMPMT_OPENCL_HH_
