/**
 * @file mcpi_test.cc
 * @author Derek Huang
 * @brief Unit test for the C++ implementation of Monte Carlo pi estimation.
 */

#include <pdmpmt/cpp/mcpi.h>

#include <cstdint>

#include <boost/math/constants/constants.hpp>
#include <gtest/gtest.h>

namespace {

/**
 * Test fixture for tests on estimating pi using Monte Carlo.
 */
class MonteCarloPiTest : public ::testing::Test {
protected:
  // number of samples to use for estimating pi in a single process; produces a
  // very crude guess, so the tolerance we will need is relatively large.
  static constexpr std::uintmax_t n_samples_ = 1000000;
  // double pi, if using C++20 we can use std::numbers instead
  static constexpr auto pi_ = boost::math::constants::pi<double>();
  // pi tolerance; using n_samples_ only gives accuracy to 5 sig figs.
  // question: tol has to be 1e-2 in Python land, why is this? conversion?
  static constexpr double pi_tol_ = 1e-5;
  // PRNG seed
  static constexpr unsigned int seed_ = 1900;
};

/**
 * Test that serial estimation of pi using Monte Carlo works as expected.
 */
TEST_F(MonteCarloPiTest, SerialTest)
{
  EXPECT_NEAR(pi_, pdmpmt::mcpi(n_samples_, seed_), pi_tol_);
}

}  // namespace