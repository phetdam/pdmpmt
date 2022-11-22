/**
 * @file mcpi.h
 * @author Derek Huang
 * @brief C header for estimating pi with Monte Carlo
 */

#ifndef PDMPMT_MCPI_H_
#define PDMPMT_MCPI_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <gsl/gsl_rng.h>

#define PDMPMT_JOB_ELEMENTS_DEFAULT 50000000

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
 */
#define pdmpmt_mt32_unit_circle_samples(n_samples, seed) \
  pdmpmt_rng_unit_circle_samples(n_samples, gsl_rng_mt19937, seed)

/**
 * Estimate pi using Monte Carlo.
 *
 * @param n_samples `size_t` number of samples to use
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param seed `unsigned long` seed value for the PRNG
 */
static inline double
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
 */
#define pdmpmt_rng_mcpi(n_samples, rng_type) \
  pdmpmt_rng_smcpi(n_samples, rng_type, (unsigned long) time(NULL))

/**
 * Estimate pi using Monte Carlo.
 *
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples `size_t` number of samples to use
 * @param seed `unsigned long` seed value for the PRNG
 */
#define pdmpmt_mt32_smcpi(n_samples, seed) \
  pdmpmt_rng_smcpi(n_samples, gsl_rng_mt19937, seed)

/**
 * Estimate pi using Monte Carlo.
 *
 * Seed is determined using the value of `time(NULL)` cast to `unsigned long`.
 * Uses the 32-bit GSL Mersenne Twister implementation as its PRNG.
 *
 * @param n_samples `size_t` number of samples to use
 */
#define pdmpmt_mt32_mcpi(n_samples) \
  pdmpmt_mt32_smcpi(n_samples, (unsigned long) time(NULL))

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // PDMPMT_MCPI_H_
