/**
 * @file mcpi.cu
 * @author Derek Huang
 * @brief CUDA C++ implementation for estimating pi with Monte Carlo
 * @copyright MIT License
 */

#include <cstdlib>
#include <iostream>

#include <thrust/device_vector.h>
#include <thrust/reduce.h>
#include <thrust/sequence.h>

#include "pdmpmt/type_traits.hh"

namespace {

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
  thrust::device_vector<double> values(4);
  thrust::sequence(values.begin(), values.end());
  std::cout << "values are " << values << std::endl;
  auto sum = thrust::reduce(values.begin(), values.end());
  std::cout << "sum is " << sum << std::endl;
  return EXIT_SUCCESS;
}
