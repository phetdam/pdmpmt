/**
 * @file span_test.cc
 * @author Derek Huang
 * @brief span.hh non-CUDA unit tests
 * @copyright MIT License
 */

#include "pdmpmt/span.hh"

#include <algorithm>
#include <type_traits>

#include <gtest/gtest.h>

#include "pdmpmt/features.h"

#if PDMPMT_HAS_GMOCK
#include <gmock/gmock.h>
#endif  // PDMPMT_HAS_GMOCK

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

/**
 * Test span modification.
 */
TEST_F(SpanTest, ModifyTest)
{
  std::vector v1{1, 2, 3, 4, 5};
  std::vector v2{0, 1, 2, 3, 4};
  pdmpmt::span s2{v2.data(), v2.size()};
  for (auto& v : s2)
    v += 1;
#if PDMPMT_HAS_GMOCK
  EXPECT_THAT(v2, ::testing::Pointwise(::testing::Eq(), v1));
#else
  EXPECT_EQ(v1, v2);
#endif  // !PDMPMT_HAS_GMOCK
}

/**
 * Test span integration with `std::vector`.
 */
TEST_F(SpanTest, VectorTest)
{
  std::vector vec{1, 2, 3, 4, 5};
  pdmpmt::span view{vec.data(), vec.size()};
#if PDMPMT_HAS_GMOCK
  // note: EXPECT_THAT works with different containers with the same element
  // type as long as they provide the value_type member
  EXPECT_THAT(view, ::testing::Pointwise(::testing::Eq(), vec));
#else
  // copy into vector for comparison
  decltype(vec) vec2(vec.size());
  std::copy(view.begin(), view.end(), vec2.begin());
  EXPECT_EQ(vec, vec2);
#endif  // !PDMPMT_HAS_GMOCK
}

}  // namespace
