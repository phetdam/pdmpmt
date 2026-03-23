/**
 * @file file_mutex_mt_test.cc
 * @author Derek Huang
 * @brief C++ program testing `file_mutex` thread synchronization
 * @copyright MIT License
 */

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <vector>

#include "pdmpmt/cpu_times.hh"
#include "pdmpmt/file_mutex.hh"
#include "pdmpmt/scoped_timer.hh"
#include "pdmpmt/warnings.h"

namespace {

// default + max number of threads
constexpr auto n_threads_default = 100u;
constexpr auto n_threads_max = (std::numeric_limits<unsigned>::max)();
// program name + usgae
const auto progname = std::filesystem::path{__FILE__}.stem().string();
const auto program_usage = "Usage: " + progname + " [-h] [-n THREADS]\n"
  "\n"
  "Test file_mutex synchronization of multithreaded container insertion.\n"
  "\n"
  "This program launches the specified number of threads using std::thread\n"
  "that attempt to acquire a file_mutex and insert into a std::unordered_set.\n"
  "The total number of thread IDs in the set should equal the thread count.\n"
  "\n"
  "Options:\n"
  "  -h, --help         Print this usage\n"
  "  -n THREADS         Number of threads to start (default " +
    std::to_string(n_threads_default) + ")";

/**
 * Command-line options struct.
 *
 * @param help Print help text
 * @param n_threads Number of threads to launch
 */
struct cli_options {
  bool help = false;
  unsigned n_threads = n_threads_default;
};

/**
 * Parse incoming command-line arguments.
 *
 * @param opts Command-line options struct
 * @param argc Argument count from `main()`
 * @param argv Argument vector from `main()`
 * @returns `true` on success, `false` on error
 */
bool parse_args(cli_options& opts, int argc, char** argv)
{
  for (int i = 1; i < argc; i++) {
    // convenience view
    std::string_view arg{argv[i]};
    // -h, --help
    if (arg == "-h" || arg == "--help") {
      opts.help = true;
      return true;
    }
    // -n THREADS
    else if (arg == "-n") {
      // missing argument
      if (++i >= argc) {
        std::cerr << "Error: -n THREADS: missing thread count" << std::endl;
        return false;
      }
      // otherwise, attempt to convert
      try {
        auto n_threads = std::stoul(argv[i]);
        // cannot be zero or exceed the max value
        if (!n_threads)
          throw std::invalid_argument{"thread count must be positive"};
        if (n_threads > n_threads_max)
          throw std::out_of_range{
            "thread count exceeds maximum " + std::to_string(n_threads_max)
          };
        // narrow to assign value
        opts.n_threads = static_cast<unsigned>(n_threads);
      }
      catch (const std::exception& exc) {
        std::cerr << "Error: -n THREADS: exception: " << exc.what() << std::endl;
        return false;
      }
    }
    // unknown argument
    else {
      std::cerr << "Error: Unknown option " << arg << std::endl;
      return false;
    }
  }
  // done
  return true;
}

}  // namespace

int main(int argc, char** argv)
{
  // parse command-line arguments
  cli_options opts;
  if (!parse_args(opts, argc, argv))
    return EXIT_FAILURE;
  // print help
  if (opts.help) {
    std::cout << program_usage << std::endl;
    return EXIT_SUCCESS;
  }
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
  std::cout << "spawning " << opts.n_threads << " threads... " << std::flush;
  // time work within braced context
  // note: using fractional seconds to emulate time(1)
  pdmpmt::cpu_times<double> ttime;
  {
    pdmpmt::scoped_timer _{ttime};
    std::vector<std::thread> threads(opts.n_threads);
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
// suppress MSVC C5219
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(5219)
  std::cout << ttime << std::endl;
PDMPMT_MSVC_WARNING_POP()
  // tids should have size n_threads
  if (tids.size() == opts.n_threads) {
    std::cout << "result:   OK"  << std::endl;
    return EXIT_SUCCESS;
  }
  else {
    std::cout << "result: FAIL (expected " << opts.n_threads << " != actual " <<
      tids.size() << std::endl;
    return EXIT_FAILURE;
  }
}
