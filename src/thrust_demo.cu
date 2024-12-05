/**
 * @file mcpi.cu
 * @author Derek Huang
 * @brief CUDA C++ implementation for estimating pi with Monte Carlo
 * @copyright MIT License
 */

#include <cstdlib>
#include <iostream>

#include <cuda_runtime.h>
#include <thrust/device_vector.h>
#include <thrust/reduce.h>
#include <thrust/sequence.h>
#include <thrust/version.h>

#include "pdmpmt/cuda_runtime.hh"
#include "pdmpmt/type_traits.hh"

namespace {

/**
 * Insert the contents of a device vector into the stream.
 *
 * @tparam T Streamable type
 * @tparam A Allocator
 *
 * @param out Output stream
 * @param vec Device vector to write
 */
template <typename T, typename A, typename = pdmpmt::ostreamable_t<T>>
auto& operator<<(std::ostream& out, const thrust::device_vector<T, A>& vec)
{
  out << '[';
  for (auto it = vec.begin(); it != vec.end(); it++) {
    if (it != vec.begin())
      out << ", ";
    out << *it;
  }
  return out << ']';
}

}  // namespace

int main()
{
  // print driver and runtime versions
  std::cout <<
    "CUDA driver version: " << pdmpmt::cuda_driver_version() << '\n' <<
    "CUDA runtime version: " << pdmpmt::cuda_runtime_version() << std::endl;
  // print the Thrust version
  std::cout << "Thrust version: " << THRUST_MAJOR_VERSION << "." <<
    THRUST_MINOR_VERSION << "." << THRUST_SUBMINOR_VERSION << std::endl;
  // create device vector ascending sequence
  thrust::device_vector<double> values(4);
  thrust::sequence(values.begin(), values.end());
  // print and reduce (accumulate)
  std::cout << "values are " << values << std::endl;
  auto sum = thrust::reduce(values.begin(), values.end());
  std::cout << "sum is " << sum << std::endl;
  return EXIT_SUCCESS;
}
