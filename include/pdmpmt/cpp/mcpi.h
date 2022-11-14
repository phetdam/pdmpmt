/**
 * @file mcpi.h
 * @author Derek Huang
 * @brief C++ template implementation for estimating pi using Monte Carlo
 * @copyright MIT License
 */

#ifndef PDMPMT_CPP_MCPI_H_
#define PDMPMT_CPP_MCPI_H_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <vector>

#include <iostream>

namespace pdmpmt {

/**
 * Return number of samples in [-1, 1] x [-1, 1] that fall in the unit circle.
 *
 * We make a copy of the PRNG instance, otherwise its state will be changed.
 *
 * @tparam Rng *UniformRandomBitGenerator* type
 *
 * @param n_samples `N_t` number of samples to use
 * @param rng `Rng` PRNG instance
 */
template <typename N_t, typename Rng>
N_t unit_circle_samples(N_t n_samples, Rng rng)
{
  std::uniform_real_distribution udist{-1., 1.};
  // we use the heap here as stack sizes are pretty small; ex. 7.4M for glibc.
  // see https://softwareengineering.stackexchange.com/a/310659/413818 and
  // https://stackoverflow.com/a/1826072/14227825 for some more details
// MSVC will complain about possible loss of data
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4244)
#endif  // _MSC_VER
  auto xs = std::make_shared<std::vector<double>>(n_samples);
  auto ys = std::make_shared<std::vector<double>>(n_samples);
#ifdef _MSC_VER
#pragma warning (pop)
#endif  // _MSC_VER
  for (auto it = xs->begin(); it != xs->end(); it++)
    *it = udist(rng);
  for (auto it = ys->begin(); it != ys->end(); it++)
    *it = udist(rng);
  // count number of points in the unit circle, i.e. 2-norm <= 1
  N_t n_inside = 0;
  for (N_t i = 0; i < n_samples; i++) {
    // no need for sqrt here since the target norm is 1
    if (xs->at(i) * xs->at(i) + ys->at(i) * ys->at(i) <= 1.)
      n_inside++;
  }
  return n_inside;
}

/**
 * Return number of samples in [-1, 1] x [-1, 1] that fall in the unit circle.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt18837_64`.
 *
 * @param n_samples `N_t` number of samples to use
 * @param seed `std::uint_fast64_t` seed for the 64-bit Mersenne Twister
 */
template <typename N_t>
inline N_t unit_circle_samples(N_t n_samples, std::uint_fast64_t seed)
{
  return unit_circle_samples(n_samples, std::mt19937_64{seed});
}

// template <typename T, typename V_t>
// T mcpi_gather(const V_t& circle_counts, const V_t& sample_counts)
// {
//   assert(circle_counts.size() == sample_counts.size());
//   // number of samples inside the unit circle, total number of samples drawn
//   std::uintmax_t n_inside = 0;
//   std::uintmax_t n_total = 0;
//   n_inside = std::accumulate(circle_counts.cbegin(), circle_counts.cend(), n_inside);
//   n_total = std::accumulate(sample_counts.cebgin(), sample_counts.cend(), n_total);
//   // do division first to reduce likelihood of overflow
//   return 4 * (static_cast<T>(n_inside) / n_total);
// }

// template <typename V_t>
// inline double mcpi_gather(const V_t& circle_counts, const V_t& sample_counts)
// {
//   return mcpi_gather<double>(circle_counts, sample_counts);
// }

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the standard "circle-filling" technique to estimate pi / 4.
 *
 * @tparam T return type
 * @tparam Rng *UniformRandomBitGenerator* type
 *
 * @param n_samples `N_t` number of samples to use
 * @param rng `const Rng&` PRNG instance
 */
template <typename T, typename N_t, typename Rng>
inline T mcpi(N_t n_samples, const Rng& rng)
{
  T x = 4 * static_cast<T>(unit_circle_samples(n_samples, rng)) / n_samples;
  std::cout << x << std::endl;
  return x;
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt18837_64`.
 *
 * @param n_samples `N_t` number of samples to use
 * @param seed `std::uint_fast64_t` seed for the 64-bit Mersenne Twister
 */
template <typename N_t>
inline double mcpi(N_t n_samples, std::uint_fast64_t seed)
{
  return mcpi<double>(n_samples, std::mt19937_64{seed});
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt18837_64`.
 *
 * @param n_samples `N_t` number of samples to use
 */
template <typename N_t>
inline double mcpi(N_t n_samples)
{
  return mcpi<double>(n_samples, std::random_device{}());
}

}  // namespace

#endif  // PDMPMT_CPP_MCPI_H_
