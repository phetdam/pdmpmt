/**
 * @file cpu_times.hh
 * @author Derek Huang
 * @brief C++ header for system timing information classes
 * @copyright MIT License
 */

#ifndef PDMPMT_CPU_TIMES_HH_
#define PDMPMT_CPU_TIMES_HH_

#include <chrono>
#include <cstdint>
#include <ostream>
#include <ratio>
#include <type_traits>

namespace pdmpmt {

/**
 * System timing results class suitable for storing the results of time(1).
 *
 * This class represents the CPU timing statistics that the time(1) program or
 * the Bash `time` command returns by default, which are the real (wall) time,
 * user CPU time, and system (kernel) CPU time.
 *
 * @tparam T `std::chrono::duration` specialization
 */
template <typename T>
class cpu_times {
public:
  using duration = T;
  using tick_type = typename T::rep;
  using period = typename T::period;

  /**
   * Default ctor.
   */
  constexpr cpu_times() = default;

  /**
   * Ctor.
   *
   * @param real Real elapsed time, also known as "wall time"
   * @param user CPU time spent in user code
   * @param sys CPU time spent in the system (kernel)
   */
  constexpr cpu_times(T real, T user, T sys)
    : real_{real}, user_{user}, sys_{sys}
  {}

  /**
   * Return the elapsed wall time.
   */
  constexpr auto& real() const noexcept { return real_; }

  /**
   * Return the elapsed user CPU time.
   */
  constexpr auto& user() const noexcept { return user_; }

  /**
   * Return the elapsed system CPU time.
   */
  constexpr auto& sys() const noexcept { return sys_;}

private:
  T real_;
  T user_;
  T sys_;
};

/**
 * Traits type for `cpu_times<T>`.
 *
 * @tparam T type
 */
template <typename T>
struct is_cpu_times : std::false_type {};

/**
 * Partial specialization for `cpu_times<T>`.
 *
 * @tparam T Tick type
 * @tparam N `std::ratio` numerator
 * @tparam D `std::ratio` denominator
 */
template <typename T, std::intmax_t N, std::intmax_t D>
struct is_cpu_times<cpu_times<std::chrono::duration<T, std::ratio<N, D>>> >
  : std::true_type {};

/**
 * Indicate if a type is a valid `cpu_times<T>`.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_cpu_times_v = is_cpu_times<T>::value;

namespace detail {

/**
 * Return the time unit string literal for the given `std::chrono::duration`.
 *
 * For example, `std::chrono::milliseconds` would map to `"ms"`. If the
 * duration has no mapping, `"???"` is returned.
 *
 * @tparam T `std::chrono::duration` specialization
 */
template <typename T>
constexpr auto time_suffix() noexcept
{
  // std::ratio type
  using period = typename T::period;
  // branch
  if constexpr (std::is_same_v<period, std::nano>)
    return "ns";
  else if constexpr (std::is_same_v<period, std::micro>)
    return "us";
  else if constexpr (std::is_same_v<period, std::milli>)
    return "ms";
  else if constexpr (std::is_same_v<period, std::ratio<1>>)
    return "s";
  else if constexpr (std::is_same_v<period, std::ratio<60>>)
    return "min";
  else if constexpr (std::is_same_v<period, std::ratio<3600>>)
    return "h";
  else
    return "???";
}

}  // namespace detail

/**
 * Stream the `cpu_times` structure to an output stream.
 *
 * @tparam T `std::chrono::duration` specialization
 *
 * @param out Output stream
 * @param times CPU times structure
 */
template <typename T>
inline auto& operator<<(std::ostream& out, const cpu_times<T>& times)
{
  return out <<
    "real: " << times.real().count() << detail::time_suffix<T>() << ", " <<
    "user: " << times.user().count() << detail::time_suffix<T>() << ", " <<
    "sys: " << times.sys().count() << detail::time_suffix<T>();
}

}  // namespace pdmpmt

#endif  // PDMPMT_CPU_TIMES_HH_
