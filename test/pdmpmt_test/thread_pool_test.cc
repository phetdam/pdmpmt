/**
 * @file thread_pool_test.cc
 * @author Derek Huang
 * @brief `thread_pool.hh` unit tests
 * @copyright MIT License
 */

#include "pdmpmt/thread_pool.hh"

#include <chrono>
#include <cstddef>
#include <future>
#include <mutex>
#include <numeric>
#include <random>
#include <string_view>
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
    futs[i] = pool.promise(
      [dur = dist(rng)](auto i)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{dur});
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

/**
 * Alphanumeric string generator.
 *
 * The lengths of each string are uniformly distributed over an interval.
 */
class alnum_generator {
private:
  // alphanumeric characters used
  static inline constexpr const char chars[] = {
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  };

public:
  /**
   * Ctor.
   *
   * @param lo Minimum string length
   * @param hi Maximum string length
   */
  alnum_generator(std::size_t lo, std::size_t hi)
    : idist_{0u, sizeof chars - 2u},  // note: excludes trailing '\0'
      ldist_{lo, hi}
  {}

  /**
   * Return a new random alphanumeric string.
   *
   * The length is guaranteed to be in the `[lo, hi]` interval. Each invocation
   * will call `operator()` on the *UniformRandomBitGenerator* an indeterminate
   * number of times based on the length of the final string.
   *
   * @tparam Gen *UniformRandomBitGenerator*
   */
  template <typename Gen>
  auto operator()(Gen& gen)
  {
    // zeroed strnig of random length
    std::string str(ldist_(gen), '\0');
    // update with random characters + return
    for (auto& c : str)
      c = chars[idist_(gen)];
    return str;
  }

private:
  std::uniform_int_distribution<std::size_t> idist_;  // index distribution
  std::uniform_int_distribution<std::size_t> ldist_;  // length distribution
};

/**
 * Test simple `std::future` consumption using several threads.
 */
TEST_F(ThreadPoolTest, FutureThenTest)
{
  pdmpmt::thread_pool pool{4u};
  // task count
  constexpr auto n_tasks = 10u;
  // first line of futures returning strings
  std::vector<std::future<std::string>> str_tasks(n_tasks);
  // second line of futures returning string lengths
  std::vector<std::future<std::size_t>> len_tasks(n_tasks);
  // entropy source, string generator, and sleep distribution
  std::mt19937 gen{8888u};
  alnum_generator alnum_gen{5u, 16u};
  std::uniform_int_distribution udist{5u, 15u};
  // create tasks that sleep and then return the created string
  for (auto& task : str_tasks)
    task = pool.promise(
      [str = alnum_gen(gen), dur = udist(gen)]
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{dur});
        return str;
      }
    );
  // vector to hold the retrieved strings (for debugging purposes)
  std::vector<std::string> strs(n_tasks);
  // create tasks that consume the previous tasks and copy the strings out
  for (auto i = 0u; i < n_tasks; i++)
    len_tasks[i] = pool.promise(
      // note: using auto for the lambda type indirectly involves a template so
      // we can't pass such a lambda with arguments properly
      [&strs, i, dur = udist(gen)](std::string_view str)
      {
        // note: no mutex needed since each index is different
        strs[i] = str;
        // simulate work + return size
        std::this_thread::sleep_for(std::chrono::milliseconds{dur});
        return str.size();
      },
      // moved-from future that will provide the value
      std::move(str_tasks[i])
    );
  // obtain lengths from the tasks
  std::vector<std::size_t> lens(n_tasks);
  for (auto i = 0u; i < n_tasks; i++)
    lens[i] = len_tasks[i].get();
  // compute expected lengths from strings
  std::vector<std::size_t> exp_lens(n_tasks);
  for (auto i = 0u; i < n_tasks; i++)
    exp_lens[i] = strs[i].size();
  // check for equality
  EXPECT_THAT(lens, ::testing::Pointwise(::testing::Eq(), exp_lens));
}

}  // namespace
