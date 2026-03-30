/**
 * @file file_mutex_mp_master.cc
 * @author Derek Huang
 * @brief C++ master program testing `file_mutex` process synchronization
 * @copyright MIT License
 */

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <errhandlingapi.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <system_error>
// Win32 implementation needs PROCESS_INFORMATION vector
#ifdef _WIN32
#include <vector>
#endif  // _WIN32

#include "pdmpmt/config/paths.hh"
#include "pdmpmt/file_mutex.hh"
#include "pdmpmt/scoped_timer.hh"

namespace {

// default + max number of worker processes
constexpr auto n_workers_default = 100u;
constexpr auto n_workers_max = (std::numeric_limits<unsigned>::max)();
// program name and usage
const auto progname = std::filesystem::path{__FILE__}.stem().string();
const auto program_usage = "Usage: " + progname + " [-h] [-n WORKERS]\n"
  "\n"
  "Test file_mutex synchronization of multiprocess shared file writing.\n"
  "\n"
  "This program will launch the spec ified number of worker processes which\n"
  "will all synchronize on the same file_mutex and update a counter in a file.\n"
  "The final counter value in the file should equal the number of workers.\n"
  "\n"
  "Options:\n"
  "  -h, --help         Print this usage\n"
  "  -n WORKERS         Number of worker processes to start (default " +
    std::to_string(n_workers_default) + ")";

/**
 * Command-line options struct.
 *
 * @param help Print usage
 * @param workers Number of worker processes to launch
 */
struct cli_options {
  bool help = false;
  unsigned workers = n_workers_default;
};

// TODO: parse options with parse_args()

// path to worker process
// note: need .exe suffix for full path for Win32 CreateProcessW()
const auto worker_path = pdmpmt::config::library_dir() / "file_mutex_mp_worker"
#ifdef _WIN32
".exe"
#endif  // _WIN32
;

/**
 * Launch the specified number of worker processes and wait until done.
 *
 * Each worker process will open the counter file, read + increment the count,
 * and then terminate. This function blocks until all workers are done.
 *
 * On Windows `CreateProcessW()` is used while Linux uses fork + `execv()`.
 *
 * @param n_workers Number of workers to launch
 */
void launch_workers(unsigned n_workers)
{
#if defined(_WIN32)
  // defaulted startup info for each process
  STARTUPINFOW start_info{};
  start_info.cb = sizeof start_info;
  // vector of process information structs
  // note: using INVALID_HANDLE_VALUE to indicate uninitialized structs
  std::vector<PROCESS_INFORMATION> proc_infos(n_workers, {INVALID_HANDLE_VALUE});
  // guard to ensure all process handles are closed on scope exit
  pdmpmt::scope_exit proc_guard{
    [&proc_infos]
    {
      for (auto& info : proc_infos)
        // note: we use the hProcess value as a sentinel for both hProcess and
        // hThread so we don't need to check hThread
        if (info.hProcess != INVALID_HANDLE_VALUE)
          CloseHandle(info.hProcess), CloseHandle(info.hThread);
    }
  };
  // create worker processes
  // note: using CreateProcessW() so std::filesystem::path::c_str() can be used
  for (auto& info : proc_infos)
    if (
      !CreateProcessW(
        worker_path.c_str(),  // process to launch
        nullptr,              // lpCommandLine
        nullptr,              // lpProcessAttributes
        nullptr,              // lpThreadAttributes
        FALSE,                // do not inherit handles
        0,                    // default creation flags
        nullptr,              // lpEnvironment (using parent's environment)
        nullptr,              // lpCurrentDirectory
        &start_info,
        &info                 // output: gets process + thread handles
      )
    )
      throw std::system_error{
        static_cast<int>(GetLastError()), std::system_category(),
        "CreateProcessW() error"
      };
  // wait on each process handle until done
  // note: if we use WaitForMultipleObjects() we'd have to do so groups of
  // MAXIMUM_WAIT_OBJECTS which is currently 64
  for (const auto& info : proc_infos)
    switch (WaitForSingleObject(info.hProcess, INFINITE)) {
    // success
    case WAIT_OBJECT_0:
      break;
    // failed
    // note: we could also just explicitly handle WAIT_FAILED as WAIT_TIMEOUT
    // shouldn't happen (we block indefinitely) and hProcess is not a mutex so
    // WAIT_ABANDONED also should not happen. but default is safer
    default:
      throw std::system_error{
        static_cast<int>(GetLastError()), std::system_category(),
        "WaitForMultipleObjects() error"
      };
    }
#else
  // arg list for exec
  const char* argv[] = {worker_path.c_str(), nullptr};
  // launch
  for (auto i = 0u; i < n_workers; i++) {
    switch (fork()) {
    // error
    case -1:
      throw std::system_error{errno, std::system_category(), "fork() error"};
    // child
    case 0:
      if (execv(worker_path.c_str(), const_cast<char* const*>(argv)) < 0)
        throw std::system_error{errno, std::system_category(), "execv() error"};
    // otherwise continue
    default:
      break;
    }
  }
  // block for all children
  int status;
  while (wait(&status) != -1);
  // handle errors
  switch (errno) {
  // no more children so done
  case ECHILD:
    break;
  // otherwise handle actual error
  default:
    throw std::system_error{{errno, std::system_category()}, "wait() error"};
  }
#endif  // defined(_WIN32)
}

}  // namespace

int main(int argc, char** /*argv*/)
{
  // command-line options
  // TODO: parse using parse_args()
  cli_options opts;
  // FIXME: always print usage if any extra arguments are given
  if (argc > 1) {
    std::cout << program_usage << std::endl;
    return EXIT_SUCCESS;
  }
  // lockfile path
  auto lock_path = pdmpmt::config::library_dir() / "file_mutex_mp_test.lock";
  std::cout << "target: " << lock_path << std::endl;
  // file mutex
  pdmpmt::file_mutex mut{lock_path};
  // attempt to lock the mutex. if failed, it means another instance is holding
  // the lock, or the lock was previously lost due to a failed run
  {
    std::unique_lock lock{mut};
    if (!lock) {
      std::cerr << "error: " << lock_path << " lockfile exists" << std::endl;
      return EXIT_FAILURE;
    }
  }
  // target file path
  auto file_path = pdmpmt::config::library_dir() / "file_mutex_mp_test.out";
  std::cout << "output: " << file_path << std::endl;
  // write zero to the file
  {
    std::ofstream ofs{file_path};
    ofs << 0u << std::endl;
  }
  // launch workers and wait
  launch_workers(opts.workers);
  // read counter
  auto count = 0u;
  {
    std::ifstream ifs{file_path};
    ifs >> count;
  }
  // check + print
  if (count == opts.workers) {
    std::cout << "result:   OK" << std::endl;
    return EXIT_SUCCESS;
  }
  else {
    std::cout << "result: FAIL (expected " << opts.workers << " != actual " <<
      count << std::endl;
    return EXIT_SUCCESS;
  }
}
