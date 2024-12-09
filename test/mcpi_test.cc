/**
 * @file mcpi_test.cc
 * @author Derek Huang
 * @brief Unit test for the C++ implementation of Monte Carlo pi estimation.
 * @copyright MIT License
 */

#include "pdmpmt/mcpi.h"
#include "pdmpmt/mcpi.hh"

#include <cmath>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include "pdmpmt/common.h"
#include "pdmpmt/features.h"

// can use <numbers> for pi
#if PDMPMT_HAS_CC20
#include <numbers>
#endif  // !PDMPMT_HAS_CC20

/**
 * Macro for test skipping when compiler does not implement OpenMP.
 */
#define PDMPMT_NO_OMP_GTEST_SKIP() \
  GTEST_SKIP() << "C++ compiler doesn't implement OpenMP"

namespace {

/**
 * Test fixture for tests on estimating pi using Monte Carlo.
 */
class MCPiTest : public ::testing::Test {
protected:
  // number of samples to use for estimating pi in a single process; produces a
  // very crude guess, so the tolerance we will need is relatively large.
  static constexpr std::size_t n_samples_ = 1000000;
  // default number of jobs to use at once
  static constexpr std::size_t n_jobs_ = 8;
  // double pi, if using C++20 we can use std::numbers instead
  // static constexpr auto pi_ = boost::math::constants::pi<double>();
#if PDMPMT_HAS_CC20
  static inline constexpr auto pi_ = std::numbers::pi;
#else
  static inline const auto pi_ = 4 * std::atan(1);
#endif  // !PDMPMT_HAS_CC20
  // pi tolerance; result can vary greatly so to be safe tol is pretty big
  static constexpr double pi_tol_ = 1e-2;
  // PRNG seed
  static constexpr unsigned int seed_ = 8888;
};

/**
 * Parametrized testing fixture for the C tests.
 */
class MCPiTestC
  : public MCPiTest, public ::testing::WithParamInterface<pdmpmt_rng_type> {};

// type alias for the C++ tests
using MCPiTestCXX = MCPiTest;

/**
 * Test that C serial estimation of pi using Monte Carlo works as expected.
 */
TEST_P(MCPiTestC, SerialTest)
{
  EXPECT_NEAR(pi_, pdmpmt_rng_smcpi(n_samples_, GetParam(), seed_), pi_tol_);
}

INSTANTIATE_TEST_SUITE_P(
  Rng,
  MCPiTestC,
  ::testing::Values(PDMPMT_RNG_MRG32K3A, PDMPMT_RNG_MT19937),
  // test name generator for easier identification of inputs
  [](const ::testing::TestParamInfo<pdmpmt_rng_type>& info) -> std::string
  {
    switch (info.param) {
      case PDMPMT_RNG_MRG32K3A:
        return "MRG32k3a";
      case PDMPMT_RNG_MT19937:
        return "MT19937";
      default:
        return "UnknownRng" + std::to_string(info.index);
    }
  }
);

/**
 * Test that C serial Monte Carlo pi estimation works with MRG32k3a.
 *
 * @note Should use `TEST_P` for this with `PDMPMT_RNG_MT19937`.
 */
TEST_F(MCPiTestC, SerialMRG32k3aTest)
{
  EXPECT_NEAR(
    pi_,
    pdmpmt_rng_smcpi(n_samples_, PDMPMT_RNG_MRG32K3A, seed_),
    pi_tol_
  );
}

/**
 * Test that C OpenMP estimation of pi using Monte Carlo works as expected.
 *
 * If the compiler does not support OpenMP, this test is skipped.
 */
TEST_F(MCPiTestC, OpenMPTest)
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
TEST_F(MCPiTestCXX, SerialTest)
{
  EXPECT_NEAR(pi_, pdmpmt::mcpi(n_samples_, seed_), pi_tol_);
}

/**
 * Test that C++ async estimation of pi using Monte Carlo works as expected.
 */
TEST_F(MCPiTestCXX, AsyncTest)
{
  EXPECT_NEAR(pi_, pdmpmt::mcpi_async(n_samples_, seed_, n_jobs_), pi_tol_);
}

/**
 * Test that C++ OpenMP estimation of pi using Monte Carlo works as expected.
 *
 * If the compiler does not support OpenMP, this test is skipped.
 */
TEST_F(MCPiTestCXX, OpenMPTest)
{
#ifdef _OPENMP
  EXPECT_NEAR(pi_, pdmpmt::mcpi_omp(n_samples_, seed_, n_jobs_), pi_tol_);
#else
  PDMPMT_NO_OMP_GTEST_SKIP();
#endif  // _OPENMP
}

}  // namespace
