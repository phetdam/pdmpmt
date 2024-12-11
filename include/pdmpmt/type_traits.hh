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

/**
 * Traits type to model the *UniformRandomBitGenerator* named concept.
 *
 * @tparam T type
 */
template <
  typename T,
  typename = void,
  typename = void,
  typename = void,
  typename = void>
struct is_uniform_random_bit_generator : std::false_type {};

/**
 * True specialization for types that satisfy *UniformRandomBitGenerator*.
 *
 * @tparam T type
 */
template <typename T>
struct is_uniform_random_bit_generator<
  T,
  // result_type member should be an unsigned type
  std::enable_if_t<std::is_unsigned_v<typename T::result_type>>,
  // min() and max() static functions should be defined and return result_type
  std::enable_if_t<std::is_same_v<decltype(T::min()), typename T::result_type>>,
  std::enable_if_t<std::is_same_v<decltype(T::max()), typename T::result_type>>,
  // invocation returns result_type
  std::enable_if_t<
    std::is_same_v<decltype(std::declval<T>()()), typename T::result_type>
  > > : std::true_type {};

/**
 * Indicate that a type satisfies *UniformRandomBitGenerator*.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool
is_uniform_random_bit_generator_v = is_uniform_random_bit_generator<T>::value;

}  // namespace pdmpmt

#endif  // PDMPMT_TYPE_TRAITS_HH_
