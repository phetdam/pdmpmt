/**
 * @file mcpi.c
 * @author Derek Huang
 * @brief C implementation for estimating pi with Monte Carlo
 */

#include <pdmpmt/mcpi.h>

#include <stdint.h>

#include <gsl/gsl_block.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>

#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

/**
 * Draw and count number of samples in [-1, 1] x [-1, 1] are in unit circle.
 *
 * Uses the GSL PRNG library to generate the random values.
 *
 * @param n_samples `size_t` number of samples to draw
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param seed `unsigned long` seed value for the PRNG
 */
size_t
pdmpmt_rng_unit_circle_samples(
  size_t n_samples, const gsl_rng_type *rng_type, unsigned long seed)
{
  // initialize PRNG (note: on error, GSL functions invoke GSL error handler)
  gsl_rng *rng = gsl_rng_alloc(rng_type);
  gsl_rng_set(rng, seed);
  // count number of samples that fall in unit circle, i.e. 2-norm <= 1
  size_t n_inside = 0;
  double x, y;
  // raw loop avoids memory allocations
  for (size_t i = 0; i < n_samples; i++) {
    x = gsl_ran_flat(rng, -1, 1);
    y = gsl_ran_flat(rng, -1, 1);
    if (x * x + y * y <= 1) n_inside++;
  }
  // free and return
  gsl_rng_free(rng);
  return n_inside;
}
