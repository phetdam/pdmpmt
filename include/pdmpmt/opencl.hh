/**
 * @file opencl.hh
 * @author Derek HUang
 * @brief C++ header for OpenCL helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_OPENCL_HH_
#define PDMPMT_OPENCL_HH_

#include <cstddef>
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
    PDMPMT_OPENCL_ERR_CASE(CL_SUCCESS);
#ifdef cl_khr_icd
    PDMPMT_OPENCL_ERR_CASE(CL_PLATFORM_NOT_FOUND_KHR);
#endif  // cl_khr_icd
    PDMPMT_OPENCL_ERR_CASE(CL_INVALID_VALUE);
    PDMPMT_OPENCL_ERR_CASE(CL_OUT_OF_HOST_MEMORY);
    PDMPMT_OPENCL_ERR_CASE(CL_INVALID_PLATFORM);
    PDMPMT_OPENCL_ERR_CASE(CL_INVALID_DEVICE_TYPE);
    PDMPMT_OPENCL_ERR_CASE(CL_DEVICE_NOT_FOUND);
    PDMPMT_OPENCL_ERR_CASE(CL_OUT_OF_RESOURCES);
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
   * @param plat OpenCL platform ID
   */
  auto operator()(cl_platform_id plat) const
  {
    // get length (including null terminator)
    std::size_t len;
    errh << clGetPlatformInfo(plat, I, 0u, nullptr, &len);
    // allocate string of NULs
    std::string str(len - 1u, '\0');
    // write null-terminated characters (including null terminator) + return.
    // this is defined behavior since C++11. see cppreference for details:
    // https://en.cppreference.com/w/cpp/string/basic_string/operator_at.html
    errh << clGetPlatformInfo(plat, I, len, &str[0], nullptr);
    return str;
  }
};

#if defined(CL_VERSION_3_0)  || defined(cl_khr_extended_versioning)
/**
 * Partial specialization for the platform's supported OpenCL numeric version.
 *
 * @note This is only available since OpenCL 3.0 or if the OpenCL extenson
 *  `cl_khr_extended_versioning` is available, e.g. the macro is defined.
 *
 * @tparam I OpenCL platform info value
 */
template <cl_platform_info I>
struct platform_info_converter<
  I,
  std::enable_if_t<
// if OpenCL 3.0 is available, use CL_PLATFORM_NUMERIC_VERSION
#if defined(CL_VERSION_3_0)
    I == CL_PLATFORM_NUMERIC_VERSION
// otherwise, use the Khronos extension
#else
    I == CL_PLATFORM_NUMERIC_VERSION_KHR
#endif  // !defined(CL_VERSION_3_0)
  > > {
  /**
   * Get the OpenCL integral version number.
   *
   * @param plat OpenCL platform ID
   */
  auto operator()(cl_platform_id plat) const
  {
// for OpenCL 3.0, use cl_version, otherwise use the extension type
#if defined(CL_VERSION_3_0)
    using version_type = cl_version;
#else
    using version_type = cl_version_khr;
#endif  // !defined(CL_VERSION_3_0)
    // get the OpenCL version number
    version_type ver;
    errh << clGetPlatformInfo(plat, I, sizeof ver, &ver, nullptr);
    return ver;
  }
};
#endif  // defined(CL_VERSION_3_0)  || defined(cl_khr_extended_versioning)

/**
 * Get a vector of OpenCL device IDs from the given platform ID.
 *
 * This enumerates OpenCL devices of the given type available for the platform.
 *
 * @param plat OpenCL platform ID
 * @param type OpenCL device type
 */
inline auto device_ids(cl_platform_id plat, cl_device_type type)
{
  // get the number of devices
  cl_uint n_dev;
  errh << clGetDeviceIDs(plat, type, 0u, nullptr, &n_dev);
  // now get the device IDs
  std::vector<cl_device_id> devs(n_dev);
  errh << clGetDeviceIDs(plat, type, n_dev, devs.data(), nullptr);
  return devs;
}

/**
 * Get a vector of all the OpenCL device IDs for the given platform ID.
 *
 * This enumerates all the OpenCL devices available for the platform.
 *
 * @param plat OpenCL platform ID
 */
inline auto device_ids(cl_platform_id plat)
{
  return device_ids(plat, CL_DEVICE_TYPE_ALL);
}

