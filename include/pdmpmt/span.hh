/**
 * @file span.hh
 * @author Derek Huang
 * @brief C++ header for a 1D XPU-compatible span
 * @copyright MIT License
 */

#ifndef PDMPMT_SPAN_HH_
#define PDMPMT_SPAN_HH_

#include <cstddef>
#include <type_traits>

#include "pdmpmt/common.h"
#include "pdmpmt/features.h"

#if PDMPMT_HAS_THRUST
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#endif  // PDMPMT_HAS_THRUST

namespace pdmpmt {

/**
 * Class for a 1D XPU-compatible span.
 *
 * This serves as a lighter version of the C++20 `std::span<T>` that is also
 * appropriately annotated for compilation in device code.
 *
 * @tparam T type
 */
template <typename T>
class span {
public:
  using value_type = T;
  // note: extra iterator type members for compatibility with older GoogleMock
  // versions that had stronger requirements for container-like types
  using iterator = T*;
  using const_iterator = const T*;

  /**
   * Ctor.
   *
   * Creates an empty span.
   */
  constexpr span() noexcept = default;

  /**
   * Ctor.
   *
   * @param data Data buffer
   * @param size Buffer element count
   */
  PDMPMT_XPU_FUNC
  constexpr span(T* data, std::size_t size) noexcept
    : data_{data}, size_{size}
  {}

private:
  /**
   * SFINAE constraint for the add-const converting ctor.s
   *
   * The member is `true` only when `T` is const-qualified and the removing the
   * const-qualification results in the same type as `U`.
   *
   * @tparam U Value type
   */
  template <typename U>
  using compatible_value_t = std::enable_if_t<
    std::is_const_v<T> && std::is_same_v<U, std::remove_const_t<T>>
  >;

public:
  /**
   * Ctor.
   *
   * Convert from another span that has the same value type but has a non-const
   * value type. This is conditionally enabled when `T` is const-qualified.
   *
   * @tparam U `T` or `std::remove_const_t<T>`
   *
   * @param other Span to convert from
   */
  template <typename U, typename = compatible_value_t<U>>
  PDMPMT_XPU_FUNC
  constexpr span(const span<U>& other) noexcept
    : data_{const_cast<T*>(other.data())}, size_{other.size()}
  {}

#if PDMPMT_HAS_THRUST
  /**
   * Ctor.
   *
   * Create a span from a Thrust host vector.
   *
   * @param vec Thrust host vecto
   */
  span(thrust::host_vector<T>& vec) noexcept
    : data_{vec.data().get()}, size_{vec.size()}
  {}

  /**
   * Ctor.
   *
   * Create a span from a Thrust device vector.
   *
   * @param vec Thrust device vector
   */
  span(thrust::device_vector<T>& vec) noexcept
    : data_{vec.data().get()}, size_{vec.size()}
  {}
#endif  // PDMPMT_HAS_THRUST

  /**
   * Return the data pointer.
   */
  PDMPMT_XPU_FUNC
  constexpr auto data() const noexcept { return data_; }

  /**
   * Return the element count.
   */
  PDMPMT_XPU_FUNC
  constexpr auto size() const noexcept { return size_; }

  /**
   * Return a reference to the `i`th element in the span.
   *
   * @param i Element index
   */
  PDMPMT_XPU_FUNC
  constexpr auto& operator[](std::size_t i) const noexcept
  {
    return data_[i];
  }

  /**
   * Return an iterator to the first element in the span.
   */
  PDMPMT_XPU_FUNC
  constexpr auto begin() const noexcept
  {
    return data_;
  }

  /**
   * Return an iterator one past the last element in the span.
   */
  PDMPMT_XPU_FUNC
  constexpr auto end() const noexcept
  {
    return data_ + size_;
  }

private:
  T* data_{};
  std::size_t size_{};
};

}  // namespace pdmpmt

#endif  // PDMPMT_SPAN_HH_
