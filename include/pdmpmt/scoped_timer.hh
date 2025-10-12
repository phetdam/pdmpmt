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
 */
class scoped_timer {
public:
  using clock_type = std::chrono::steady_clock;
  using duration = clock_type::duration;
  using time_point = std::chrono::time_point<clock_type>;

  /**
   * Ctor.
   *
   * Marks the starting time point.
   *
   * @param out Duration value to update on scope exit
   */
  scoped_timer(duration& out) : out_{out}
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
    out_ = clock_type::now() - begin_;
  }

private:
  time_point begin_;
  duration& out_;
};

}  // namespace pdmpmt

#endif  // PDMPMT_SCOPED_TIMER_H_