/**
 * OpenCL device info converter.
 *
 * This takes the `ck_device_info` value and converts it into a C++ type.
 * Each valid specialization implements the following:
 *
 * @code{.cc}
 * T operator()(cl_device_id) const;
 * @endcode
 *
 * The operator may be `noexcept` and the `T` is defined by the specialization.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I, typename = void>
struct device_info_converter {};

/**
 * Retrieve the specified OpenCL device info.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I>
constexpr device_info_converter<I> device_info;

/**
 * Partial specialization for the types that return a string.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I>
struct device_info_converter<
  I,
  std::enable_if_t<
#if defined(CL_VERSION_1_2)
    I == CL_DEVICE_IL_VERSION ||
#elif defined(cl_khr_il_program)
    I == CL_DEVICE_IL_VERSION_KHR ||
#endif  // !defined(CL_VERSION_1_2) && !defined(cl_khr_il_program)
#ifdef CL_VERSION_1_2
    I == CL_DEVICE_BUILT_IN_KERNELS ||
#endif  // CL_VERSION_1_2
    I == CL_DEVICE_NAME ||
    I == CL_DEVICE_VENDOR ||
    I == CL_DRIVER_VERSION ||
    I == CL_DEVICE_PROFILE ||
    I == CL_DEVICE_VERSION ||
#ifdef CL_VERSION_3_0
    I == CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED ||
#endif  // CL_VERSION_3_0
#ifdef cl_khr_spir
    I == CL_DEVICE_SPIR_VERSIONS ||
#endif  // cl_khr_spir
#ifdef CL_VERSION_3_0
    I == CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED ||
#endif  // CL_VERSION_3_0
    I == CL_DEVICE_EXTENSIONS
  > > {
  /**
   * Write the returned null-terminated character array as a `std::string`.
   *
   * @param dev OpenCL device ID
   */
  auto operator()(cl_device_id dev) const
  {
    // get length (including null terminator)
    std::size_t len;
    errh << clGetDeviceInfo(dev, I, 0u, nullptr, &len);
    // allocate string of NULs
    std::string str(len - 1u, '\0');
    // write null-terminated characters (including null terminator) + return.
    // this is defined behavior since C++11. see cppreference for details:
    // https://en.cppreference.com/w/cpp/string/basic_string/operator_at.html
    errh << clGetDeviceInfo(dev, I, len, &str[0], nullptr);
    return str;
  }
};

#if defined(CL_VERSION_3_0)  || defined(cl_khr_extended_versioning)
/**
 * Partial specialization for the device's supported OpenCL numeric version.
 *
 * @note This is only available since OpenCL 3.0 or if the OpenCL extenson
 *  `cl_khr_extended_versioning` is available, e.g. the macro is defined.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I>
struct device_info_converter<
  I,
  std::enable_if_t<
// if OpenCL 3.0 is available, use CL_DEVICE_NUMERIC_VERSION
#if defined(CL_VERSION_3_0)
    I == CL_DEVICE_NUMERIC_VERSION
// otherwise, use the Khronos extension
#else
    I == CL_DEVICE_NUMERIC_VERSION_KHR
#endif  // !defined(CL_VERSION_3_0)
  > > {
  /**
   * Return the numeric OpenCL veresion supported by the device.
   *
   * @param dev OPenCL device ID
   */
  auto operator()(cl_device_id dev) const
  {
    // init + return the numeric OpenCL version
#if defined(CL_VERSION_3_0)
    cl_version ver;
#else
    cl_version_khr ver;
#endif  // !defined(CL_VERSION_3_0)
    errh << clGetDeviceInfo(dev, I, sizeof ver, &ver, nullptr);
    return ver;
  }
};
#endif  // defined(CL_VERSION_3_0)  || defined(cl_khr_extended_versioning)

/**
 * Partial specialization for the OpenCL device type bitmask.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I>
struct device_info_converter<I, std::enable_if_t<I == CL_DEVICE_TYPE>> {
  /**
   * Return the OpenCL device type bitmask for the device.
   *
   * @param dev OpenCL device ID
   */
  auto operator()(cl_device_id dev) const
  {
    cl_device_type type;
    errh << clGetDeviceInfo(dev, I, sizeof type, &type, nullptr);
    return type;
  }
};

/**
 * Partial specialization for the `cl_uint` types.
 *
 * This handles *only* the `cl_device_info` values whose `clGetDeviceInfo`
 * return type is explicitly stated as `cl_uint`. Any typedefs are excluded.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I>
struct device_info_converter<
  I,
  std::enable_if_t<
    // TODO: incomplete
    I == CL_DEVICE_VENDOR_ID ||
    I == CL_DEVICE_MAX_COMPUTE_UNITS ||
    I == CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS ||
    I == CL_DEVICE_MAX_CLOCK_FREQUENCY ||
    I == CL_DEVICE_ADDRESS_BITS ||
    I == CL_DEVICE_MAX_READ_IMAGE_ARGS ||
    I == CL_DEVICE_MAX_WRITE_IMAGE_ARGS
  > > {
  /**
   * Return the `cl_uint` info value for the given OpenCL device ID.
   *
   * @param dev OpenCL device ID
   */
  auto operator()(cl_device_id dev) const
  {
    cl_uint res;
    errh << clGetDeviceInfo(dev, I, sizeof res, &res, nullptr);
    return res;
  }
};

