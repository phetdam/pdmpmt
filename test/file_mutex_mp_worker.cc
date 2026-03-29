/**
 * @file file_mutex_mp_worker.cc
 * @author Derek Huang
 * @brief C++ worker program testing `file_mutex` process synchronization
 */

#include <cstdlib>
#include <fstream>
#include <mutex>

#include "pdmpmt/config/paths.hh"
#include "pdmpmt/file_mutex.hh"

// note: file_mutex_mp_master is responsible for launching the worker process
int main()
{
  // lockfile path
  auto lock_path = pdmpmt::config::library_dir() / "file_mutex_mp_test.lock";
  // create + acquire mutex
  pdmpmt::file_mutex mut{lock_path};
  std::unique_lock _{mut};
  // target file path
  auto file_path = pdmpmt::config::library_dir() / "file_mutex_mp_test.out";
  // open + read count
  std::fstream fs{file_path};
  auto count = 0u;
  fs >> count;
  // rewind, write incremented count, and finish
  fs.seekg(0);
  fs << ++count << std::endl;
  return EXIT_SUCCESS;
}
