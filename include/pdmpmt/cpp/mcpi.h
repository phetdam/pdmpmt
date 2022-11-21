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
#include <future>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <type_traits>
#include <vector>

namespace pdmpmt {

namespace detail {

// default number of elements each job is responsible for at maximum
inline const std::size_t job_elements_default = 50000000;

/**
 * Return number of samples in [-1, 1] x [-1, 1] that fall in the unit circle.
 *
 * We make a copy of the PRNG instance, otherwise its state will be changed.
 *
 * @tparam N_t integer type
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
  std::for_each(xs->begin(), xs->end(), [&](auto& x) { x = udist(rng); });
  std::for_each(ys->begin(), ys->end(), [&](auto& x) { x = udist(rng); });
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
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * @tparam N_t integer type
 *
 * @param n_samples `N_t` number of samples to use
 * @param seed `std::uint_fast64_t` seed for the 64-bit Mersenne Twister
 */
template <typename N_t>
inline N_t unit_circle_samples(N_t n_samples, std::uint_fast64_t seed)
{
  return unit_circle_samples(n_samples, std::mt19937_64{seed});
}

/**
 * Return a `std::vector` of seed values for a specified PRNG instance's type.
 *
 * @tparam N_t integer type
 * @tparam Rng *UniformRandomBitGenerator* type
 *
 * @param n_seeds `N_t` number of PRNG seeds to generate
 * @param rng `Rng` PRNG instance for whose type seeds will be generated
 */
template <typename N_t, typename Rng>
std::vector<typename Rng::result_type> generate_seeds(N_t n_seeds, Rng rng)
{
  std::vector<typename Rng::result_type> seeds(n_seeds);
  std::for_each(seeds.begin(), seeds.end(), [&](auto& x) { x = rng(); });
  return seeds;
}

/**
 * Return a `std::vector` of seed values for the 64-bit Mersenne Twister.
 *
 * @tparam N_t integer type
 *
 * @param n_seeds `N_t` number of PRNG seeds to generate
 * @param initial_seed `std::uint_fast64_t` starting seed for the
 *    `std::mt19937_64` used to generate the seeds
 */
template <typename N_t>
inline std::vector<std::uint_fast64_t> generate_seeds(
  N_t n_seeds, std::uint_fast64_t initial_seed)
{
  return generate_seeds(n_seeds, std::mt19937_64{initial_seed});
}

/**
 * Given `n_jobs` jobs, divide `n_samples` sample to generate evenly.
 *
 * @tparam N_t integer type
 *
 * @param n_samples `N_t` total number of samples
 * @param n_jobs `N_t` number of jobs to split sample generation across
 */
template <typename N_t>
inline auto generate_sample_counts(N_t n_samples, N_t n_jobs)
{
  // sample counts, i.e. number of samples each job will generate
  std::vector<N_t> sample_counts(n_jobs, n_samples / n_jobs);
  // if there is a remainder, +1 count for the first n_rem jobs
  N_t n_rem = n_samples % n_jobs;
  std::for_each_n(sample_counts.begin(), n_rem, [](auto& n) { n++; });
  return sample_counts;
}

/**
 * Gather `unit_circle_samples` results with sample counts to estimate pi.
 *
 * @tparam T return type
 * @tparam V_t *Container*
 *
 * @param circle_counts `const V_t&` counts of samples falling in unit circle
 * @param sample_counts `const V_t&` per-job total sample counts
 */
template <typename T, typename V_t>
T mcpi_gather(const V_t& circle_counts, const V_t& sample_counts)
{
  assert(circle_counts.size() == sample_counts.size());
  using N_t = typename V_t::value_type;
  // number of samples inside the unit circle, total number of samples drawn
  N_t n_inside = 0;
  N_t n_total = 0;
  n_inside = std::accumulate(circle_counts.cbegin(), circle_counts.cend(), n_inside);
  n_total = std::accumulate(sample_counts.cbegin(), sample_counts.cend(), n_total);
  // do division first to reduce likelihood of overflow
  return 4 * (static_cast<T>(n_inside) / n_total);
}

/**
 * Gather `unit_circle_samples` results with sample counts to estimate pi.
 *
 * @tparam V_t *Container*
 *
 * @param circle_counts `const V_t&` counts of samples falling in unit circle
 * @param sample_counts `const V_t&` per-job total sample counts
 */
template <typename V_t>
inline double mcpi_gather(const V_t& circle_counts, const V_t& sample_counts)
{
  return mcpi_gather<double>(circle_counts, sample_counts);
}

}  // namespace detail

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the standard "circle-filling" technique to estimate pi / 4.
 *
 * @tparam T return type
 * @tparam N_t integer type
 * @tparam Rng *UniformRandomBitGenerator* type
 *
 * @param n_samples `N_t` number of samples to use
 * @param rng `const Rng&` PRNG instance
 */
template <typename T, typename N_t, typename Rng>
inline T mcpi(N_t n_samples, const Rng& rng)
{
  const auto uct{static_cast<T>(detail::unit_circle_samples(n_samples, rng))};
  return 4 * (uct / n_samples);
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * @tparam N_t integer type
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
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * @tparam N_t integer type
 *
 * @param n_samples `N_t` number of samples to use
 */
template <typename N_t>
inline double mcpi(N_t n_samples)
{
  return mcpi<double>(n_samples, std::random_device{}());
}

/**
 * Parallel estimation of pi through Monte Carlo by launching async jobs.
 *
 * Simple map-reduce using `std::async` provided in `<future>`.
 *
 * @tparam T return type
 * @tparam N_t integer type
 * @tparam Rng *UniformRandomBitGenerator* type
 *
 * @param n_samples `N_t` number of samples to use
 * @param rng `const Rng&` PRNG instance
 * @param n_jobs `N_t` number of async jobs to split work over
 */
template <typename T, typename N_t, typename Rng>
T mcpi_async(N_t n_samples, const Rng& rng, N_t n_jobs)
{
  // generate the seeds used by jobs for generate samples + the sample counts
  const auto seeds{detail::generate_seeds(n_jobs, rng)};
  const auto sample_counts{detail::generate_sample_counts(n_samples, n_jobs)};
  // submit unit_circle_samples tasks asynchronously + block for results
  std::vector<std::future<N_t>> circle_count_futures(n_jobs);
  for (N_t i = 0; i < n_jobs; i++) {
    circle_count_futures[i] = std::async(
      std::launch::async,
      detail::unit_circle_samples<N_t, Rng>,
      sample_counts[i],
      Rng{seeds[i]}
    );
  }
  // need to remove the const from const auto
  std::remove_const_t<decltype(sample_counts)> circle_counts(n_jobs);
  std::transform(
    circle_count_futures.begin(),
    circle_count_futures.end(),
    circle_counts.begin(),
    [](auto& x) { return x.get(); }
  );
  return detail::mcpi_gather(circle_counts, sample_counts);
}

/**
 * Parallel estimation of pi through Monte Carlo by launching async jobs.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * @tparam N_t integer type
 *
 * @param n_samples `N_t` number of samples to use
 * @param seed `std::uint_fast64_t` seed for the 64-bit Mersenne Twister
 * @param n_jobs `N_t` number of async jobs to split work over
 */
template <typename N_t>
inline double mcpi_async(N_t n_samples, std::uint_fast64_t seed, N_t n_jobs)
{
  return mcpi_async<double>(n_samples, std::mt19937_64{seed}, n_jobs);
}

/**
 * Parallel estimation of pi through Monte Carlo by launching async jobs.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`, with
 * the default number of jobs chosen to be such that each jobs will use at most
 * 400 MB when generating samples from [-1, 1] x [-1, 1].
 *
 * @tparam N_t integer type
 *
 * @param n_samples `N_t` number of samples to use
 * @param seed `std::uint_fast64_t` seed for the 64-bit Mersenne Twister
 */
template <typename N_t>
inline double mcpi_async(
  N_t n_samples, std::uint_fast64_t seed = std::random_device{}())
{
  N_t n_jobs = n_samples / detail::job_elements_default;
  if (n_samples % detail::job_elements_default) n_jobs++;
  return mcpi_async(n_samples, seed, n_jobs);
}

}  // namespace

#endif  // PDMPMT_CPP_MCPI_H_
