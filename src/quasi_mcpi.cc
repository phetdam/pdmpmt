/**
 * @file quasi_mcpi.hh
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
  pdmpmt::scoped_timer::duration time;
  // compute pi using n_int * n_int points
  // note: MSVC requires default captures (this is compiler conformance bug) to
  // use the value of n_int despite n_int being constexpr
  auto pi_hat = [=, &time]
  {
    pdmpmt::scoped_timer timer{time};
    return pdmpmt::quasi_mcpi(n_int);
  }();
  // convert time to milliseconds
  auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(time);
  // print result and other info
  std::cout << "pi (n_points=" << (n_int * n_int) << "): " << pi_hat <<
    " in " << ms_time.count() << " ms" << std::endl;
  return EXIT_SUCCESS;
}
