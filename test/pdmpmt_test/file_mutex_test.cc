/**
 * @file file_mutex_test.cc
 * @author Derek Huang
 * @brief `file_mutex.hh` unit tests
 * @copyright MIT License
 */

#include "pdmpmt/file_mutex.hh"

#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

namespace {

/**
 * `file_mutex` test fixture.
 */
class FileMutexTest : public ::testing::Test {
protected:
  /**
   * Return a suitable file path for a lockfile given the test name.
   *
   * THe lockfile will be created in `std::filesystem::temp_directory_path()`
   * and have the `FileMutexTest.` prefix and end with the extension `.lock`.
   *
   * @param name Test name, e.g. the second parameter of `TEST_F()`
   */
  static auto lockfile_path(std::string name)
  {
    using std::filesystem::temp_directory_path;
    return temp_directory_path() / ("FileMutexTest." + name + ".lock");
  }
};

/**
 * Test that `file_mutex` is not copy-constructible.
 */
TEST_F(FileMutexTest, NoCopyTest)
{
  EXPECT_FALSE(std::is_copy_constructible_v<pdmpmt::file_mutex>);
}

/**
 * Test that `file_mutex` is not copy-assignable.
 */
TEST_F(FileMutexTest, NoCopyAssignTest)
{
  EXPECT_FALSE(std::is_copy_assignable_v<pdmpmt::file_mutex>);
}

/**
 * Test that `file_mutex` is not move-constructible.
 */
TEST_F(FileMutexTest, NoMoveTest)
{
  EXPECT_FALSE(std::is_move_constructible_v<pdmpmt::file_mutex>);
}

/**
 * Test that `file_mutex` is not move-assignable.
 */
TEST_F(FileMutexTest, NoMoveAssignTest)
{
  EXPECT_FALSE(std::is_move_assignable_v<pdmpmt::file_mutex>);
}

/**
 * Test a simple use-case of `file_mutex` to synchronize a couple threads.
 */
TEST_F(FileMutexTest, ThreadSyncTest)
{
  // path to lockfile for this test
  auto path = lockfile_path("ThreadSyncTest");
  // ensure cleaned up + create file mutex
  std::filesystem::remove(path);
  pdmpmt::file_mutex mut{path};
  // counter to update to 100
  auto count = 0u;
  // thread task to update count 50 times. we purposefully have the thread
  // release the lock with each iteration to force contention
  auto task = [&count, &mut]
  {
    for (auto i = 0u; i < 50; i++) {
      std::lock_guard _{mut};
      count++;
    }
  };
  // launch 2 threads that alternate to update the counter 50 times
  std::thread t1{task};
  std::thread t2{task};
  t1.join();
  t2.join();
  // counter should be 100
  EXPECT_EQ(100u, count);
}

}  // namespace
