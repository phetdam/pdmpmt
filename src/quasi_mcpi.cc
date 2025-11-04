/**
 * @file quasi_mcpi.cc
 * @author Derek Huang
 * @brief C++ program computing pi using quasi Monte Carlo
 * @copyright MIT License
 */

#include <chrono>
#include <cstdlib>
#include <iostream>

#include "pdmpmt/mcpi.hh"
#include "pdmpmt/scoped_timer.hh"

int main()
{
  // number of intervals to make on an axis
  constexpr auto n_int = 5000u;
  // time taken to estimate pi
  std::chrono::milliseconds time;
  // compute pi using n_int * n_int points
  auto pi_hat = [&time]
  {
    pdmpmt::scoped_timer timer{time};
    return pdmpmt::quasi_mcpi(n_int);
  }();
  // print result and other info
  std::cout << "pi (n_points=" << (n_int * n_int) << "): " << pi_hat <<
    " in " << time.count() << " ms" << std::endl;
  return EXIT_SUCCESS;
}
