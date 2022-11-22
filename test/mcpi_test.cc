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
  static constexpr std::size_t n_samples_ = 1000000;
  // default number of jobs to use at once
  static constexpr std::size_t n_jobs_ = 2;
  // double pi, if using C++20 we can use std::numbers instead
  static constexpr auto pi_ = boost::math::constants::pi<double>();
  // pi tolerance; result can vary greatly so to be safe tol is pretty big
  static constexpr double pi_tol_ = 1e-2;
  // PRNG seed
  static constexpr unsigned int seed_ = 8888;
};

/**
 * Test that serial estimation of pi using Monte Carlo works as expected.
 */
TEST_F(MonteCarloPiTest, SerialTest)
{
  EXPECT_NEAR(pi_, pdmpmt::mcpi(n_samples_, seed_), pi_tol_);
}

/**
 * Test that async estimation of pi using Monte Carlo works as expected.
 */
TEST_F(MonteCarloPiTest, AsyncTest)
{
  EXPECT_NEAR(pi_, pdmpmt::mcpi_async(n_samples_, seed_, n_jobs_), pi_tol_);
}

/**
 * Test that OpenMP estimation of pi using Monte Carlo works as expected.
 *
 * If the compiler does not support OpenMP, this test is skipped.
 */
TEST_F(MonteCarloPiTest, OpenMPTest)
{
#ifdef _OPENMP
  EXPECT_NEAR(
    pi_, pdmpmt::mcpi_omp(n_samples_, seed_, n_jobs_, n_jobs_), pi_tol_
  );
#else
  GTEST_SKIP() << "C++ compiler doesn't implement OpenMP";
#endif
}

}  // namespace
