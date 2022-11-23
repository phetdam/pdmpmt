/**
 * @file mcpi.c
 * @author Derek Huang
 * @brief C implementation for estimating pi with Monte Carlo
 */

#include <pdmpmt/mcpi.h>

#include <assert.h>
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
  assert(n_samples && "n_samples must be positive");
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

/**
 * Return a new block of `unsigned long` values usable as GSL PRNG seeds.
 *
 * @param n_jobs `unsigned int` number of seeds to generate
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param seed `unsigned long` seed value for the PRNG
 */
gsl_block_ulong *
pdmpmt_rng_generate_seeds(
  unsigned int n_seeds, const gsl_rng_type *rng_type, unsigned long seed)
{
  assert(n_seeds && "n_seeds must be positive");
  // allocate new block and seeded PRNG
  gsl_block_ulong *seeds = gsl_block_ulong_alloc(n_seeds);
  gsl_rng *rng = gsl_rng_alloc(rng_type);
  gsl_rng_set(rng, seed);
  // fill block with PRNG values to use as seeds
  for (unsigned int i = 0; i < n_seeds; i++)
    seeds->data[i] = gsl_rng_get(rng);
  // clean up and return
  gsl_rng_free(rng);
  return seeds;
}

/**
 * Return a new block of `unsigned long` sample counts assigned to each job.
 *
 * @param n_samples `size_t` total number of samples
 * @param n_jobs `unsigned int` number of jobs to split samples over
 */
gsl_block_ulong *
pdmpmt_generate_sample_counts(size_t n_samples, unsigned int n_jobs)
{
  assert(n_samples && "n_samples must be positive");
  assert(n_jobs && "n_jobs must be positive");
  gsl_block_ulong *counts = gsl_block_ulong_alloc(n_jobs);
  // sample counts split as evenly as possible, i.e. remainder 0 < k < n_jobs
  // is evenly distributed over the first k jobs. we cast after computing
  // result to reduce loss of precision as much as possible
  unsigned long base_count = (unsigned long) (n_samples / n_jobs);
  unsigned int n_rem = (unsigned int) (n_samples % n_jobs);
  // could rewrite this as a two loops that make a single pass
  for (unsigned int i = 0; i < n_jobs; i++)
    counts->data[i] = base_count;
  for (unsigned int i = 0; i < n_rem; i++)
    counts->data[i]++;
  return counts;
}

/**
 * Gather `pdmpmt_*_unit_circle_samples` results to estimate pi.
 *
 * We sum all the counts of samples that fell in the unit circle, divide this
 * by the sum of the sample counts, and multiply by 4.
 *
 * @param circle_counts `gsl_block_ulong *` counts of samples in unit circle
 * @param sample_counts `gsl_block_ulong *` per-job total sample counts
 */
double
pdmpmt_mcpi_gather(
  gsl_block_ulong *circle_counts, gsl_block_ulong *sample_counts)
{
  assert(circle_counts->size && sample_counts->size);
  assert(circle_counts->size == sample_counts->size);
  size_t n_jobs = circle_counts->size;
  // number of samples inside the unit circle, total number of samples drawn
  size_t n_inside = 0;
  size_t n_total = 0;
  for (size_t i = 0; i < n_jobs; i++) {
    n_inside += circle_counts->data[i];
    n_total += sample_counts->data[i];
  }
  return 4 * ((double) n_inside / n_total);
}
