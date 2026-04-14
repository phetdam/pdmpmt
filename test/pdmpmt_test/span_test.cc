/**
 * @file span_test.cc
 * @author Derek Huang
 * @brief span.hh non-CUDA unit tests
 * @copyright MIT License
 */

#include "pdmpmt/span.hh"

// note: included earlier for feature detection macros
#include "pdmpmt/features.h"

#include <algorithm>
#include <type_traits>

#if PDMPMT_HAS_GMOCK
#include <gmock/gmock.h>
#endif  // PDMPMT_HAS_GMOCK
#include <gtest/gtest.h>

#if PDMPMT_HAS_CUDA
#include "pdmpmt/cuda_runtime.hh"
#include "pdmpmt/scope_exit.hh"    // for scoping cudaFree(), cudafreeHost()
#endif  // PDMPMT_HAS_CUDA

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

/**
 * Test that memory type queries work correctly with unregistered memory.
 */
TEST_F(SpanTest, IsHostNativeTest)
{
  std::vector<double> v{1., 2., 3., 4., 5.};
  pdmpmt::span s{v.data(), v.size()};
  EXPECT_TRUE(s.is_host());
  EXPECT_FALSE(s.is_device());
  EXPECT_FALSE(s.is_managed());
}

/**
 * Test that memory type queries work correctly with CUDA host pinned memory.
 */
TEST_F(SpanTest, IsHostTest)
{
#if PDMPMT_HAS_CUDA
  // allocate page-locked memory on host
  constexpr auto n_elem = 256u;
  unsigned char* data;
  cudaMallocHost(&data, n_elem);
  PDMPMT_CUDA_THROW_IF_ERROR();
  // ensure memory is freed on scope exit (ignore erros)
  pdmpmt::scope_exit _{[data] { cudaFreeHost(data); }};
  // wrap page-locked memory in span + check
  pdmpmt::span view{data, n_elem};
  EXPECT_TRUE(view.is_host());
  EXPECT_FALSE(view.is_device());
  EXPECT_FALSE(view.is_managed());
#else
  GTEST_SKIP() << "no CUDA headers available";
#endif  // !PDMPMT_HAS_CUDA
}

/**
 * Test that memory type queries work correctly with CUDA device memory.
 */
TEST_F(SpanTest, IsDeviceTest)
{
#if PDMPMT_HAS_CUDA
  // allocate device memory
  constexpr auto n_elem = 1024u;
  unsigned char* data;
  cudaMalloc(&data, n_elem);
  PDMPMT_CUDA_THROW_IF_ERROR();
  // ensure memory is freed on scope exit (ignore erros)
  pdmpmt::scope_exit _{[data] { cudaFree(data); }};
  // wrap device memory in span + check
  pdmpmt::span view{data, n_elem};
  EXPECT_FALSE(view.is_host());
  EXPECT_TRUE(view.is_device());
  EXPECT_FALSE(view.is_managed());
#else
  GTEST_SKIP() << "no CUDA headers available";
#endif  // !PDMPMT_HAS_CUDA
}

/**
 * Test that memory type queries work correctly with CUDA managed memory.
 */
TEST_F(SpanTest, IsManagedTest)
{
#if PDMPMT_HAS_CUDA
  // allocate CUDA managed memory
  constexpr auto n_elem = 512u;
  unsigned char* data;
  cudaMallocManaged(&data, n_elem);
  PDMPMT_CUDA_THROW_IF_ERROR();
  // ensure memory is freed on scope exit (ignore errors)
  pdmpmt::scope_exit _{[data] { cudaFree(data); }};
  // wrap managed memory in span + check
  pdmpmt::span view{data, n_elem};
  EXPECT_FALSE(view.is_host());
  EXPECT_FALSE(view.is_device());
  EXPECT_TRUE(view.is_managed());
#else
  GTEST_SKIP() << "no CUDA headers available";
#endif  // !PDMPMT_HAS_CUDA
}

}  // namespace
