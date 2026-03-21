/**
 * @file file_mutex_mt_test.cc
 * @author Derek Huang
 * @brief C++ program testing `file_mutex` thread synchronization
 * @copyright MIT License
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include "pdmpmt/file_mutex.hh"

int main()
{
  // thread count
  // TODO: enable this to be specified through command-line later
  constexpr auto n_threads = 250u;
  // set of thread IDs collected during run
  std::unordered_set<std::thread::id> tids;
  // get lockfile path + ensure any previous lockfile is removed
  auto path = std::filesystem::temp_directory_path() / "file_mutex_mt_test.lock";
  if (std::filesystem::remove(path))
    std::cout << "removed " << path << std::endl;
  std::cout << "target: " << path << std::endl;
  // create mutex
  pdmpmt::file_mutex mut{path};
  // launch threads inserting IDs into tids
  std::cout << "spawning " << n_threads << " threads... " << std::flush;
  {
    std::vector<std::thread> threads(n_threads);
    // start threads
    for (auto& thread : threads)
      thread = std::thread{
        [&mut, &tids]
        {
          std::lock_guard _{mut};
          tids.insert(std::this_thread::get_id());
        }
      };
    // join
    for (auto& thread : threads)
      thread.join();
  }
  std::cout << "done" << std::endl;
  // tids should have size n_threads
  if (tids.size() == n_threads) {
    std::cout << "result:   OK"  << std::endl;
    return EXIT_SUCCESS;
  }
  else {
    std::cout << "result: FAIL (expected " << n_threads << " != actual " <<
      tids.size() << std::endl;
    return EXIT_FAILURE;
  }
}
