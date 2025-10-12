/**
 * @file block_test.cc
 * @author Derek Huang
 * @brief block.h unit tests
 * @copyright MIT License
 */

#include "pdmpmt/block.h"

#include <gtest/gtest.h>

namespace {

/**
 * Base test fixture for the block tests.
 */
class BlockTest : public ::testing::Test {};

/**
 * Test fixture for ulong block tests.
 */
class UlongBlockTest : public BlockTest {
protected:
  /**
   * Clean up the allocated block.
   *
   * @note If the data pointer is `NULL` then nothing is done.
   */
  ~UlongBlockTest()
  {
    pdmpmt_block_ulong_free(&block_);
  }

  pdmpmt_block_ulong block_{};
};

/**
 * Allocate some memory for the ulong block.
 */
TEST_F(UlongBlockTest, AllocTest)
{
  constexpr auto size = 100u;
  block_ = pdmpmt_block_ulong_alloc(size);
  // validity check
  ASSERT_TRUE(block_.data) << "block data must not be NULL";
  // expected size
  EXPECT_EQ(size, block_.size);
}

/**
 * Allocate some zeroed memory for the ulong block.
 */
TEST_F(UlongBlockTest, CallocTest)
{
  constexpr auto size = 128u;
  block_ = pdmpmt_block_ulong_calloc(size);
  // validity check
  ASSERT_TRUE(block_.data) << "block data must not be NULL";
  // expected size
  EXPECT_EQ(size, block_.size);
  // data elements all expected to be empty
  EXPECT_EQ(0, block_.data[0]);
  EXPECT_EQ(0, block_.data[size - 1]);
}

}  // namespace
