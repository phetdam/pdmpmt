/**
 * @file thurst.hh
 * @author Derek Huang
 * @brief C++ header for Thrust helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_THRUST_HH_
#define PDMPMT_THRUST_HH_

#include <ostream>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include "pdmpmt/cuda_runtime.hh"
#include "pdmpmt/ostream.hh"

// NB: operator<< overloads placed in std:: since NVIDIA CCCL compatibility
// guidelines for well-behaved code state not to add symbols to thrust::. thus
// for ADL to work the operator<< overloads must go in std::
namespace std {

/**
 * Insert the contents of a Thrust host vector into the stream.
 *
 * @tparam T Streamable type
 * @tparam A Allocator type
 *
 * @param out Output stream
 * @param vec Device vector to write
 */
template <typename T, typename A, typename = pdmpmt::ostreamable_t<T>>
auto& operator<<(std::ostream& out, const thrust::host_vector<T, A>& vec)
{
  pdmpmt::write(out, vec.begin(), vec.end());
  return out;
}

/**
 * Insert the contents of a Thrust device vector into the stream.
 *
 * @tparam T Streamable type
 * @tparam A Allocator type
 *
 * @param out Output stream
 * @param vec Device vector to write
 */
template <typename T, typename A, typename = pdmpmt::ostreamable_t<T>>
auto& operator<<(std::ostream& out, const thrust::device_vector<T, A>& vec)
{
  out << "<device " << pdmpmt::cuda_get_device() << "> ";
  pdmpmt::write(out, vec.begin(), vec.end());
  return out;
}

}  // namespace std

#endif  // PDMPMT_THRUST_HH_
