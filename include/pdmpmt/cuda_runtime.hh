/**
 * @file cuda_runtime.hh
 * @author Derek Huang
 * @brief C++ header with CUDA runtime API helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_CUDA_RUNTIME_HH_
#define PDMPMT_CUDA_RUNTIME_HH_

#include <cstdlib>
#include <iomanip>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <cuda_runtime.h>

namespace pdmpmt {

/**
 * Format the CUDA error code as a string.
 *
 * @param err CUDA error code
 */
inline auto cuda_error_string(cudaError_t err)
{
  return std::string{"CUDA error: "} + cudaGetErrorName(err) +
    std::string{": "} + cudaGetErrorString(err);
}

/**
 * CUDA runtime API exception type.
 *
 * This takes the CUDA error code and uses it to create the exception message.
 */
class cuda_runtime_exception : public std::runtime_error {
public:
  /**
   * Ctor.
   *
   * @param err CUDA error code
   */
  cuda_runtime_exception(cudaError_t err)
    : std::runtime_error{cuda_error_string(err)}
  {}
};

/**
 * Macro to throw a `cuda_runtime_exception` if there is a runtime API error.
 */
#define PDMPMT_CUDA_THROW_IF_ERROR() \
  do { \
    auto err = cudaGetLastError(); \
    if (err != cudaSuccess) \
      throw pdmpmt::cuda_runtime_exception{err}; \
  } \
  while (false)

/**
 * Print the error and exit if the CUDA error code indicates failure.
 *
 * @param out Stream to write error message to
 * @param err CUDA error code
 */
inline void cuda_error_exit(std::ostream& out, cudaError_t err)
{
  // success
  if (err == cudaSuccess)
    return;
  // failure
  out << cuda_error_string(err) << std::endl;
  std::exit(EXIT_FAILURE);
}

/**
 * Print the error to stderr and exit if CUDA error code indicates failure.
 *
 * @param err CUDA error code
 */
inline void cuda_error_exit(cudaError_t err)
{
  cuda_error_exit(std::cerr, err);
}

/**
 * Print the error to stderr and exit if the last CUDA runtime API call failed.
 */
inline void cuda_error_exit()
{
  cuda_error_exit(cudaGetLastError());
}

/**
 * Helper class for the CUDA driver and runtime versions.
 *
 * These have only major and minor version components hence the class name.
 */
class cuda_major_minor_ver {
public:
  /**
   * Ctor.
   *
   * @param int CUDA version encoded as (1000 * major + 10 * minor)
   */
  cuda_major_minor_ver(int ver) noexcept
      // cast required to suppress C2397 due to list-initialization
    : major_{static_cast<unsigned>(ver / 1000)},
      minor_{static_cast<unsigned>(ver % 100 / 10)}
  {}

  /**
   * Return the major version.
   */
  auto major() const noexcept { return major_; }

  /**
   * Return the minor version.
   */
  auto minor() const noexcept { return minor_; }

private:
  unsigned int major_;
  unsigned int minor_;
};

/**
 * Insertion operator for `cuda_major_minor_ver`.
 *
 * @param out Output stream
 * @param ver Version object to write
 */
inline auto& operator<<(std::ostream& out, const cuda_major_minor_ver& ver)
{
  return out << ver.major() << '.' << ver.minor();
}

/**
 * Return the CUDA driver version.
 */
inline cuda_major_minor_ver cuda_driver_version()
{
  int ver;
  cudaDriverGetVersion(&ver);
  PDMPMT_CUDA_THROW_IF_ERROR();
  return ver;
}

/**
 * Return the CUDA runtime version.
 */
inline cuda_major_minor_ver cuda_runtime_version()
{
  int ver;
  cudaRuntimeGetVersion(&ver);
  PDMPMT_CUDA_THROW_IF_ERROR();
  return ver;
}

/**
 * Return the number of available CUDA devices.
 *
 * @note For compatibility with the CUDA runtime API the return type is signed.
 */
inline int cuda_device_count()
{
  int n;
  cudaGetDeviceCount(&n);
  PDMPMT_CUDA_THROW_IF_ERROR();
  return n;
}

/**
 * Return the current CUDA device.
 *
 * If there is no available device an exception will be thrown. Therefore, to
 * check if there are available CUDA devices, check that `cuda_device_count()`
 * is a nonzero value before calling this function.
 *
 * @note Originally we wanted to return an empty `std::optional<>` in the case
 *  that the CUDA error code is `cudaErrorDeviceUnavailable` but this is not
 *  a valid enumerator value for `cudaError_t`.
 */
inline int cuda_get_device()
{
  int dev;
  cudaGetDevice(&dev);
  PDMPMT_CUDA_THROW_IF_ERROR();
  return dev;
}

/**
 * Return the property structure for the specified CUDA device.
 *
 * @param device CUDA device ID
 */
inline auto cuda_get_device_props(int device)
{
  cudaDeviceProp props;
  cudaGetDeviceProperties(&props, device);
  PDMPMT_CUDA_THROW_IF_ERROR();
  return props;
}

}  // namespace pdmpmt

/**
 * Insertion operator for a CUDA UUID type.
 *
 * The bytes are printed in hexadecimal.
 *
 * @note This is defined in the top-level namespace to allow ADL to work.
 *
 * @param out Stream to write to
 * @param uuid CUDA UUID struct
 */
inline auto& operator<<(std::ostream& out, const cudaUUID_t& uuid)
{
  // stream to take formatting
  std::stringstream ss;
  // cycle through array as if they were raw bytes
  for (unsigned i = 0; i < sizeof uuid.bytes; i++) {
    // add padding to distinguish bytes
    if (i)
      ss << ' ';
    // print byte with zero fill
    ss << std::setw(2) << std::hex << std::setfill('0') << (uuid.bytes[i] & 0xFF);
  }
  // stream result
  return out << ss.str();
}

#endif  // PDMPMT_CUDA_RUNTIME_HH_
