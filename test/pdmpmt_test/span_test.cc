/**
 * @file span_test.cc
 * @author Derek Huang
 * @brief span.hh non-CUDA unit tests
 * @copyright MIT License
 */

#include "pdmpmt/span.hh"

#include <type_traits>

#include <gtest/gtest.h>

namespace {

/**
 * Test fixture for span non-CUDA tests.
 */
class SpanTest : public ::testing::Test {};

/**
 * Test span add-const conversion using type traits.
 */
TEST_F(SpanTest, AddConstTest)
{
  using pdmpmt::span;
  EXPECT_TRUE((std::is_convertible_v<span<float>, span<const float>>));
}

}  // namespace