/**
 * Partial specialization for `CL_DEVICE_MAX_WORK_ITEM_SIZES`.
 *
 * The `operator()` returns a `std::vector<std::size_t>`.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I>
struct device_info_converter<
  I, std::enable_if_t<I == CL_DEVICE_MAX_WORK_ITEM_SIZES> > {
  /**
   * Return a vector of the max work items per dimension for the device.
   *
   * @param dev OpenCL device ID
   */
  auto operator()(cl_device_id dev) const
  {
    // get length of size_t array in bytes
    // note: CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS gives the number of dimensions
    // which wouuld be len / sizeof(std::size_t)
    std::size_t len;
    errh << clGetDeviceInfo(dev, I, 0u, nullptr, &len);
    // get vector of max work item dimensions
    std::vector<std::size_t> dims(len / sizeof(std::size_t));
    errh << clGetDeviceInfo(dev, I, len, dims.data(), nullptr);
    return dims;
  }
};

/**
 * Partial specialization for the `cl_ulong` types.
 *
 * This handles *only* the `cl_device_info` values whose `clGetDeviceInfo`
 * return type is explicitly stated as `cl_ulong`. Any typedefs are excluded.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I>
struct device_info_converter<
  I,
  std::enable_if_t<
    I == CL_DEVICE_MAX_MEM_ALLOC_SIZE ||
    I == CL_DEVICE_GLOBAL_MEM_CACHE_SIZE ||
    I == CL_DEVICE_GLOBAL_MEM_SIZE ||
    I == CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE ||
    I == CL_DEVICE_LOCAL_MEM_SIZE
  > > {
  /**
   * Return the `cl_ulong` info value for the given OpenCL device ID.
   *
   * @param dev OpenCL device ID
   */
  auto operator()(cl_device_id dev) const
  {
    cl_ulong res;
    errh << clGetDeviceInfo(dev, I, sizeof res, &res, nullptr);
    return res;
  }
};

/**
 * Partial specialization for the `size_t` types.
 *
 * This handles *only* the `cl_device_info` values whose `clGetDeviceInfo`
 * return type is explicitly stated as `size_t`. Any typedefs are excluded.
 *
 * @tparam I OpenCL device info value
 */
template <cl_device_info I>
struct device_info_converter<
  I,
  std::enable_if_t<
    I == CL_DEVICE_MAX_WORK_GROUP_SIZE ||
    I == CL_DEVICE_IMAGE2D_MAX_WIDTH ||
    I == CL_DEVICE_IMAGE2D_MAX_HEIGHT ||
    I == CL_DEVICE_IMAGE3D_MAX_WIDTH ||
    I == CL_DEVICE_IMAGE3D_MAX_HEIGHT ||
    I == CL_DEVICE_IMAGE3D_MAX_DEPTH ||
#ifdef CL_VERSION_1_2
    I == CL_DEVICE_IMAGE_MAX_BUFFER_SIZE ||
    I == CL_DEVICE_IMAGE_MAX_ARRAY_SIZE ||
#endif  // CL_VERSION_1_2
    I == CL_DEVICE_MAX_PARAMETER_SIZE ||
#ifdef CL_VERSION_2_0
    I == CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE ||
    I == CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE ||
#endif  // CL_VERSION_2_0
#ifdef CL_VERSION_1_2
    I == CL_DEVICE_PRINTF_BUFFER_SIZE ||
#endif  // CL_VERSION_1_2
#ifdef CL_VERSION_3_0
    I == CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE ||
#endif  // CL_VERSION_3_0
    I == CL_DEVICE_PROFILING_TIMER_RESOLUTION
  > > {
  /**
   * Return the `size_t` info value for the given openCL device ID.
   *
   * @param dev OPenCL device ID
   */
  auto operator()(cl_device_id dev) const
  {
    size_t res;
    errh << clGetDeviceInfo(dev, I, sizeof res, &res, nullptr);
    return res;
  }
};

}  // namespace opencl
}  // namespace pdmpmt

#endif  // PDMPMT_OPENCL_HH_
