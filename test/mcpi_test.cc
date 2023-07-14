/**
 * @file mcpi_test.cc
 * @author Derek Huang
 * @brief Unit test for the C++ implementation of Monte Carlo pi estimation.
 */

#include <pdmpmt/mcpi.h>
#include <pdmpmt/cpp/mcpi.h>

#include <cmath>
#include <cstdint>

#include <gtest/gtest.h>

/**
 * Macro for test skipping when compiler does not implement OpenMP.
 */
#define PDMPMT_NO_OMP_GTEST_SKIP() \
  GTEST_SKIP() << "C++ compiler doesn't implement OpenMP"

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
  // static constexpr auto pi_ = boost::math::constants::pi<double>();
  static inline const auto pi_ = 4 * std::atan(1);
  // pi tolerance; result can vary greatly so to be safe tol is pretty big
  static constexpr double pi_tol_ = 1e-2;
  // PRNG seed
  static constexpr unsigned int seed_ = 8888;
};

// aliases for C and C++ tests
using MonteCarloPiTestC = MonteCarloPiTest;
using MonteCarloPiTestCXX = MonteCarloPiTest;

/**
 * Test that C serial estimation of pi using Monte Carlo works as expected.
 */
TEST_F(MonteCarloPiTestC, SerialTest)
{
  EXPECT_NEAR(pi_, pdmpmt_mt32_smcpi(n_samples_, seed_), pi_tol_);
}

/**
 * Test that C OpenMP estimation of pi using Monte Carlo works as expected.
 *
 * If the compiler does not support OpenMP, this test is skipped.
 */
TEST_F(MonteCarloPiTestC, OpenMPTest)
{
#ifdef _OPENMP
  EXPECT_NEAR(pi_, pdmpmt_mt32_smcpi_ompm(n_samples_, n_jobs_, seed_), pi_tol_);
#else
  PDMPMT_NO_OMP_GTEST_SKIP();
#endif  // _OPENMP
}

/**
 * Test that C++ serial estimation of pi using Monte Carlo works as expected.
 */
TEST_F(MonteCarloPiTestCXX, SerialTest)
{
  EXPECT_NEAR(pi_, pdmpmt::mcpi(n_samples_, seed_), pi_tol_);
}

/**
 * Test that C++ async estimation of pi using Monte Carlo works as expected.
 */
TEST_F(MonteCarloPiTestCXX, AsyncTest)
{
  EXPECT_NEAR(pi_, pdmpmt::mcpi_async(n_samples_, seed_, n_jobs_), pi_tol_);
}

/**
 * Test that C++ OpenMP estimation of pi using Monte Carlo works as expected.
 *
 * If the compiler does not support OpenMP, this test is skipped.
 */
TEST_F(MonteCarloPiTestCXX, OpenMPTest)
{
#ifdef _OPENMP
  EXPECT_NEAR(
    pi_, pdmpmt::mcpi_omp(n_samples_, seed_, n_jobs_), pi_tol_
  );
#else
  PDMPMT_NO_OMP_GTEST_SKIP();
#endif  // _OPENMP
}

}  // namespace
