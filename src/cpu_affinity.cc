/**
 * @file cpu_affinity.cc
 * @author Derek Huang
 * @brief C++ program to display + set CPU affinity
 * @copyright MIT License
 */

// note: Windows requires _WIN32_WINNT >= 0x0601, which is _WIN32_WINNT_WIN7 in
// sdkddkver.h, for correct conditional compilation of some function decls
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinBase.h>
#include <processthreadsapi.h>
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // for sched_setaffinity()
#endif  // _GNU_SOURCE
#include <sched.h>
#endif  // !defined(_WIN32)

#ifndef _WIN32
#include <cerrno>
#endif  // _WIN32
#include <cstdlib>
#include <iostream>
#include <system_error>

#include "pdmpmt/cpu_set.hh"

namespace {

/**
 * Return the current logical processor number the thread is running on.
 */
auto current_processor()
{
#if defined(_WIN32)
  return GetCurrentProcessorNumber();
#else
  return sched_getcpu();
#endif  // !defined(_WIN32)
}

}  // namespace

int main()
{
  // helper to print CPU set + current logical CPU number
  auto print_info = [](const pdmpmt::cpu_set& cpus)
  {
    std::cout << cpus << " " << pdmpmt::cpu_set::fmt() << cpus <<
      " current: " << current_processor() << std::endl;
  };
  // get CPU set with current thread's process affinity + print
  auto cpus = pdmpmt::cpu_set::current();
  print_info(cpus);
  // modify + set thread affinity
  for (auto i = 0u; i < cpus.size(); i += 2u)
    cpus[i] = false;
#if defined(_WIN32)
  if (!SetThreadAffinityMask(GetCurrentThread(), cpus))
    throw std::system_error{
      static_cast<int>(GetLastError()), std::system_category(),
      "SetThreadAffinityMask() for current thread failed"
    };
#else
  if (sched_setaffinity(0, cpus.alloc_size(), cpus) < 0)
    throw std::system_error{
      {errno, std::system_category()},
      "call to sched_setaffinity() for current thread failed"
    };
#endif  // !defined(_WIN32)
  // get thread affinity again + print
  cpus = pdmpmt::cpu_set::current();
  print_info(cpus);
  // clear and repeat for each available CPU number
  for (auto i = 0u; i < cpus.size(); i++) {
    cpus = {};
    cpus[i] = true;
#if defined(_WIN32)
    // if ERROR_INVALID_PARAMETER, CPU is not online/allowed, so don't error
    if (!SetThreadAffinityMask(GetCurrentThread(), cpus)) {
      switch (GetLastError()) {
      case ERROR_INVALID_PARAMETER:
        break;
      default:
        throw std::system_error{
          static_cast<int>(GetLastError()), std::system_category(),
          "SetThreadAffinityMask() for current thread failed"
        };
      }
    }
#else
    // if EINVAL, CPU is not online/insufficient perms, so don't error
    if (sched_setaffinity(0, cpus.alloc_size(), cpus) < 0) {
      switch (errno) {
      case EINVAL:
        break;
      default:
        throw std::system_error{
          {errno, std::system_category()},
          "call to sched_setaffinity() for current thread failed"
        };
      }
    }
#endif  // !defined(_WIN32)
    // if current CPU is not i affinity switch failed
    std::cout << cpus << " " << pdmpmt::cpu_set::fmt() << cpus << " current: ";
    auto cur_cpu = current_processor();
    // note: cast to appease compiler warning about signed/unsigned mismatch
    if (i == static_cast<decltype(i)>(cur_cpu))
      std::cout << cur_cpu << std::endl;
    else
      std::cout <<
#if defined(_WIN32)
        "ERROR_INVALID_PARAMETER"
#else
        "EINVAL"
#endif  // !defined(_WIN32)
        " (no migration)" << std::endl;
  }
  // done
  return EXIT_SUCCESS;
}
