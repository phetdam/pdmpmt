/**
 * @file format.hh
 * @author Derek Huang
 * @brief C++ header for stream and text formatting
 * @copyright MIT License
 */

#ifndef PDMPMT_FORMAT_HH_
#define PDMPMT_FORMAT_HH_

#include <ostream>
#include <type_traits>

#include "pdmpmt/warnings.h"

namespace pdmpmt {

/**
 * `std::ostream` wrapper for lowercase hex string formatting.
 */
class hex_lower_ostream_wrapper {
public:
  /**
   * Ctor.
   *
   * @param out Output stream
   */
  hex_lower_ostream_wrapper(std::ostream& out) noexcept : out_{&out} {}

  /**
   * Access a member of the output stream.
   */
  auto operator->() const noexcept
  {
    return out_;
  }

private:
  std::ostream* out_;
};

/**
 * `std::ostream` wrapper for fixed-width lowercase hex string formatting.
 */
class hex_lower_fixed_ostream_wrapper {
public:
  /**
   * Ctor.
   *
   * @param out Output stream
   */
  hex_lower_fixed_ostream_wrapper(std::ostream& out) noexcept : out_{&out} {}

  /**
   * Access a member of the output stream.
   */
  auto operator->() const noexcept
  {
    return out_;
  }

private:
  std::ostream* out_;
};

/**
 * `std::ostream` wrapper for uppercase hex string formatting.
 */
class hex_upper_ostream_wrapper {
public:
  /**
   * Ctor.
   *
   * @param out Output stream
   */
  hex_upper_ostream_wrapper(std::ostream& out) noexcept : out_{&out} {}

  /**
   * Access a member of the output stream.
   */
  auto operator->() const noexcept
  {
    return out_;
  }

private:
  std::ostream* out_;
};

/**
 * `std::ostream` wrapper for fixed-width uppercase hex string formatting.
 */
class hex_upper_fixed_ostream_wrapper {
public:
  /**
   * Ctor.
   *
   * @param out Output stream
   */
  hex_upper_fixed_ostream_wrapper(std::ostream& out) noexcept : out_{&out} {}

