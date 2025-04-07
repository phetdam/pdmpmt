/**
 * @file thrust_mcpi.cu
 * @author Derek Huang
 * @brief CUDA C++ program to compute pi with Monte Carlo using Thrust + CUDA
 */

#include <cstdlib>
#include <iostream>

#include <thrust/device_vector.h>
#include <thrust/random.h>

#include "pdmpmt/mcpi.hh"
#include "pdmpmt/thrust.hh"

int main()
{
  // seed + sample count
  constexpr auto seed = 8888u;
  constexpr auto n_samples = 1000000u;
  // compute + print
  // TODO: should put this as part of the test runner
  auto pi = pdmpmt::mcpi(n_samples, seed);
  std::cout << "pi (n_samples=" << n_samples << ", seed=" << seed << "): " <<
    pi << std::endl;
  return EXIT_SUCCESS;
}
