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

#include <gsl/gsl_block.h>
#include <gsl/gsl_rng.h>

#include "pdmpmt/common.h"
#include "pdmpmt/dllexport.h"

PDMPMT_EXTERN_C_BEGIN

/**
 * Draw and count number of samples in [-1, 1] x [-1, 1] are in unit circle.
 *
 * Uses the GSL PRNG library to generate the random values.
 *
 * @param n_samples `size_t` number of samples to draw
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param seed `unsigned long` seed value for the PRNG
 */
PDMPMT_PUBLIC
size_t
pdmpmt_rng_unit_circle_samples(
  size_t n_samples, const gsl_rng_type *rng_type, unsigned long seed);

/**
 * Draw and count number of samples in [-1, 1] x [-1, 1] are in unit circle.
 *
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples `size_t` number of samples to draw
 * @param seed `unsigned long` seed value for the PRNG
 *
 * @returns `size_t`
 */
PDMPMT_INLINE size_t
pdmpmt_mt32_unit_circle_samples(size_t n_samples, unsigned long seed)
{
  return pdmpmt_rng_unit_circle_samples(n_samples, gsl_rng_mt19937, seed);
}

/**
 * Return a new block of `unsigned long` values usable as GSL PRNG seeds.
 *
 * @param n_jobs `unsigned int` number of seeds to generate
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param seed `unsigned long` seed value for the PRNG
 */
PDMPMT_PUBLIC
gsl_block_ulong *
pdmpmt_rng_generate_seeds(
  unsigned int n_seeds, const gsl_rng_type *rng_type, unsigned long seed);

/**
 * Return a block of `unsigned long` values usable as GSL PRNG seeds.
 *
 * @param n_jobs `unsigned int` number of seeds to generate
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param seed `unsigned long` seed value for the PRNG
 *
 * @returns `gsl_block_ulong *`
 */
PDMPMT_INLINE gsl_block_ulong *
pdmpmt_mt32_generate_seeds(unsigned int n_seeds, unsigned long seed)
{
  return pdmpmt_rng_generate_seeds(n_seeds, gsl_rng_mt19937, seed);
}

/**
 * Return a new block of `unsigned long` sample counts assigned to each job.
 *
 * @param n_samples `size_t` total number of samples
 * @param n_jobs `unsigned int` number of jobs to split samples over
 */
PDMPMT_PUBLIC
gsl_block_ulong *
pdmpmt_generate_sample_counts(size_t n_samples, unsigned int n_jobs);

/**
 * Estimate pi by gathering `pdmpmt_*_unit_circle_samples` results.
 *
 * We sum all the counts of samples that fell in the unit circle, divide this
 * by the sum of the sample counts, and multiply by 4.
 *
 * @param circle_counts `gsl_block_ulong *` counts of samples in unit circle
 * @param sample_counts `gsl_block_ulong *` per-job total sample counts
 */
PDMPMT_PUBLIC
double
pdmpmt_mcpi_gather(
  gsl_block_ulong *circle_counts, gsl_block_ulong *sample_counts);

/**
 * Estimate pi using Monte Carlo.
 *
 * @param n_samples `size_t` number of samples to use
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param seed `unsigned long` seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_rng_smcpi(
  size_t n_samples, const gsl_rng_type *rng_type, unsigned long seed)
{
  double uctd = pdmpmt_rng_unit_circle_samples(n_samples, rng_type, seed);
  return 4 * uctd / n_samples;
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Seed is determined using the value of `time(NULL)` cast to `unsigned long`.
 *
 * @param n_samples `size_t` number of samples to use
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 *
 * @returns `double`
 */
PDMPMT_INLINE double
pdmpmt_rng_mcpi(size_t n_samples, const gsl_rng_type *rng_type)
{
  return pdmpmt_rng_smcpi(n_samples, rng_type, (unsigned long) time(NULL));
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples `size_t` number of samples to use
 * @param seed `unsigned long` seed value for the PRNG
 *
 * @returns `double`
 */
PDMPMT_INLINE double
pdmpmt_mt32_smcpi(size_t n_samples, unsigned long seed)
{
  return pdmpmt_rng_smcpi(n_samples, gsl_rng_mt19937, seed);
}

/**
 * Estimate pi using Monte Carlo.
 *
 * Seed is determined using the value of `time(NULL)` cast to `unsigned long`.
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples `size_t` number of samples to use
 *
 * @returns `double`
 */
PDMPMT_INLINE double
pdmpmt_mt32_mcpi(size_t n_samples)
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
 * @param n_samples `size_t` number of samples to draw
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param n_threads `unsigned int` number of OpenMP threads to split work over
 * @param seed `unsigned long` seed value for the PRNG
 */
PDMPMT_PUBLIC
double
pdmpmt_rng_smcpi_ompm(
  size_t n_samples,
  const gsl_rng_type *rng_type,
  unsigned int n_threads,
  unsigned long seed);

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If `n_threads`
 * is set to `PDMPMT_AUTO_OMP_JOBS`, i.e. 0, OpenMP sets the thread count. Seed
 * is determined using the value of `time(NULL)` cast to `unsigned long`.
 *
 * @param n_samples `size_t` number of samples to draw
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param n_threads `unsigned int` number of OpenMP threads to split work over
 */
PDMPMT_INLINE double
pdmpmt_rng_mcpi_ompm(
  size_t n_samples, const gsl_rng_type *rng_type, unsigned int n_threads)
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
 * @param n_samples `size_t` number of samples to draw
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param seed `unsigned long` seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_rng_smcpi_omp(
  size_t n_samples, const gsl_rng_type *rng_type, unsigned long seed)
{
  return pdmpmt_rng_smcpi_ompm(n_samples, rng_type, PDMPMT_AUTO_OMP_JOBS, seed);
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool and thread count.
 * Seed is determined using the value of `time(NULL)` cast to `unsigned long`.
 *
 * @param n_samples `size_t` number of samples to draw
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 */
PDMPMT_INLINE double
pdmpmt_rng_mcpi_omp(size_t n_samples, const gsl_rng_type *rng_type)
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
 * @param n_samples `size_t` number of samples to draw
 * @param n_threads `unsigned int` number of OpenMP threads to split work over
 * @param seed `unsigned long` seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_mt32_smcpi_ompm(
  size_t n_samples, unsigned int n_threads, unsigned long seed)
{
  return pdmpmt_rng_smcpi_ompm(n_samples, gsl_rng_mt19937, n_threads, seed);
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. Seed is
 * determined using the value of `time(NULL)` cast to `unsigned long`. Uses the
 * 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples `size_t` number of samples to draw
 * @param n_threads `unsigned int` number of OpenMP threads to split work over
 */
PDMPMT_INLINE double
pdmpmt_mt32_mcpi_ompm(size_t n_samples, unsigned int n_threads)
{
  return pdmpmt_mt32_smcpi_ompm(n_samples, n_threads, (unsigned long) time(NULL));
}

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool and thread count.
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples `size_t` number of samples to draw
 * @param seed `unsigned long` seed value for the PRNG
 */
PDMPMT_INLINE double
pdmpmt_mt32_smcpi_omp(size_t n_samples, unsigned long seed)
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
 * @param n_samples `size_t` number of samples to draw
 */
PDMPMT_INLINE double
pdmpmt_mt32_mcpi_omp(size_t n_samples)
{
  return pdmpmt_mt32_smcpi_omp(n_samples, (unsigned long) time(NULL));
}
#endif  // _OPENMP

PDMPMT_EXTERN_C_END

#endif  // PDMPMT_MCPI_H_
