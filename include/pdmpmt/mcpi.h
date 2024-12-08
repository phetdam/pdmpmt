/**
 * @file mcpi.h
 * @author Derek Huang
 * @brief C header for estimating pi with Monte Carlo
 * @copyright MIT License
 */

#ifndef PDMPMT_MCPI_H_
#define PDMPMT_MCPI_H_

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "pdmpmt/block.h"
#include "pdmpmt/common.h"
#include "pdmpmt/dllexport.h"
#include "pdmpmt/warnings.h"

PDMPMT_EXTERN_C_BEGIN

/**
 * Enum indicating the supported PRNG schemes.
 *
 * @note Do not rely on the enumerator members having specific integral values.
 *  These may change if the backing implementation changes.
 */
typedef enum {
  PDMPMT_RNG_MRG32K3A = 0,  // MRG32k3a
  PDMPMT_RNG_MT19937 = 1,   // 32-bit Mersenne Twister
  PDMPMT_RNG_COUNT          // available methods
} pdmpmt_rng_type;

/**
 * Draw and count samples in [-1, 1] x [-1, 1] that fall in the unit circle.
 *
 * @param n_samples Number of samples to draw
 * @param rng_type PRNG type
 * @param seed Seed value for the PRNG
 */
PDMPMT_PUBLIC
size_t
pdmpmt_rng_unit_circle_samples(
  size_t n_samples,
  pdmpmt_rng_type rng_type,
  unsigned seed) PDMPMT_NOEXCEPT;

/**
 * Draw and count number of samples in [-1, 1] x [-1, 1] are in unit circle.
 *
 * Uses the 32-bit Mersenne Twister as the PRNG scheme.
 *
 * @param n_samples Number of samples to draw
 * @param seed Seed value for the PRNG
 */
PDMPMT_INLINE size_t
pdmpmt_mt32_unit_circle_samples(size_t n_samples, unsigned seed) PDMPMT_NOEXCEPT
{
  return pdmpmt_rng_unit_circle_samples(n_samples, PDMPMT_RNG_MT19937, seed);
}

/**
 * Return a new block of unsigned long values usable as PRNG seeds.
 *
 * @param n_jobs Number of seeds to generate
 * @param rng_type Address to a GSL PRNG
 * @param seed Seed value for the PRNG
 */
PDMPMT_PUBLIC
pdmpmt_block_ulong
pdmpmt_rng_generate_seeds(
  unsigned n_seeds,
  pdmpmt_rng_type rng_type,
  unsigned seed) PDMPMT_NOEXCEPT;

/**
 * Return a block of `unsigned long` values usable as GSL PRNG seeds.
 *
 * @param n_jobs Number of seeds to generate
 * @param rng_type Address to a GSL PRNG
 * @param seed Seed value for the PRNG
 */
PDMPMT_INLINE
pdmpmt_block_ulong
pdmpmt_mt32_generate_seeds(unsigned int n_seeds, unsigned seed) PDMPMT_NOEXCEPT
{
  return pdmpmt_rng_generate_seeds(n_seeds, PDMPMT_RNG_MT19937, seed);
}

/**
 * Return a new block of `unsigned long` sample counts assigned to each job.
 *
 * @param n_samples Total number of samples
 * @param n_jobs Number of jobs to split samples over
 */
PDMPMT_PUBLIC
pdmpmt_block_ulong
pdmpmt_generate_sample_counts(
  size_t n_samples,
  unsigned int n_jobs) PDMPMT_NOEXCEPT;

/**
 * Estimate pi by gathering `pdmpmt_*_unit_circle_samples` results.
 *
 * We sum all the counts of samples that fell in the unit circle, divide this
 * by the sum of the sample counts, and multiply by 4.
 *
 * @param circle_counts Block with counts of samples in unit circle
 * @param sample_counts Block with per-job total sample counts
 */
PDMPMT_PUBLIC
double
pdmpmt_mcpi_gather(
  pdmpmt_block_ulong circle_counts,
  pdmpmt_block_ulong sample_counts) PDMPMT_NOEXCEPT;

/**
 * Estimate pi using Monte Carlo.
 *
 * @param n_samples Number of samples to use
 * @param rng_type PRNG type
 * @param seed Seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_rng_smcpi(
  size_t n_samples,
  pdmpmt_rng_type rng_type,
  unsigned long seed) PDMPMT_NOEXCEPT
{
// MSVC complains that size_t to double may lose data
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4244 5219)
  double uctd = pdmpmt_rng_unit_circle_samples(n_samples, rng_type, seed);
  return 4 * uctd / n_samples;
PDMPMT_MSVC_WARNING_POP()
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Seed is determined using the value of `time(NULL)` cast to `unsigned long`.
 *
 * @param n_samples Number of samples to use
 * @param rng_type PRNG type
 */
