/**
 * @file scoped_timer.hh
 * @author Derek Huang
 * @brief C++ header for a simple scoped timer class
 * @copyright MIT License
 */

#ifndef PDMPMT_SCOPED_TIMER_H_
#define PDMPMT_SCOPED_TIMER_H_

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <processthreadsapi.h>
#else
#include <sys/times.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <chrono>
// need std::uint64_t for performing arithmetic on FILETIME DWORDs
#ifdef _WIN32
#include <cstdint>
#endif  // _WIN32
#include <system_error>

#include "pdmpmt/cpu_times.hh"

namespace pdmpmt {

/**
 * Simple scoped timer class.
 *
 * When the duration type is a `std::chrono::duration` this class measures the
 * wall time from creation to destruction via the difference of `steady_clock`
 * time points. However, if using a `cpu_times<T, P>` as the duration type,
 * system-specific calls are also made to obtain user and kernel CPU times,
 * which can be useful for profiling CPU consumption.
 *
 * @tparam T `std::chrono::duration` or `cpu_times<T, P>` specialization
 */
template <typename T>
class scoped_timer {
public:
  using clock_type = std::chrono::steady_clock;
  using duration = T;
  using time_point = std::chrono::time_point<clock_type>;

private:
  /**
   * Starting time point structure.
   *
   * The base specialization contains only the `time_point` member indicating
   * the beginning time point for the wall time measurement.
   *
   * @tparam T_ `std::chrono::duration`
   */
  template <typename T_>
  struct start_times {
    time_point real_;  // starting wall time point
  };

  /**
   * Partial specialization for a `cpu_times<T, P>`.
   *
   * In addition to the starting wall time point this also contains the user
   * and kernel starting time points encapsulated in a platform-specific
   * structure or structures, e.g. `tms` for Linux, `FILETIME`s for Windows.
   *
   * @tparam T_ Tick type
   * @tparam P_ Period type
   */
  template <typename T_, typename P_>
  struct start_times<cpu_times<T_, P_>> {
    time_point real_;  // starting wall time point
#if defined(_WIN32)
    FILETIME user_;    // starting user CPU time point
    FILETIME sys_;     // starting system CPU time point
#else
    tms pt_;           // starting process user + system CPU times
#endif  // !defined(_WIN32)
  };

  /**
   * Indicate if we are compiling on Windows.
   *
   * @note Currently only used to support conditional `noexcept` in ctor.
   */
#if defined(_WIN32)
  static constexpr bool win32 = true;
#else
  static constexpr bool win32 = false;
#endif  // !defined(_WIN32)

public:
  /**
   * Ctor.
   *
   * Marks the starting time point.
   *
   * @param out Duration value to update on scope exit
   */
  scoped_timer(T& out) noexcept(!is_cpu_times_v<T> || !win32) : out_{out}
  {
    st_.real_ = clock_type::now();
    // cpu_times class requires user + system CPU times
    if constexpr (is_cpu_times_v<T>) {
// on Windows use GetProcessTimes()
#if defined(_WIN32)
      // note: creation and exit time are unused + exit values are unspecified
      FILETIME ct;
      FILETIME et;
      if (!GetProcessTimes(GetCurrentProcess(), &ct, &et, &st_.sys_, &st_.user_))
        throw std::system_error{
          static_cast<int>(GetLastError()),
          std::system_category(),
          "GetProcessTimes() error"
        };
// on Linux use times()
#else
      // note: not checking error since &st_.pt_ is in current address space
      times(&st_.pt_);
#endif  // !defined(_WIN32)
    }
  }

  /**
   * Dtor.
   *
   * Marks the ending time point and updates the reference to the duration.
   */
  ~scoped_timer()
  {
    // for cpu_times<T, P> structure
    if constexpr (is_cpu_times_v<T>) {
      // first record the ending time point to preserve operation ordering
      auto now = clock_type::now();
      // now get user and kernel CPU times
#if defined(_WIN32)
      // creation + unspecified exit time are unused as usual
      FILETIME ct;
      FILETIME et;
      // we only care about the user and kernel times
      FILETIME ut;
      FILETIME kt;
      // note: discarding error here
      GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut);
#else
      tms ts;
      times(&ts);
#endif  // !defined(_WIN32)
      // on Linux we need to get the clock ticks per second. this is not needed
      // on Windows since FILETIME values are in units of 100ns
#ifndef _WIN32
      // TODO: by default, the clock ticks per second on Linux corresponds to
      // the USER_HZ constant which is typically 100 (unless configured). we
      // should be using getrusage() for microsecond-level time resolution
      auto tps = sysconf(_SC_CLK_TCK);
#endif  // _WIN32
      // now perform differences. first get wall time duration
      auto real = now - st_.real_;
// on Windows we must copy the low and high DWORDs into uint64_t since type
// punning a union, although allowed by some compilers as an extension, is not
// allowed under the C++ standard (despite Microsoft docs recommending punning)
#if defined(_WIN32)
      // user + kernel ticks computed by copying + shifting
      std::uint64_t user_ticks = ut.dwHighDateTime;
      user_ticks = (user_ticks << 32u) + ut.dwLowDateTime;
      std::uint64_t sys_ticks = kt.dwHighDateTime;
      sys_ticks = (sys_ticks << 32u) + kt.dwLowDateTime;
      // compute user + kernel durations in nanoseconds
      std::chrono::nanoseconds user_time{100 * user_ticks};
      std::chrono::nanoseconds sys_time{100 * sys_ticks};
// on Linux we need to sum the differences in the user and system times
#else
      // user + kernel ticks for self + children
      // note: take differences before addition to avoid overflow
      auto user_ticks = (
        (ts.tms_utime - st_.pt_.tms_utime) +
        (ts.tms_cutime - st_.pt_.tms_cutime)
      );
      auto sys_ticks = (
        (ts.tms_stime - st_.pt_.tms_stime) +
        (ts.tms_cstime - st_.pt_.tms_cstime)
      );
      // compute user + kernel durations in nanoseconds. this is because the
      // tick count may be less than the ticks per second
      std::chrono::nanoseconds user_time{1000000000 * user_ticks / tps};
      std::chrono::nanoseconds sys_time{1000000000 * sys_ticks / tps};
#endif  // !defined(_WIN32)
      // update cpu_times
      out_ = {
        std::chrono::duration_cast<typename T::duration>(real),
        std::chrono::duration_cast<typename T::duration>(user_time),
        std::chrono::duration_cast<typename T::duration>(sys_time)
      };
    }
    // for ordinary std::chrono::duration
    else
      out_ = std::chrono::duration_cast<T>(clock_type::now() - st_.real_);
  }

private:
  start_times<T> st_;
  T& out_;
};

}  // namespace pdmpmt

#endif  // PDMPMT_SCOPED_TIMER_H_