  /**
   * Access a member of the output stream.
   */
  auto operator->() const noexcept
  {
    return out_;
  }

private:
  std::ostream* out_;
};

/**
 * Stream manipulator tag type for lowercase hex formatting.
 */
struct hex_lower_type {};

/**
 * Stream manipulator tag type for fixed-width lowercase hex formatting.
 */
struct hex_lower_fixed_type {};

/**
 * Stream manipulator tag type for uppercase hex formatting.
 */
struct hex_upper_type {};

/**
 * Stream manupulator tag type for fixed-width uppercase hex formatting.
 */
struct hex_upper_fixed_type {};

/**
 * Tag global for output stream manipulation for lowercase hex formatting.
 */
inline constexpr hex_lower_type hex_l{};

/**
 * Tag global for output stream fixed-width lowercase hex formatting.
 *
 * This ensures that integral values are formatted without leading zeros
 * trimmed. `hex[_(lower|upper)]` trim leading zeros to match `std::hex`.
 */
inline constexpr hex_lower_fixed_type hexf_l{};

/**
 * Tag global for output stream manipulation for uppercase hex formatting.
 */
inline constexpr hex_upper_type hex_u{};

/**
 * Tag global for output stream fixed-width uppercase hex formatting.
 *
 * Ths ensures that integral values are formatted without leading zeros
 * trimmed. `hex_upper` trims leading zeros to match `std::hex` behavior.
 */
inline constexpr hex_upper_fixed_type hexf_u{};

/**
 * Tag global for output stream manipulation for lowercase hex formatting.
 *
 * This is provided as a drop-in replacement for `std::hex`.
 */
inline constexpr hex_lower_type hex{};

/**
 * Tag global for output straem fixed-width lowercase hex formatting.
 *
 * This is an alias for `hexf_l` as default formatting is lowercase.
 */
inline constexpr hex_lower_fixed_type hexf{};

/**
 * Return a stream wrapper for lowercase hex string formatting.
 *
 * This triggers the `operator<<` for the `hex_lower_stream_wrapper` that
 * performs the actual output formatting work.
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @param out Output stream
 */
inline auto operator<<(std::ostream& out, const hex_lower_type&)
{
  return hex_lower_ostream_wrapper{out};
}

/**
 * Return a stream wrapper for fixed-width lowercase hex string formatting.
 *
 * This triggers the appropriate `operator<<` that does the actual formatting.
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @param out Output stream
 */
inline auto operator<<(std::ostream& out, const hex_lower_fixed_type&)
{
  return hex_lower_fixed_ostream_wrapper{out};
}

/**
 * Return a stream wrapper for uppercase hex string formatting.
 *
 * This triggers the `operator<<` for the `hex_upper_stream_wrapper` that
 * performs the actual output formatting work.
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @param out Output stream
 */
inline auto operator<<(std::ostream& out, const hex_upper_type&)
{
  return hex_upper_ostream_wrapper{out};
}

/**
 * Return a stream wrapper for fixed-width uppercase hex string formatting.
 *
 * This triggers the appropriate `operator<<` that does the actual formatting.
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @param out Output stream
 */
inline auto operator<<(std::ostream& out, const hex_upper_fixed_type&)
{
  return hex_upper_fixed_ostream_wrapper{out};
}

namespace detail {

/**
 * Constraints type to require a `hex_(lower|upper)_ostream_wrapper`.
 *
 * @tparam T type
 */
template <typename T>
struct enable_if_hex_ostream_wrapper {};

/**
 * SFINAE helper to require a `hex_(lower|upper)_ostream_wrapper`.
 *
 * @tparam T type
 */
template <typename T>
using enable_if_hex_ostream_wrapper_t =
  typename enable_if_hex_ostream_wrapper<T>::type;

/**
 * True specialization for `hex_lower_ostream_wrapper`.
 */
template <>
struct enable_if_hex_ostream_wrapper<hex_lower_ostream_wrapper> {
  using type = int;
};

/**
 * True specialization for `hex_lower_fixed_ostream_wrapper`.
 */
template <>
struct enable_if_hex_ostream_wrapper<hex_lower_fixed_ostream_wrapper> {
  using type = int;
};

/**
 * True specialization for `hex_upper_ostream_wrapper`.
 */
template <>
struct enable_if_hex_ostream_wrapper<hex_upper_ostream_wrapper> {
  using type = int;
};

/**
 * True specialization for `hex_upper_fixed_ostream_wrapper`.
 */
template <>
struct enable_if_hex_ostream_wrapper<hex_upper_fixed_ostream_wrapper> {
  using type = int;
};

/**
 * Traits helper to indicate that a type is a lowercase hex stream wrapper.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_lower_hex_ostream_wrapper_v = (
  std::is_same_v<T, hex_lower_ostream_wrapper> ||
  std::is_same_v<T, hex_lower_fixed_ostream_wrapper>
);

/**
 * Traits helper to indicate that a type is a fixed-width hex stream wrapper.
 *
 * @tparam T type
 */
template <typename T>
constexpr bool is_fixed_hex_ostream_wrapper_v = (
  std::is_same_v<T, hex_lower_fixed_ostream_wrapper> ||
  std::is_same_v<T, hex_upper_fixed_ostream_wrapper>
);

/**
 * Format the given object as a hexadecimal string.
 *
 * Endianness is handled automatically for integral types while all other types
 * are simply reinterpreted as character sequences. Lower or upper case
 * formatting is done based on the type of the hex formatting stream wrapper.
 * Whether or not leading zeros are trimmed depends on the formatter type.
 *
 * @tparam O `hex_(lower|upper)_[fixed_]ostream_wrapper`
 * @tparam T type
 *
 * @param out Output stream hex formatting wrapper
 * @param value Value to format
 */
template <typename O, typename T, enable_if_hex_ostream_wrapper_t<O> = 0>
auto& to_hex(const O& out, const T& value)
{
  // hex digits
  constexpr auto digits = []
  {
    if constexpr (is_lower_hex_ostream_wrapper_v<O>)
      return "0123456789abcdef";
    else
      return "0123456789ABCDEF";
  }();
  // buffer holding hex digits to write to stream
  char buf[2u * sizeof(T)];
  // for each byte i
  for (auto i = 0u; i < sizeof(T); i++) {
    // for integral types we shift and mask to preserve endianness
    if constexpr (std::is_integral_v<T>) {
      // shift amount in bits for ith most significant byte
      auto shift = 8u * (sizeof(T) - i - 1u);
      // shift + mask + index into digits
      buf[i + i] = digits[(value >> (shift + 4u)) & 0xF];
      buf[i + i + 1u] = digits[(value >> shift) & 0xF];
    }
    // otherwise reinterpret as char sequence
    else {
      // get ith char + shift + mask + index into digits
      auto c = *(reinterpret_cast<const char*>(&value) + i);
      buf[i + i] = digits[(c >> 4u) & 0xF];
      buf[i + i + 1u] = digits[c & 0xF];
    }
  }
// MSVC emits C4365 due to std::streamsize being signed
PDMPMT_MSVC_WARNING_PUSH()
PDMPTM_MSVC_WARNING_DISABLE(4365)
  // write buffer into stream. if not using fixed-width hex formatter skip all
  // leading 0 digits in the buffer before writing if possible
  if constexpr (std::is_integral_v<T> && !is_fixed_hex_ostream_wrapper_v<O>) {
    // find first none-'0' character and write from there
    for (auto i = 0u; i < sizeof buf; i++)
      if (buf[i] != '0')
        return out->write(buf + i, sizeof buf - i);
  }
  // otherwise write entire buffer
  return out->write(buf, sizeof buf);
PDMPMT_MSVC_WARNING_POP();
}

}  // namespace detail

/**
 * Format the given object as a lowercase hexadecimal string.
 *
 * For integral types endianness is automatically handled by shifting and
 * masking each byte starting from the most significant while all other types
 * are reinterpreted as a character sequence.
 *
 * After the formatting is done the original `std::ostream` reference is
 * returned to reset the formatting sequence. Multiple formatting requires
 * multiple uses of the `operator<<` with `hex` or `hex_l`.
 *
 * To prevent trimming leading zeros when formatting integral values the
 * fixed-width `hexf` or `hexf_l` manipulators should be used.
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @tparam T type
 *
 * @param out Output stream wrapper
 * @param value Value to format
 */
template <typename T>
auto& operator<<(const hex_lower_ostream_wrapper& out, const T& value)
{
  return detail::to_hex(out, value);
}

/**
 * Format the given object as a fixed-width lowercase hexadecimal string.
 *
 * This performs the same operations as the `operator<<` for
 * `hex_lower_ostream_wrapper` but does not trim leading zeros when formatting
 * integral values, hence the moniker of "fixed-width".
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @tparam T type
 *
 * @param out Output stream wrapper
 * @param value Value to format
 */
template <typename T>
auto& operator<<(const hex_lower_fixed_ostream_wrapper& out, const T& value)
{
  return detail::to_hex(out, value);
}

/**
 * Format the given object as an uppercase hexadecimal string.
 *
 * For integral types endianness is automatically handled by shifting and
 * masking each byte starting from the most significant while all other types
 * are reinterpreted as a character sequence.
 *
 * To prevent trimming leading zeros when formatting integral values the
 * fixed-width `hexf_u` manipulator should be used.
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @tparam T type
 *
 * @param out Output stream wrapper
 * @param value Value to format
 */
template <typename T>
auto& operator<<(const hex_upper_ostream_wrapper& out, const T& value)
{
  return detail::to_hex(out, value);
}

/**
 * Format the given object as a fixed-width uppercase hexadecimal string.
 *
 * This performs the same operations as the `operator<<` for
 * `hex_upper_ostream_wrapper` but does not trim leading zeros when formatting
 * integral values, hence the moniker of "fixed-width".
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @tparam T type
 *
 * @param out Output stream wrapper
 * @param value Value to format
 */
template <typename T>
auto& operator<<(const hex_upper_fixed_ostream_wrapper& out, const T& value)
{
  return detail::to_hex(out, value);
}

}  // namespace pdmpmt

#endif  // PDMPMT_FORMAT_HH_
