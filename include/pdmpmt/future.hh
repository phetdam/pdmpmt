/**
 * @file future.hh
 * @author Derek Huang
 * @brief C++ header for async task support
 * @copyright MIT License
 */

#ifndef PDMPMT_FUTURE_HH_
#define PDMPMT_FUTURE_HH_

#include <cstddef>
#include <functional>
#include <future>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pdmpmt {

/**
 * Traits to check if a type is a `std::future`.
 *
 * @tparam T type
 */
template <typename T>
struct is_future : std::false_type {};

/**
 * Partial specialization for a `std::future`.
 *
 * @tparam T type
 */
template <typename T>
struct is_future<std::future<T>> : std::true_type {};

/**
 * Indicate if the type is a `std::future`.
 *
 * @tparam T type
 */
template <typename T>
static constexpr bool is_future_v = is_future<T>::value;

/**
 * Obtain the result type of a `std::future`.
 *
 * For other types this is simply a no-op.
 *
 * @tparam T type
 */
template <typename T>
struct result_type {
  using type = T;
};

/**
 * Partial specialization for a `std::future<T>`.
 *
 * @tparam T type
 */
template <typename T>
struct result_type<std::future<T>> {
  using type = T;
};

/**
 * Obtain the result type for a `std::future` or return the original type.
 *
 * @tparam T type
 */
template <typename T>
using result_type_t = typename result_type<T>::type;

/**
 * Perfect forwarding wrapper for `std::future` or a value.
 *
 * This automatically ensures `get()` is called to retrieve the value of the
 * future on `std::future` non-const references *only*.
 *
 * @tparam T Value or `std::future`
 *
 * @param v Value or `std::future` to call `get()` on
 */
template <typename T>
decltype(auto) get(T&& v)
{
  if constexpr (is_future_v<std::remove_cv_t<std::remove_reference_t<T>>>)
    return v.get();
  else
    return std::forward<T>(v);
}

namespace detail {

/**
 * `std::apply` variant implementation.
 *
 * @tparam F *Callable*
 * @tparam T `std::tuple` specialization
 * @tparam Is Indices 0 through `sizeof...(Is)` - 1 for tuple indices
 */
template <typename F, typename T, std::size_t... Is>
decltype(auto) apply_impl(F&& f, T&& t, std::index_sequence<Is...>)
{
  // must have same number of indices as tuple values
  static_assert(sizeof...(Is) == std::tuple_size_v<std::decay_t<T>>);
  // invoke on potentially unwrapped values
  return std::invoke(std::forward<F>(f), get(std::get<Is>(std::forward<T>(t)))...);
}

/**
 * `std::apply` emulator that automatically blocks for `std::future` values.
 *
 * The function call operation is part of a functor so we can define the
 * `apply` customization point object to sidestep Koenig lookup as otherwise
 * `std::apply()` overloads can compete in overload resolution.
 */
struct apply_type {
  /**
   * Invoke the callable with the values held in the `std::tuple`.
   *
   * @tparam F *Callable*
   * @tparam T `std::tuple` specialization
   */
  template <typename F, typename T>
  decltype(auto) operator()(F&& f, T&& v) const
  {
    return apply_impl(
      std::forward<F>(f),
      std::forward<T>(v),
      std::make_index_sequence<std::tuple_size_v<std::decay_t<T>>>{}
    );
  }
};

}  // namespace detail

/**
 * `std::apply` emulator that blocks for `std::future` values.
 *
 * This is defined as a customization point object to sidestep Koenig lookup as
 * otherwise `std::apply()` overloads can compete in overload resolution.
 */
inline constexpr detail::apply_type apply{};

}  // namespace pdmpmt

#endif  // PDMPMT_FUTURE_HH_
