/**
 * @file mcpi.hh
 * @author Derek Huang
 * @brief C++ template implementation for estimating pi using Monte Carlo
 * @copyright MIT License
 */

#ifndef PDMPMT_MCPI_HH_
#define PDMPMT_MCPI_HH_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <future>
#include <iterator>
#include <numeric>
#include <random>
#include <thread>
#include <type_traits>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

#ifdef __CUDACC__
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/random.h>
#include <thrust/random/uniform_real_distribution.h>
#endif  // __CUDACC__

#include "pdmpmt/warnings.h"

namespace pdmpmt {

namespace detail {

/**
 * Traits class for a callable that returns an unsigned value on invocation.
 *
 * This is a weaker requirement than the `is_uniform_random_bit_generator<T>`
 * traits and allows interop with Thrust PRNG classes (which do not satisfy
 * this traits concept) or use with generic callables.
 *
 * @tparam T type
 */
template <typename T, typename = void>
struct is_entropy_source : std::false_type {};

/**
 * True specialization for callables that can serve as an entropy source.
 *
 * @tparam T type
 */
template <typename T>
struct is_entropy_source<
  T,
  std::enable_if_t<
    std::is_invocable_v<T> &&
    std::is_unsigned_v<decltype(std::declval<T>()())>
  >
> : std::true_type {};

/**
 * SFINAE helper for callables that can serve as an entropy source.
 *
 * @tparam T type
 */
template <typename T>
using entropy_source_t = std::enable_if_t<is_entropy_source<T>::value>;

/**
 * Return number of samples in [-1, 1] x [-1, 1] that fall in the unit circle.
 *
 * We make a copy of the PRNG instance, otherwise its state will be changed.
 *
 * @tparam Rng *UniformRandomBitGenerator* or other entropy source
 *
 * @param n_samples Number of samples to use
 * @param rng PRNG instance
 */
template <typename Rng, typename = entropy_source_t<Rng>>
PDMPMT_XPU_FUNC
auto unit_circle_samples(std::size_t n_samples, Rng rng)
{
#if defined(__CUDACC__)
  thrust::random::uniform_real_distribution udist{-1., 1.};
#else
  std::uniform_real_distribution udist{-1., 1.};
#endif  // !defined(__CUDACC__)
  // count number of points in the unit circle, i.e. 2-norm <= 1
  double x, y;
  std::size_t n_inside = 0;
  // we can use a raw loop to avoid memory allocations
  for (std::size_t i = 0; i < n_samples; i++) {
    x = udist(rng);
    y = udist(rng);
    // no need for sqrt here since the target norm is 1
    if (x * x + y * y <= 1.)
      n_inside++;
  }
  return n_inside;
}

/**
 * Return number of samples in [-1, 1] x [-1, 1] that fall in the unit circle.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * If compiled as CUDA C++, Thrust's RANLUX48 is the entropy source.
 *
 * @param n_samples Number of samples to use
 * @param seed Seed for the 64-bit Mersenne Twister
 */
PDMPMT_XPU_FUNC
inline auto unit_circle_samples(std::size_t n_samples, std::uint_fast64_t seed)
{
#if defined(__CUDACC__)
  return unit_circle_samples(n_samples, thrust::random::ranlux48{seed});
#else
  return unit_circle_samples(n_samples, std::mt19937_64{seed});
#endif  // !defined(__CUDACC__)
}

/**
 * Return a vector of seed values for a specified PRNG instance's type.
 *
 * If compiled as standard C++, a `std::vector` is returned, while if compiled
 * as CUDA C++, a `thrust::device_vector` is returned instead.
 *
 * @todo When compiling as CUDA C++ this can only be used from host code.
 *
 * @tparam Rng *UniformRandomBitGenerator* or other entropy source
 *
 * @param n_seeds Number of PRNG seeds to generate
 * @param rng PRNG instance for whose type seeds will be generated
 */
template <typename Rng, typename = entropy_source_t<Rng>>
auto generate_seeds(std::size_t n_seeds, Rng rng)
{
#if defined(__CUDACC__)
  thrust::device_vector<typename Rng::result_type> seeds(n_seeds);
#else
  std::vector<typename Rng::result_type> seeds(n_seeds);
#endif  // !defined(__CUDACC__)
  std::for_each(seeds.begin(), seeds.end(), [&](auto& x) { x = rng(); });
  return seeds;
}

/**
 * Return a vector of seed values for the 64-bit Mersenne Twister.
 *
 * If compiled as standard C++, a `std::vector` is returned, while if compiled
 * as CUDA C++, a `thrust::device_vector` is returned instead.
 *
 * @todo When compiling as CUDA C++ this can only be used from host code.
 *
 * @param n_seeds Number of PRNG seeds to generate
 * @param start_seed Starting seed for the PRNG used to generate seeds
 */
inline auto generate_seeds(std::size_t n_seeds, std::uint_fast64_t start_seed)
{
#if defined(__CUDACC__)
  return generate_seeds(n_seeds, thrust::random::ranlux48{start_seed});
#else
  return generate_seeds(n_seeds, std::mt19937_64{start_seed});
#endif  // !defined(__CUDACC__)
}

/**
 * Given `n_jobs` jobs, divide `n_samples` sample to generate evenly.
 *
 * @param n_samples Total number of samples
 * @param n_jobs Number of jobs to split generation across
 */
inline auto generate_sample_counts(std::size_t n_samples, std::size_t n_jobs)
{
  // sample counts, i.e. number of samples each job will generate
  std::vector<std::size_t> sample_counts(n_jobs, n_samples / n_jobs);
  // if there is a remainder, +1 count for the first n_rem jobs
  auto n_rem = n_samples % n_jobs;
  std::for_each_n(sample_counts.begin(), n_rem, [](auto& n) { n++; });
  return sample_counts;
}

/**
 * Gather `unit_circle_samples` results with sample counts to estimate pi.
 *
 * @tparam T Return type
 * @tparam C1 *Container* with integral value type
 * @tparam C2 *Container* with integral value type
 *
 * @param circle_counts Counts of samples falling in unit circle
 * @param sample_counts Per-job total sample counts
 */
template <typename T, typename C1, typename C2>
T mcpi_gather(const C1& circle_counts, const C2& sample_counts)
{
  assert(std::size(circle_counts) && std::size(sample_counts));
  assert(std::size(circle_counts) == std::size(sample_counts));
  // number of samples inside the unit circle, total number of samples drawn
  auto n_inside = std::reduce(std::begin(circle_counts), std::end(circle_counts));
  auto n_total = std::reduce(std::begin(sample_counts), std::end(sample_counts));
  // do division first to reduce likelihood of overflow
// MSVC complains of possible loss of data converting size_t to double
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(5219)
  return 4 * (static_cast<T>(n_inside) / n_total);
PDMPMT_MSVC_WARNING_POP()
}

/**
 * Gather `unit_circle_samples` results with sample counts to estimate pi.
 *
 * @tparam C1 *Container* with integral value type
 * @tparam C2 *Container* with integral value type
 *
 * @param circle_counts Counts of samples falling in unit circle
 * @param sample_counts Per-job total sample counts
 */
template <typename C1, typename C2>
inline auto mcpi_gather(const C1& circle_counts, const C2& sample_counts)
{
  return mcpi_gather<double>(circle_counts, sample_counts);
}

}  // namespace detail

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the standard "circle-filling" technique to estimate pi / 4.
 *
 * @todo Figure out how to enable Thrust PRNG usage in constraint.
 *
 * @tparam T Return type
 * @tparam Rng *UniformRandomBitGenerator* or other entropy source
 *
 * @param n_samples Number of samples to use
 * @param rng PRNG instance
 */
template <typename T, typename Rng, typename = detail::entropy_source_t<Rng>>
inline T mcpi(std::size_t n_samples, const Rng& rng)
{
  // MSVC complains about size_t to double loss of data
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4244 5219)
  auto uctd = static_cast<T>(detail::unit_circle_samples(n_samples, rng));
  return 4 * (uctd / n_samples);
PDMPMT_MSVC_WARNING_POP()
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * @todo Need a better way to integrate Thrust code path when using NVCC.
 *
 * @param n_samples Number of samples to use
 * @param seed Seed for the 64-bit Mersenne Twister
 */
inline double mcpi(std::size_t n_samples, std::uint_fast64_t seed)
{
#if defined(__CUDACC__)
  return mcpi<double>(n_samples, thrust::random::ranlux48{seed});
#else
  return mcpi<double>(n_samples, std::mt19937_64{seed});
#endif  // !defined(__CUDACC__)
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * @param n_samples Number of samples to use
 */
inline double mcpi(std::size_t n_samples)
{
  return mcpi(n_samples, std::random_device{}());
}

/**
 * Parallel estimation of pi through Monte Carlo by launching async jobs.
 *
 * Simple map-reduce using `std::async` provided in `<future>`.
 *
 * @tparam T Return type
 * @tparam Rng *UniformRandomBitGenerator* or other entropy source
 *
 * @param n_samples Number of samples to use
 * @param rng PRNG instance
 * @param n_jobs Number of async jobs to split work over
 */
template <typename T, typename Rng>
T mcpi_async(std::size_t n_samples, const Rng& rng, std::size_t n_jobs)
{
  using N_t = decltype(n_samples);
  // generate the seeds used by jobs for generate samples + the sample counts
  auto seeds = detail::generate_seeds(n_jobs, rng);
  auto sample_counts = detail::generate_sample_counts(n_samples, n_jobs);
  // submit unit_circle_samples tasks asynchronously + block for results
  std::vector<std::future<N_t>> circle_count_futures(n_jobs);
  for (N_t i = 0; i < n_jobs; i++) {
    circle_count_futures[i] = std::async(
      std::launch::async,
      detail::unit_circle_samples<Rng>,
      sample_counts[i],
      Rng{seeds[i]}
    );
  }
  // get circle counts from futures
  decltype(sample_counts) circle_counts(n_jobs);
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
 * @tparam N_t Integral type
 *
 * @param n_samples Number of samples to use
 * @param seed Seed for the 64-bit Mersenne Twister
 * @param n_jobs Number of async jobs to split work over
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
 * the number of jobs given by `std::thread::hardware_concurrency()`, unless
 * the returned value is 0, in which case only 1 job will be used.
 *
 * @tparam N_t Integral type
 *
 * @param n_samples Number of samples to use
 * @param seed Seed for the 64-bit Mersenne Twister
 */
template <typename N_t>
inline double mcpi_async(
  N_t n_samples, std::uint_fast64_t seed = std::random_device{}())
{
  auto n_threads = std::thread::hardware_concurrency();
  // can be zero if not well defined or not computable
  if (!n_threads)
    n_threads = 1;
  // if not explicitly specifying the template parameter, n_threads must be
  // static_cast to N_t in order for template deduction to work correctly
  return mcpi_async<N_t>(n_samples, seed, n_threads);
}

#ifdef _OPENMP
/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If the number of
 * threads is not given, i.e. left as 0, then OpenMP chooses number of threads.
 *
 * @tparam T Return type
 * @tparam N_t Integral type
 * @tparam Rng *UniformRandomBitGenerator* or other entropy source
 *
 * @param n_samples Number of samples to use
 * @param rng PRNG instance
 * @param n_threads Number of OpenMP threads to split work over
 */
template <typename T, typename N_t, typename Rng>
T mcpi_omp(N_t n_samples, const Rng& rng, unsigned n_threads = 0u)
{
  // MSVC complains about signed/unsigned mismatch
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4365)
  // set number of threads if nonzero else use default
  if (n_threads)
    omp_set_num_threads(n_threads);
  else
    n_threads = omp_get_num_threads();
PDMPMT_MSVC_WARNING_POP()
  // generate seeds used by jobs for generating samples + the sample counts
  auto seeds = detail::generate_seeds(n_threads, rng);
  // to use template deduction, would have to static_cast n_threads to N_t
  auto sample_counts = detail::generate_sample_counts(n_samples, n_threads);
  // compute circle counts using multiple threads using OpenMP
  std::vector<N_t> circle_counts(n_threads);
  #pragma omp parallel for
// for MSVC, since its OpenMP version is quite old (2.0), must use signed var
  for (
#ifdef _MSC_VER
    std::intmax_t i = 0;
    i < static_cast<decltype(i)>(n_threads);
#else
    N_t i = 0;
    i < n_threads;
#endif  // _MSC_VER
    i++
  ) {
// MSVC complains of signed/unsigned mismatch as i is intmax_t
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4365)
    circle_counts[i] = detail::
      unit_circle_samples(sample_counts[i], Rng{seeds[i]});
PDMPMT_MSVC_WARNING_POP()
  }
  return detail::mcpi_gather(circle_counts, sample_counts);
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If the number of
 * threads is not given, i.e. left as 0, then OpenMP chooses number of threads.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * @tparam N_t Integral type
 *
 * @param n_samples Number of samples to use
 * @param seed Seed for the 64-bit Mersenne Twister
 * @param n_threads Number of OpenMP threads to split work over
 */
template <typename N_t>
inline auto mcpi_omp(
  N_t n_samples, std::uint_fast64_t seed, unsigned n_threads = 0u)
{
  return mcpi_omp<double>(n_samples, std::mt19937_64{seed}, n_threads);
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If the number of
 * threads is not given, i.e. left as 0, then OpenMP chooses number of threads.
 *
 * Uses the 64-bit Mersenne Twister implemented through `std::mt19937_64`.
 *
 * @tparam N_t Integral type
 *
 * @param n_samples Number of samples to use
 * @param n_threads Number of OpenMP threads to split work over
 */
template <typename N_t>
inline auto mcpi_omp(N_t n_samples, unsigned n_threads = 0u)
{
  return mcpi_omp(n_samples, std::random_device{}(), n_threads);
}
#endif  // _OPENMP

}  // namespace

#endif  // PDMPMT_MCPI_HH_
