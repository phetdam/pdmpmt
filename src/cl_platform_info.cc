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
#include <sstream>
#include <stdexcept>
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
constexpr opencl_error_handler cl_check;

/**
 * Exception type for an OpenCL error.
 */
class cl_runtime_error : public std::runtime_error {
public:
  /**
   * Ctor.
   *
   * @param err OpenCL error code
   */
  cl_runtime_error(cl_int err)
    : std::runtime_error{
        [err]
        {
          std::stringstream ss;
          ss << "OpenCL error: " << cl_strerror(err) << " (" << err << ")";
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
void operator<<(opencl_error_handler /*handler*/, cl_int err)
{
  if (err == CL_SUCCESS)
    return;
  throw cl_runtime_error{err};
}

/**
 * Indentation helper.
 */
class indentation_type {
public:
  /**
   * Ctor.
   *
   * This uses the default indentation size of 2 spaces per level.
   *
   * @param levels Number of indentation levels
   */
  constexpr indentation_type(unsigned levels) noexcept
    : indentation_type(2u, levels)
  {}

   /**
    * Ctor.
    *
    * @param size Number of characters in one indentation level
    * @param levels NUmber of indentation levels
    */
  constexpr indentation_type(unsigned size, unsigned levels) noexcept
    : size_{size}, levels_{levels}
  {}

  // getters
  constexpr auto size() const noexcept { return size_; }
  constexpr auto levels() const noexcept { return levels_; }

private:
  unsigned size_;   // number of characters in one indentation level
  unsigned levels_;  // number of indentation levels
};

/**
 * Insert the correct number of spaces given the indentation object.
 *
 * @param out Output stream
 * @param ind Indentation object
 */
auto& operator<<(std::ostream& out, indentation_type ind)
{
  auto n = ind.size() * ind.levels();
  for (decltype(n) i = 0; i < n; i++)
    out.put(' ');
  return out;
}

/**
 * Customization point object that returns an indentation object.
 */
struct indentation_factory {
  constexpr indentation_type operator()(unsigned levels) const noexcept
  {
    return levels;
  }
};

// global for generating indentation objects
constexpr indentation_factory indent;

/**
 * Helper to get a vector of OpenCL platform IDs.
 *
 * This enumerates the OpenCL platforms that are available.
 */
auto cl_platform_ids()
{
  // get the number of platforms
  cl_uint n_plats;
  cl_check << clGetPlatformIDs(0u, nullptr, &n_plats);
  // now get the platform IDs
  std::vector<cl_platform_id> ids(n_plats);
  cl_check << clGetPlatformIDs(n_plats, ids.data(), nullptr);
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
struct cl_platform_info_converter {};

/**
 * Retrieve the specified OpenCL platform info.
 *
 * @note This is intended to be namespaced at some point.
 *
 * @tparam I OpenCL platform info value
 */
template <cl_platform_info I>
constexpr cl_platform_info_converter<I> platform_info;

/**
 * Partial specialization for the types that return a string.
 *
 * @tparam I OpenCL platform info value
 */
template <cl_platform_info I>
struct cl_platform_info_converter<
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
    cl_check << clGetPlatformInfo(id, I, 0u, nullptr, &len);
    // allocate buffer + get null-terminated name
    auto buf = std::make_unique<char[]>(len);
    cl_check << clGetPlatformInfo(id, I, len, buf.get(), nullptr);
    // return std::string from characters (minus null terminator)
    return {buf.get(), len - 1};
  }
};

}  // namespace

int main()
{
  // get the OpenCL platform IDs
  auto plat_ids = cl_platform_ids();
  // get info for each platform
  std::cout << "OpenCL platforms: " << plat_ids.size() << std::endl;
  for (auto i = 0u; i < plat_ids.size(); i++) {
    // platform ID
    auto plat_id = plat_ids[i];
    std::cout << indent(1) << "Platform " << i << ":\n";
    // print platform name + platform version string
    std::cout <<
      indent(2) << platform_info<CL_PLATFORM_NAME>(plat_id) << "\n" <<
      indent(2) << platform_info<CL_PLATFORM_VERSION>(plat_id) << "\n";
    // ensure flush
    std::cout << std::flush;
  }
  return EXIT_SUCCESS;
}
