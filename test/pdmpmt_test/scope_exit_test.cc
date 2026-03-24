/**
 * @file scope_exit_test.cc
 * @author Derek Huang
 * @brief `scope_exit.hh` unit tests
 * @copyright MIT License
 */

#include "pdmpmt/scope_exit.hh"

#include "pdmpmt/features.h"  // included early for PDMPMT_HAS_GMOCK

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#if PDMPMT_HAS_GMOCK
#include <gmock/gmock.h>
#endif  // PDMPMT_HAS_GMOCK
#include <gtest/gtest.h>

namespace {

/**
 * `scope_exit` test fixture.
 */
class ScopeExitTest : public ::testing::Test {};

/**
 * Test that `scope_exit` is not copy-constructible.
 */
TEST_F(ScopeExitTest, NoCopyTest)
{
  EXPECT_FALSE(std::is_copy_constructible_v<pdmpmt::scope_exit>);
}

/**
 * Test that `scope_exit` is not copy-assignable.
 */
TEST_F(ScopeExitTest, NoCopyAssignTest)
{
  EXPECT_FALSE(std::is_copy_assignable_v<pdmpmt::scope_exit>);
}

/**
 * Test that `scope_exit` is move-constructible.
 */
TEST_F(ScopeExitTest, MoveTest)
{
  // original scope_exit with no-op callable
  pdmpmt::scope_exit s1{[] {}};
  // move from s1
  pdmpmt::scope_exit s2{std::move(s1)};
  // s1 should not be active
  EXPECT_FALSE(s1);
}

/**
 * Test that `scope_exit` is not move-assignable.
 */
TEST_F(ScopeExitTest, NoMoveAssignTest)
{
  EXPECT_FALSE(std::is_move_assignable_v<pdmpmt::scope_exit>);
}

/**
 * Test that a single `scope_exit` works as expected from a nested scope.
 */
TEST_F(ScopeExitTest, SingleTest)
{
  // success indicator
  bool done = false;
  // run scope guard and check
  {
    pdmpmt::scope_exit _{[&done] { done = true; }};
  }
  EXPECT_TRUE(done);
}

/**
 * Test that the `scope_exit` works in the correct order.
 */
TEST_F(ScopeExitTest, OrderTest)
{
  // vector to push values into
  std::vector<int> res;
  // lambda factory for the scope guard callable that emplaces an int into res
  auto make_action = [&res](int v)
  {
    return [&res, v] { res.push_back(v); };
  };
  // scope guards. on exit, res should have {1, 2, 3, 4}
  {
    pdmpmt::scope_exit s1{make_action(4)};
    pdmpmt::scope_exit s2{make_action(3)};
    pdmpmt::scope_exit s3{make_action(2)};
    pdmpmt::scope_exit s4{make_action(1)};
  }
  // check
#if PDMPMT_HAS_GMOCK
  EXPECT_THAT(res, ::testing::Pointwise(::testing::Eq(), {1, 2, 3, 4}));
#else
  EXPECT_EQ((std::vector<int>{1, 2, 3, 4}), res);
#endif  // !PDMPMT_HAS_GMOCK
}

/**
 * Test that the `scope_exit` swallows its callable's exceptions.
 */
TEST_F(ScopeExitTest, NoThrowTest)
{
  // lambda for scoping and macro safety
  auto action = []
  {
    pdmpmt::scope_exit _{[] { throw std::runtime_error{"intentional"}; }};
  };
  EXPECT_NO_THROW(action());
}

/**
 * Test that `scope_exit::release()` works as expected.
 */
TEST_F(ScopeExitTest, ReleaseTest)
{
  // should be left false
  bool done = false;
  // create + release scope guard
  {
    pdmpmt::scope_exit s1{[&done] { done = true; }};
    s1.release();
  }
  // no change expected
  EXPECT_FALSE(done);
}

}  // namespace
