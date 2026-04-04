/**
 * @file alnum_generator.hh
 * @author Derek Huang
 * @brief C++ header for a random alphanumeric string generator
 * @copyright MIT License
 */

#ifndef PDMPMT_TESTING_ALNUM_GENERATOR_HH_
#define PDMPMT_TESTING_ALNUM_GENERATOR_HH_

#include <random>

namespace pdmpmt {
namespace testing {

/**
 * Alphanumeric string generator.
 *
 * The lengths of each string and the frequencies of each selected character
 * are both uniformly distributed over appropriate intervals.
 */
class alnum_generator {
private:
  // alphanumeric characters used
  static inline constexpr const char chars[] = {
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  };

public:
  /**
   * Ctor.
   *
   * @param lo Minimum string length
   * @param hi Maximum string length
   */
  alnum_generator(std::size_t lo, std::size_t hi)
    : idist_{0u, sizeof chars - 2u},  // note: excludes trailing '\0'
      ldist_{lo, hi}
  {}

  /**
   * Return a new random alphanumeric string.
   *
   * The length is guaranteed to be in the `[lo, hi]` interval. Each invocation
   * will call `operator()` on the *UniformRandomBitGenerator* an indeterminate
   * number of times based on the length of the final string.
   *
   * @tparam Gen *UniformRandomBitGenerator*
   */
  template <typename Gen>
  auto operator()(Gen& gen)
  {
    // zeroed strnig of random length
    std::string str(ldist_(gen), '\0');
    // update with random characters + return
    for (auto& c : str)
      c = chars[idist_(gen)];
    return str;
  }

private:
  std::uniform_int_distribution<std::size_t> idist_;  // index distribution
  std::uniform_int_distribution<std::size_t> ldist_;  // length distribution
};

}  // namespace testing
}  // namespace pdmpmt

#endif  // PDMPMT_TESTING_ALNUM_GENERATOR_HH_
