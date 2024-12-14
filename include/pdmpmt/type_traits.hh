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
#include <utility>

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
 * SFINAE helper for types that can be streamed to a `std::ostream`.
 *
 * @tparam T type
 */
template <typename T>
using ostreamable_t = std::enable_if_t<is_ostreamable_v<T>>;

/**
 * Traits helpers to indicate that a type is deferenceable.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_indirectly_readable : std::false_type {};

/**
 * True specialization for types that are dereferenceable.
 *
 * @tparam T type
 */
template <typename T>
struct is_indirectly_readable<T, std::void_t<decltype(*std::declval<T>())>>
  : std::true_type {};

/**
 * Indicate that a type is dereferenceable.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_indirectly_readable_v = is_indirectly_readable<T>::value;

/**
 * Traits helper to indicate that a type is equality comparable.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_equality_comparable : std::false_type {};

/**
 * True specialization for types that are equality comparable.
 *
 * @tparam T type
 */
template <typename T>
struct is_equality_comparable<
  T, std::void_t<decltype(std::declval<T>() == std::declval<T>())> >
  : std::true_type {};

/**
 * Traits helper to indicate that a type is equality comparable.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

/**
 * Indicate that a type is pre-incrementable.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_pre_incrementable : std::false_type {};

/**
 * True specialization for pre-incrementable types.
 *
 * @tparam T type
 */
template <typename T>
struct is_pre_incrementable<T, std::void_t<decltype(++std::declval<T>())> >
  : std::true_type {};

/**
 * Indicate that a type is pre-incrementable.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_pre_incrementable_v = is_pre_incrementable<T>::value;

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

/**
 * SFINAE helper for *UniformRandomBitGenerator* types.
 *
 * @tparam T type
 */
template <typename T>
using uniform_random_bit_generator_t = std::enable_if_t<
  is_uniform_random_bit_generator_v<T>
>;

}  // namespace pdmpmt

#endif  // PDMPMT_TYPE_TRAITS_HH_
