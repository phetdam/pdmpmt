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
 * Traits type to indicate that a type is deferenceable.
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
 * Traits type to indicate that a type enables member access with `->`.
 *
 * For indirectly readable types `T`, an instance `v` may not necessarily allow
 * member access with `->`. This is typically only true for pointer types.
 *
 * @note Indirect readability is *not* required by this traits.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_member_accessible : std::false_type {};

/**
 * True specialization for pointers.
 *
 * Pointers of course do not implement `->` as an operator.
 *
 * @tparam T type
 */
template <typename T>
struct is_member_accessible<T, std::enable_if_t<std::is_pointer_v<T>> >
  : std::true_type {};

/**
 * True specialization for types that implement `->` as an operator.
 *
 * @tparam T type
 */
template <typename T>
struct is_member_accessible<
  T, std::void_t<decltype(std::declval<T>()->operator())> > : std::true_type {};

/**
 * Indicate that a type allows member access with `->`.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_member_accessible_v = is_member_accessible<T>::value;

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
 * Yraits helper to indicate that a type is inequaltiy comparable.
 *
 * Before C++20 where `!=` is synthesized from `==`, two instances `u`, `v` of
 * equality-comparable `T` were not comparable using `!=`.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_inequality_comparable : std::false_type {};

/**
 * True specialization for types that are inequality comparable.
 *
 * @tparam T type
 */
template <typename T>
struct is_inequality_comparable<
  T, std::void_t<decltype(std::declval<T>() != std::declval<T>())> >
  : std::true_type {};

/**
 * Indicate that a type is inequaltiy comparable.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_inequality_comparable_v = is_inequality_comparable<T>::value;

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
 * Indicate that a type is post-incrementable.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_post_incrementable : std::false_type {};

/**
 * True specialization for post-incrementable types.
 *
 * @tparam T type
 */
template <typename T>
struct is_post_incrementable<T, std::void_t<decltype(std::declval<T>()++)> >
  : std::true_type {};

/**
 * Indicate that a type is post-incrementable.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_post_incrementable_v = is_post_incrementable<T>::value;

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

/**
 * Boost-style traits type constraints type.
 *
 * This is modeled after observing the usage of `constraint_t` in Boost.Asio
 * code for SFINAE as a function instead of template parameter. For example:
 *
 * @code{.cc}
 * template <typename T>
 * auto f(T&&, constraint_t<is_something_v<T>> = 0)
 * {
 *   // ...
 * }
 * @endcode
 *
 * @tparam truth Truth condition
 */
template <bool truth>
struct constraint_type {};

/**
 * True specialization when the truth condition is `true`.
 */
template <>
struct constraint_type<true> {
  using type = int;
};

/**
 * SFINAE helper for template constraints.
 *
 * @tparam truth Truth condition
 */
template <bool truth>
using constraint_t = typename constraint_type<truth>::type;

/**
 * Traits type loosely satisfying the *LegacyIterator* named requirements.
 *
 * This is somewhat looser than the real *LegacyIterator* named requirements
 * since it does not require type members for `iterator_traits<It>`.
 *
 * @note *LegacyIterator* does *not* require the type to be post-incrementable.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_legacy_iterator : std::false_type {};

/**
 * Partial specialization for types that satisfy *LegacyIterator*.
 *
 * @tparam T type
 */
template <typename T>
struct is_legacy_iterator<
  T,
  std::enable_if_t<
    std::is_copy_constructible_v<T> &&
    std::is_copy_assignable_v<T> &&
    std::is_destructible_v<T> &&
    std::is_swappable_v<T> &&
    // *it
    is_indirectly_readable_v<T> &&
    // ++it -> T&
    std::is_same_v<decltype(++std::declval<T>()), T&>
  > > : std::true_type {};

/**
 * Indicate that a type loosely satsfies *LegacyIterator*.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_legacy_iterator_v = is_legacy_iterator<T>::value;

/**
 * Traits type loosely satisfying the *LegacyInputIterator* named requirements.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_legacy_input_iterator : std::false_type {};

/**
 * True specialization for a type satisfying *LegacyInputIterator*.
 *
 * @tparam T type
 */
template <typename T>
struct is_legacy_input_iterator<
  T,
  std::enable_if_t<
    // satisfies LegacyIterator
    is_legacy_iterator_v<T> &&
    is_equality_comparable_v<T> &&
    is_inequality_comparable_v<T> &&
    // it->member is valid (only automatically true for pointers)
    is_member_accessible_v<T> &&
    // it++
    // note: should more precisely check that *it++ convertible to value type
    is_post_incrementable_v<T>
  > > : std::true_type {};

/**
 * Indicate that a type loosely satsifies *LegacyInputIterator*.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_legacy_input_iterator_v = is_legacy_input_iterator<T>::value;

/**
 * Traits type loosely satisfying *LegacyForwardIterator* named requirements.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_legacy_forward_iterator : std::false_type {};

/**
 * True specialization for a type satisfying *LegacyForwardIterator*.
 *
 * @tparam T type
 */
template <typename T>
struct is_legacy_forward_iterator<
  T,
  std::enable_if_t<
    // satisfies LegacyInputIterator
    is_legacy_input_iterator_v<T> &&
    // it++ convertible to const T& (e.g. prvalue materialization)
    std::is_convertible_v<decltype(std::declval<T>()++), const T&> &&
    // *it++ results in a reference. this is what makes the forward iterator
    // different from the input iterator (which may yield a value only)
    std::is_reference_v<decltype(*std::declval<T>()++)>
  > > : std::true_type {};

/**
 * Indicate thata type loosely satisfies *LegacyForwardIterator*.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_legacy_forward_iterator_v = is_legacy_forward_iterator<T>::value;

/**
 * Traits type modeling a range.
 *
 * This differs from the C++20 range concept since it works with the legacy
 * iterator named concepts used in C++ prior to C++20.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_range : std::false_type {};

/**
 * True specialization for a range-like type.
 *
 * @tparam T type
 */
template <typename T>
struct is_range<
  T,
  std::enable_if_t<
    std::is_same_v<void, std::void_t<decltype(std::begin(std::declval<T>()))>> &&
    std::is_same_v<void, std::void_t<decltype(std::end(std::declval<T>()))>>
  > > : std::true_type {};

/**
 * Indicate that a type is range-like.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_range_v = is_range<T>::value;

}  // namespace pdmpmt

#endif  // PDMPMT_TYPE_TRAITS_HH_
