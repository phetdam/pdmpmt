/**
 * @file thrust_demo.cc
 * @author Derek Huang
 * @brief C++ Thrust demo program
 * @copyright MIT License
 */

#include <cstdlib>
#include <iostream>

#include <thrust/device_vector.h>
#include <thrust/reduce.h>
#include <thrust/sequence.h>
#include <thrust/version.h>

#include "pdmpmt/cuda_runtime.hh"
#include "pdmpmt/thrust.hh"  // for device_vector operator<<

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
