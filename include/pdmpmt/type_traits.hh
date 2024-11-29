/**
 * @file type_traits.hh
 * @author Derek Huang
 * @brief C++ type traits helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_TYPE_TRAITS_HH_
#define PDMPMT_TYPE_TRAITS_HH_

#include <iosfwd>
#include <type_traits>

namespace pdmpmt {

/**
 * Traits helper to indicate a type can be streamed to a `std::ostream`.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_ostreamable : std::false_type {};

/**
 * True specialization for types that can be streamed to a `std::ostream`.
 *
 * @tparam T type
 */
template <typename T>
struct is_ostreamable<
  T,
  std::void_t<decltype(std::declval<std::ostream>() << std::declval<T>())> >
  : std::true_type {};

/**
 * Indicate that a type can be streamed to a `std::ostream`.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_ostreamable_v = is_ostreamable<T>::value;

/**
 * SFINAE helper for types that can be stream to a `std::ostream`.
 *
 * @tparam T type
 */
template <typename T>
using ostreamable_t = std::enable_if_t<is_ostreamable_v<T>>;

}  // namespace pdmpmt

#endif  // PDMPMT_TYPE_TRAITS_HH_
