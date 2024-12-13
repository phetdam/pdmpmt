/**
 * @file ostream.hh
 * @author Derek Huang
 * @brief C++ header for output stream helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_OSTREAM_HH_
#define PDMPMT_OSTREAM_HH_

#include <type_traits>

#include "pdmpmt/type_traits.hh"

namespace pdmpmt {

namespace detail {

/**
 * Traits helper providing conditions for the `write` input iterator type.
 *
 * @tparam T type
 */
template <typename T, typename = void, typename = void>
struct is_write_input_iterator : std::false_type {};

/**
 * True specialization that satisfies a few *LegacyInputIterator* requirements.
 *
 * The satisfied *LegacyIterator* requirements are the following:
 *
 * 1. Copy constructible
 * 2. Copy assignable
 * 3. Destructible (or pseudo-destructible)
 * 4. Swappable
 * 5. Indirectly readable
 * 6. Pre-incrementable
 *
 * The satisfied *LegacyInputIterator* requirements are the following:
 *
 * 1. Equality comparable
 *
 * Furthermore, the dereferenced value must decay to a streamable type.
 *
 * @tparam T type
 */
template <typename T>
struct is_write_input_iterator<
  T,
  std::enable_if_t<
    // LegacyIterator requirements
    std::is_copy_constructible_v<T> &&
    std::is_copy_assignable_v<T> &&
    std::is_destructible_v<T> &&
    std::is_swappable_v<T> &&
    is_indirectly_readable_v<T> &&
    is_pre_incrementable_v<T> &&
    // LegacyInputIterator requirements
    is_equality_comparable_v<T> &&
    // reference-removed cv-unqualified dereferenced type is streamable
    is_ostreamable_v<
      std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<T>())>>
    >
  > > : std::true_type {};

/**
 * SFINAE helper for `write`.
 *
 * @tparam T type
 */
template <typename T>
using write_input_iterator_t = std::enable_if_t<is_write_input_iterator<T>::value>;

}  // namespace detail

/**
 * Write any flat range to an output stream.
 *
 * The dereferenced type must be insertable into a stream.
 *
 * @tparam It Iterator type weakly satisfying *LegacyInputIterator*
 *
 * @param begin Iterator to first element
 * @param end Iterator to one past the last element
 */
template <typename It, typename = detail::write_input_iterator_t<It>>
void write(std::ostream& out, It begin, It end)
{
  out << '[';
  for (auto it = begin; !(it == end); ++it) {
    // note: in C++20 operator!= is synthesized from operator==
    if (!(it == begin))
      out << ", ";
    out << *it;
  }
  out << ']';
}

}  // namespace pdmpmt

#endif  // PDMPMT_OSTREAM_HH_
