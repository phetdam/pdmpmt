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

#include "pdmpmt/type_traits.hh"

namespace {

/**
 * Exit with error if the CUDA error code indicates failure.
 *
 * @param err CUDA error code
 */
void cuda_check(cudaError_t err)
{
  // success
  if (err == cudaSuccess)
    return;
  // failure
  std::cerr << "CUDA error: " << cudaGetErrorName(err) << ": " <<
    cudaGetErrorString(err) << std::endl;
  std::exit(EXIT_FAILURE);
}

/**
 * Exit with error if the last CUDA runtime API call failed.
 */
void cuda_check()
{
  cuda_check(cudaGetLastError());
}

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
  // print some version info
  int dr_ver, rt_ver;
  cudaDriverGetVersion(&dr_ver);
  cuda_check();
  std::cout << "CUDA driver version: " <<
    (dr_ver / 1000) << "." << (dr_ver % 100 / 10) << std::endl;
  cudaRuntimeGetVersion(&rt_ver);
  cuda_check();
  std::cout << "CUDA runtime version: " <<
    (rt_ver / 1000) << "." << (rt_ver % 100 / 10) << std::endl;
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
