/**
 * @file scoped_timer.hh
 * @author Derek Huang
 * @brief C++ header for a simple scoped timer class
 * @copyright MIT License
 */

#ifndef PDMPMT_SCOPED_TIMER_H_
#define PDMPMT_SCOPED_TIMER_H_

#include <chrono>

namespace pdmpmt {

/**
 * Simple scoped timer class measuring wall time.
 *
 * @tparam T `std::chrono::duration` specialization
 */
template <typename T>
class scoped_timer {
public:
  using clock_type = std::chrono::steady_clock;
  using duration = T;
  using time_point = std::chrono::time_point<clock_type>;

  /**
   * Ctor.
   *
   * Marks the starting time point.
   *
   * @param out Duration value to update on scope exit
   */
  scoped_timer(T& out) noexcept : out_{out}
  {
    begin_ = clock_type::now();
  }

  /**
   * Dtor.
   *
   * Marks the ending time point and updates the reference to the duration.
   */
  ~scoped_timer()
  {
    out_ = std::chrono::duration_cast<T>(clock_type::now() - begin_);
  }

private:
  time_point begin_;
  T& out_;
};

}  // namespace pdmpmt

#endif  // PDMPMT_SCOPED_TIMER_H_