PDMPMT_INLINE double
pdmpmt_rng_mcpi(size_t n_samples, pdmpmt_rng_type rng_type) PDMPMT_NOEXCEPT
{
  return pdmpmt_rng_smcpi(n_samples, rng_type, (unsigned long) time(NULL));
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the 32-bit Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples Number of samples to use
 * @param seed Seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_mt32_smcpi(size_t n_samples, unsigned long seed) PDMPMT_NOEXCEPT
{
  return pdmpmt_rng_smcpi(n_samples, PDMPMT_RNG_MT19937, seed);
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Seed is determined using the value of `time(NULL)` cast to `unsigned long`.
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples Number of samples to use
 */
PDMPMT_INLINE double
pdmpmt_mt32_mcpi(size_t n_samples) PDMPMT_NOEXCEPT
{
  return pdmpmt_mt32_smcpi(n_samples, (unsigned long) time(NULL));
}

#ifdef _OPENMP
// macro to indicate that number of jobs should equal number of OpenMP threads
#define PDMPMT_AUTO_OMP_JOBS 0

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If `n_threads`
 * is set to `PDMPMT_AUTO_OMP_JOBS`, i.e. 0, OpenMP sets the thread count.
 *
 * @param n_samples Number of samples to draw
 * @param rng_type PRNG type
 * @param n_threads Number of OpenMP threads to split work over
 * @param seed Seed value for the PRNG
 */
PDMPMT_PUBLIC
double
pdmpmt_rng_smcpi_ompm(
  size_t n_samples,
  pdmpmt_rng_type rng_type,
  unsigned int n_threads,
  unsigned long seed) PDMPMT_NOEXCEPT;

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If `n_threads`
 * is set to `PDMPMT_AUTO_OMP_JOBS`, i.e. 0, OpenMP sets the thread count. Seed
 * is determined using the value of `time(NULL)` cast to `unsigned long`.
 *
 * @param n_samples Number of samples to draw
 * @param rng_type PRNG type
 * @param n_threads Number of OpenMP threads to split work over
 */
PDMPMT_INLINE double
pdmpmt_rng_mcpi_ompm(
  size_t n_samples,
  pdmpmt_rng_type rng_type,
  unsigned int n_threads) PDMPMT_NOEXCEPT
{
  return pdmpmt_rng_smcpi_ompm(
    n_samples, rng_type, n_threads, (unsigned long) time(NULL)
  );
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool and thread count.
 *
 * @param n_samples Number of samples to draw
 * @param rng_type PRNG type
 * @param seed Seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_rng_smcpi_omp(
  size_t n_samples,
  pdmpmt_rng_type rng_type,
  unsigned long seed) PDMPMT_NOEXCEPT
{
  return pdmpmt_rng_smcpi_ompm(n_samples, rng_type, PDMPMT_AUTO_OMP_JOBS, seed);
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool and thread count.
 * Seed is determined using the value of `time(NULL)` cast to `unsigned long`.
 *
 * @param n_samples Number of samples to draw
 * @param rng_type PRNG type
 */
PDMPMT_INLINE double
pdmpmt_rng_mcpi_omp(size_t n_samples, pdmpmt_rng_type rng_type) PDMPMT_NOEXCEPT
{
  return pdmpmt_rng_smcpi_omp(n_samples, rng_type, (unsigned long) time(NULL));
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If `n_threads`
 * is set to `PDMPMT_AUTO_OMP_JOBS`, i.e. 0, OpenMP sets the thread count. Uses
 * the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples Number of samples to draw
 * @param n_threads Number of OpenMP threads to split work over
 * @param seed Seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_mt32_smcpi_ompm(
  size_t n_samples, unsigned int n_threads, unsigned long seed) PDMPMT_NOEXCEPT
{
  return pdmpmt_rng_smcpi_ompm(n_samples, PDMPMT_RNG_MT19937, n_threads, seed);
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. Seed is
 * determined using the value of `time(NULL)` cast to `unsigned long`. Uses the
 * 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples Number of samples to draw
 * @param n_threads Number of OpenMP threads to split work over
 */
PDMPMT_INLINE double
pdmpmt_mt32_mcpi_ompm(size_t n_samples, unsigned int n_threads) PDMPMT_NOEXCEPT
{
  return pdmpmt_mt32_smcpi_ompm(n_samples, n_threads, (unsigned long) time(NULL));
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool and thread count.
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples Number of samples to draw
 * @param seed Seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_mt32_smcpi_omp(size_t n_samples, unsigned long seed) PDMPMT_NOEXCEPT
{
  return pdmpmt_mt32_smcpi_ompm(n_samples, PDMPMT_AUTO_OMP_JOBS, seed);
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool and thread count.
 * Seed is determined using the value of `time(NULL)` cast to `unsigned long`.
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples Number of samples to draw
 */
PDMPMT_INLINE double
pdmpmt_mt32_mcpi_omp(size_t n_samples) PDMPMT_NOEXCEPT
{
  return pdmpmt_mt32_smcpi_omp(n_samples, (unsigned long) time(NULL));
}
#endif  // _OPENMP

PDMPMT_EXTERN_C_END

#endif  // PDMPMT_MCPI_H_
