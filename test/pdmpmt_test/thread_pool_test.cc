/**
 * @file thread_pool_test.cc
 * @author Derek Huang
 * @brief `thread_pool.hh` unit tests
 * @copyright MIT License
 */

#include "pdmpmt/thread_pool.hh"

#include <chrono>
#include <future>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <type_traits>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

/**
 * `thread_pool` test fixture class.
 */
class ThreadPoolTest : public ::testing::Test {};

/**
 * Test that the `thread_pool` is not copyable.
 */
TEST_F(ThreadPoolTest, NoCopyTest)
{
  EXPECT_FALSE(std::is_copy_constructible_v<pdmpmt::thread_pool>);
}

/**
 * Test that the `thread_pool` is not copy assignable.
 */
TEST_F(ThreadPoolTest, NoCopyAssignTest)
{
  EXPECT_FALSE(std::is_copy_assignable_v<pdmpmt::thread_pool>);
}

/**
 * Test that the `thread_pool` is not move-constructible.
 */
TEST_F(ThreadPoolTest, NoMoveTest)
{
  EXPECT_FALSE(std::is_move_constructible_v<pdmpmt::thread_pool>);
}

/**
 * Test that the `thread_pool` is not move-assignable.
 */
TEST_F(ThreadPoolTest, NoMoveAssignTest)
{
  EXPECT_FALSE(std::is_move_assignable_v<pdmpmt::thread_pool>);
}

/**
 * Test that multiple calls to `start()` are not harmful.
 */
TEST_F(ThreadPoolTest, ConcurrentStartTest)
{
  pdmpmt::thread_pool pool{std::launch::deferred, 8u};
  // launch a bunch of threads that call start
  std::vector<std::thread> threads(64u);
  for (auto& thread : threads)
    thread = std::thread{[&pool] { pool.start(); }};
  for (auto& thread : threads)
    thread.join();
  // pool should be running
  EXPECT_TRUE(pool.running());
  // stop the pool
  pool.stop();
}

/**
 * Test that multiple calls to `stop()` are not harmful.
 */
TEST_F(ThreadPoolTest, ConcurrentStopTest)
{
  pdmpmt::thread_pool pool{8u};
  // launch a bunch of threads that call stop
  std::vector<std::thread> threads(64u);
  for (auto& thread : threads)
    thread = std::thread{[&pool] { pool.stop(); }};
  for (auto& thread : threads)
    thread.join();
  // pool should be stopped
  EXPECT_FALSE(pool.running());
}

/**
 * Test simple counter increment using several threads.
 */
TEST_F(ThreadPoolTest, CounterTest)
{
  pdmpmt::thread_pool pool{4u};
  // counter + mutex
  auto count = 0u;
  std::mutex mut;
  // post tasks that compete to increment the counter
  for (auto i = 0u; i < 100u; i++)
    pool.post(
      [&count, &mut]
      {
        for (auto i = 0u; i < 10u; i++) {
          std::lock_guard _{mut};
          count++;
        }
      }
    );
  // block until pending tasks are empty + stop
  pool.wait();
  pool.stop();
  // counter should be 1000 + number of pending tasks should be zero
  EXPECT_EQ(1000u, count);
  EXPECT_EQ(0u, pool.pending());
}

/**
 * Test simple `std::future` integration using several threads.
 */
TEST_F(ThreadPoolTest, FutureTest)
{
  pdmpmt::thread_pool pool{4u};
  // futures
  std::vector<std::future<unsigned>> futs(10u);
  // entropy source + distribution for sleep in milliseconds
  std::mt19937 rng{8888u};
  std::uniform_int_distribution dist{5u, 15u};
  // post tasks that randomly sleep and then return their index
  for (auto i = 0u; i < futs.size(); i++)
    futs[i] = pool.post(
      pdmpmt::use_future,
      [&rng, &dist](auto i)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{dist(rng)});
        return i;
      },
      // note: yes, we could capture by copy, but this is to show forwarding
      i
    );
  // block for values
  std::vector<unsigned> values(futs.size());
  std::transform(
    futs.begin(),
    futs.end(),
    values.begin(),
    [](auto& f) { return f.get(); }
  );
  // check
  std::vector<unsigned> iota(futs.size());
  std::iota(iota.begin(), iota.end(), 0u);
  // note: would be nice if we could use some value generator but GoogleTest
  // can't do container comparison without a value_type member
  EXPECT_THAT(values, ::testing::Pointwise(::testing::Eq(), iota));
}

}  // namespace
