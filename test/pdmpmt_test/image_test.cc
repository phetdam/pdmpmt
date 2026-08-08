/**
 * @file image_test.cc
 * @author Derek Huang
 * @brief image.hh unit tests
 * @copyright MIT License
 */

#include "pdmpmt/image.hh"

#include <type_traits>

#include <gtest/gtest.h>

namespace {

/**
 * Test fixture for `pixel` tests.
 */
class PixelTest : public ::testing::Test {};

/**
 * Test that the `pixel::view` is not copyable.
 */
TEST_F(PixelTest, ViewNoCopyTest)
{
  EXPECT_FALSE(std::is_copy_constructible_v<pdmpmt::pixel::view>);
}

/**
 * Test that the `pixel::view` is not movable.
 */
TEST_F(PixelTest, ViewNoMoveTest)
{
  EXPECT_FALSE(std::is_move_constructible_v<pdmpmt::pixel::view>);
}

/**
 * Check that pixel assignment modifies the `pixel::view`.
 */
TEST_F(PixelTest, ViewFromPixelTest)
{
  // buffer
  pdmpmt::pixel::byte buf[4];
  buf[0] = 0xFF;
  buf[1] = 0;
  buf[2] = 0;
  buf[3] = 0;
  // create view + update using green pixel
  pdmpmt::pixel::view pv{buf};
  pv = pdmpmt::pixel::green();
  // check bytes
  EXPECT_EQ(0, buf[0]);
  EXPECT_EQ(0xFF, buf[1]);
  EXPECT_EQ(0, buf[2]);
  EXPECT_EQ(0xFF, buf[3]);
}

/**
 * Check pixel default construction.
 */
TEST_F(PixelTest, PixelDefaultTest)
{
  pdmpmt::pixel px;
  // check bytes
  EXPECT_EQ(0, px.r());
  EXPECT_EQ(0, px.g());
  EXPECT_EQ(0, px.b());
  EXPECT_EQ(0xFF, px.a());
}

/**
 * Check pixel and pixel view equality.
 */
TEST_F(PixelTest, ViewValueEqTest)
{
  // buffer
  pdmpmt::pixel::byte buf[4];
  buf[0] = 0;
  buf[1] = 0xFF;
  buf[2] = 0;
  buf[3] = 0xFF;
  // create view and compare against green pixel both ways
  pdmpmt::pixel::view pv{buf};
  auto green = pdmpmt::pixel::green();
  EXPECT_EQ(green, pv);
  EXPECT_EQ(pv, green);
}

/**
 * Check pixel and pixel view inequality.
 */
TEST_F(PixelTest, ViewValueNeTest)
{
  // buffer
  pdmpmt::pixel::byte buf[4];
  buf[0] = 0;
  buf[1] = 0;
  buf[2] = 0xFF;
  buf[3] = 0xFF;
  // create view and compara against red pixel both ways
  pdmpmt::pixel::view pv{buf};
  auto red = pdmpmt::pixel::red();
  EXPECT_NE(red, pv);
  EXPECT_NE(pv, red);
}

}  // namespace
