/**
 * @file span_test.cu
 * @author Derek Huang
 * @brief span.hh CUDA tests
 * @copyright MIT License
 */

#include "pdmpmt/span.hh"

#include <cstddef>

#include <gtest/gtest.h>

#include "pdmpmt/common.h"
#include "pdmpmt/cuda_runtime.hh"
#include "pdmpmt/features.h"

#if PDMPMT_HAS_GMOCK
#include <gmock/gmock.h>
#endif  // PDMPMT_HAS_GMOCK
#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

namespace {

/**
 * Test fixture for span Thrust tests.
 */
class SpanThrustTest : public ::testing::Test {};

/**
 * CUDA kernel that increments part of a span.
 *
 * @note CUDA kernels cannot be class members.
 *
 * @param n Number of threads <= `out.size()`
 * @param out Span with values to increment
 */
PDMPMT_KERNEL
void span_increment_test_kernel(std::size_t n, pdmpmt::span<int> out)
{
  // starting thread index
  auto ti = blockIdx.x * blockDim.x + threadIdx.x;
  // determine work size
  auto work_size = out.size() / n;
  // if last thread pick up the slack
  if (ti == n - 1)
    work_size += out.size() % n;
  // starting index offset
  auto offset = out.size() / n * ti;
  // increment values
  for (auto i = offset; i < offset + work_size; i++)
    out[i] += 1;
}

/**
 * Test Thrust host vector modification.
 */
TEST_F(SpanThrustTest, HostVectorTest)
{
  thrust::host_vector v1{1, 2, 3, 4, 5};
  thrust::host_vector v2{0, 2, 2, 4, 5};
  pdmpmt::span s2{v2};
  s2[0] += 1;
  s2[2] += 1;
#if PDMPMT_HAS_GMOCK
  EXPECT_THAT(v2, ::testing::Pointwise(::testing::Eq(), v1));
#else
  EXPECT_EQ(v1, v2);
#endif  // PDMPMT_HAS_GMOCK
}

/**
 * Test Thrust device vector modification.
 *
 * This launches a kernel to do the modification before returning control and
 * copying the device data back to the CPU for comparison.
 */
TEST_F(SpanThrustTest, DeviceVectorTest)
{
  thrust::host_vector h1{1, 2, 3, 4, 5};
  thrust::device_vector d2{0, 1, 2, 3, 4};
  // 2 blocks of 2 threads
  span_increment_test_kernel<<<2, 2>>>(4u, d2);
  // sync + copy back to host vector
  cudaDeviceSynchronize();
  PDMPMT_CUDA_THROW_IF_ERROR();
  thrust::host_vector<int> h2(d2.size());
  thrust::copy(d2.begin(), d2.end(), h2.begin());
  // compare values
#if PDMPMT_HAS_GMOCK
  EXPECT_THAT(h2, ::testing::Pointwise(::testing::Eq(), h1));
#else
  EXPECT_EQ(h1, h2);
#endif  // !PDMPMT_HAS_GMOCK
}

/**
 * Test that memory type queries work correctly with a host vector.
 *
 * @note Could be moved to `span_test.cc` as there is no device code here.
 */
TEST_F(SpanThrustTest, IsHostTest)
{
  thrust::host_vector v{1, 2, 3, 4, 5};
  pdmpmt::span s{v};
  EXPECT_TRUE(s.is_host());
  EXPECT_FALSE(s.is_device());
  EXPECT_FALSE(s.is_managed());
}

/**
 * Test that memory type queries work correctly with a device vector.
 *
 * @note Could be moved to `span_test.cc` as there is no device code here.
 */
TEST_F(SpanThrustTest, IsDeviceTest)
{
  thrust::device_vector v{1, 2, 3, 4, 5};
  pdmpmt::span s{v};
  EXPECT_FALSE(s.is_host());
  EXPECT_TRUE(s.is_device());
  EXPECT_FALSE(s.is_managed());
}

}  // namespace
