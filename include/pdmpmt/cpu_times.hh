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
 * @note The class is intentionally modeled after `std::chrono::duration`.
 *
 * @tparam T Arithmetic or arithmetic-like tick type
 * @tparam P `std::ratio` representing the tick period
 */
template <typename T, typename P = std::ratio<1>>
class cpu_times {
public:
  using duration = std::chrono::duration<T, P>;
  using tick_type = typename duration::rep;
  using period = typename duration::period;

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
  constexpr cpu_times(duration real, duration user, duration sys)
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
  duration real_;
  duration user_;
  duration sys_;
};

// user-defined deduction guide for CTAD
template <typename T, typename P>
cpu_times(
  std::chrono::duration<T, P>,
  std::chrono::duration<T, P>,
  std::chrono::duration<T, P>) -> cpu_times<T, P>;

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
struct is_cpu_times<cpu_times<T, std::ratio<N, D>> > : std::true_type {};

/**
 * Indicate if a type is a valid `cpu_times<T>`.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_cpu_times_v = is_cpu_times<T>::value;

namespace detail {

/**
 * Return the SI time unit suffix corresponding to `std::ratio` seconds.
 *
 * This function uses seconds as the base time unit and treats `std::ratio`
 * types as fractional units of a second, e.g. `std::milli` would result in
 * `"ms"` being returned. If there is no mapping, `"???"` is returned.
 *
 * @tparam T `std::ratio` specialization
 */
template <typename T>
constexpr auto time_suffix() noexcept
{
  if constexpr (std::is_same_v<T, std::nano>)
    return "ns";
  else if constexpr (std::is_same_v<T, std::micro>)
    return "us";
  else if constexpr (std::is_same_v<T, std::milli>)
    return "ms";
  else if constexpr (std::is_same_v<T, std::ratio<1>>)
    return "s";
  else if constexpr (std::is_same_v<T, std::ratio<60>>)
    return "min";
  else if constexpr (std::is_same_v<T, std::ratio<3600>>)
    return "h";
  else
    return "???";
}

}  // namespace detail

/**
 * Stream the `cpu_times` structure to an output stream.
 *
 * The format is `"real: <wall time>, user: <user CPU>, sys: <kernel CPU>"`.
 *
 * @tparam T Tick period type
 * @tparam P `std::ratio` tick period
 *
 * @param out Output stream
 * @param times CPU times structure
 */
template <typename T, typename P>
inline auto& operator<<(std::ostream& out, const cpu_times<T, P>& times)
{
  return out <<
    "real: " << times.real().count() << detail::time_suffix<P>() << ", " <<
    "user: " << times.user().count() << detail::time_suffix<P>() << ", " <<
    "sys: " << times.sys().count() << detail::time_suffix<P>();
}

}  // namespace pdmpmt

#endif  // PDMPMT_CPU_TIMES_HH_
