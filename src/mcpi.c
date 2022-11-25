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
 * Estimate pi by gathering `pdmpmt_*_unit_circle_samples` results.
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

#ifdef _OPENMP

/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If `n_threads`
 * is set to `PDMPMT_AUTO_OMP_THREADS`, i.e. 0, OpenMP sets the thread count.
 * If `n_jobs` is set to `PDMPMT_AUTO_OMP_JOBS`, i.e. 0, `n_jobs` is set equal
 * to the number of threads that will be used by OpenMP.
 *
 * @param n_samples `size_t` number of samples to draw
 * @param rng_type `const gsl_rng_type *` GSL PRNG type pointer
 * @param n_jobs `unsigned int` number of jobs to split work over
 * @param n_threads `unsigned int` number of threads to use
 * @param seed `unsigned long` seed value for the PRNG
 */
double
pdmpmt_rng_smcpi_ompm(
  size_t n_samples,
  const gsl_rng_type *rng_type,
  unsigned int n_jobs,
  unsigned int n_threads,
  unsigned long seed)
{
  if (n_threads) omp_set_num_threads(n_threads);
  if (!n_jobs) n_jobs = n_threads;
  // generate seeds used by jobs for generating samples + the sample counts
  gsl_block_ulong *seeds, *sample_counts;
  seeds = pdmpmt_rng_generate_seeds(n_jobs, rng_type, seed);
  sample_counts = pdmpmt_generate_sample_counts(n_samples, n_jobs);
  // compute circle counts with OpenMP
  gsl_block_ulong *circle_counts = gsl_block_ulong_alloc(n_jobs);
// for MSVC, since its OpenMP version is quite old (2.0), must use signed var.
// furthermore, unlike when compiling C++ code, cannot use C99-style loop, even
// with C11 language specification (/std:c11) passed to compiler
#ifdef _MSC_VER
  int i;
  #pragma omp parallel for
  for (i = 0; i < n_jobs; i++) {
#else
  #pragma omp parallel for
  for (unsigned int i = 0; i < n_jobs; i++) {
#endif  // _MSC_VER
    circle_counts->data[i] = pdmpmt_rng_unit_circle_samples(
      sample_counts->data[i], rng_type, seeds->data[i]
    );
  }
  // get pi estimate, clean up, and return
  double pi_hat = pdmpmt_mcpi_gather(circle_counts, sample_counts);
  gsl_block_ulong_free(seeds);
  gsl_block_ulong_free(circle_counts);
  gsl_block_ulong_free(sample_counts);
  return pi_hat;
}

#endif  // _OPENMP
