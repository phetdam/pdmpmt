/**
 * @file pdmpmt/mcpi.c
 * @author Derek Huang
 * @brief C implementation for estimating pi with Monte Carlo
 * @copyright MIT License
 */

#include "pdmpmt/mcpi.h"

#include <assert.h>
#include <stdint.h>

#include <prand.h>

#include "pdmpmt/block.h"
#include "pdmpmt/warnings.h"

#ifdef _OPENMP
#include <omp.h>
#endif  // _OPENMP

/**
 * Helper function to create a new prand structure.
 *
 * @param type PRNG type
 * @param seed PRNG seed value
 */
static prand_t *
make_prand(prand_rng_enum type, uint64_t seed)
{
  int rng_err = 0;
  // note: step is hardcoded since we only have one stream
  prand_t *rng = prand_init(type, seed, 1u, (1 << 14), &rng_err);
  // note: should we more comprehensively check for errors?
  assert(!PRAND_IS_ERROR(rng_err) && "RNG creation must not error");
  return rng;
}

/**
 * Draw and count samples in [-1, 1] x [-1, 1] that fall in the unit circle.
 *
 * @param n_samples Number of samples to draw
 * @param rng_type PRNG type
 * @param seed Seed value for the PRNG
 */
size_t
pdmpmt_rng_unit_circle_samples(
  size_t n_samples,
  pdmpmt_rng_type rng_type,
  unsigned seed)
{
  assert(n_samples && "n_samples must be positive");
  // initialize PRNG (note: on error, GSL functions invoke GSL error handler)
#if 0
  gsl_rng *rng = gsl_rng_alloc(rng_type);
  gsl_rng_set(rng, seed);
#endif  // 0
  prand_t *rng = make_prand(rng_type, seed);
  // count number of samples that fall in unit circle, i.e. 2-norm <= 1
  size_t n_inside = 0;
  double x, y;
  // raw loop avoids memory allocations
  for (size_t i = 0; i < n_samples; i++) {
#if 0
    x = gsl_ran_flat(rng, -1, 1);
    y = gsl_ran_flat(rng, -1, 1);
#endif  // 0
    x = 2 * rng->get_double_pos(rng->state) - 1;
    y = 2 * rng->get_double_pos(rng->state) - 1;
    if (x * x + y * y <= 1)
      n_inside++;
  }
  // free and return
#if 0
  gsl_rng_free(rng);
#endif  // 0
  prand_destroy(rng);
  return n_inside;
}

/**
 * Return a new block of `unsigned long` values usable as GSL PRNG seeds.
 *
 * @param n_jobs Number of seeds to generate
 * @param rng_type Address to a GSL PRNG
 * @param seed Seed value for the PRNG
 */
pdmpmt_block_ulong
pdmpmt_rng_generate_seeds(
  unsigned n_seeds,
  pdmpmt_rng_type rng_type,
  unsigned seed)
{
  assert(n_seeds && "n_seeds must be positive");
  // allocate new block and seeded PRNG
  pdmpmt_block_ulong seeds = pdmpmt_block_ulong_alloc(n_seeds);
  assert(seeds.data && "block memory must be allocated");
#if 0
  gsl_rng *rng = gsl_rng_alloc(rng_type);
  gsl_rng_set(rng, seed);
#endif  // 0
  prand_t *rng = make_prand(rng_type, seed);
  // fill block with PRNG values to use as seeds
  // FIXME: have a more mathematically appropriate way to handle the reduction
  // in type width from uint64_t to unsigned long for 32-bit systems
  for (unsigned int i = 0; i < n_seeds; i++)
    // seeds->data[i] = gsl_rng_get(rng);
    seeds.data[i] = rng->get(rng->state);
  // clean up and return
#if 0
  gsl_rng_free(rng);
#endif  // 0
  prand_destroy(rng);
  return seeds;
}

/**
 * Return a new block of `unsigned long` sample counts assigned to each job.
 *
 * @param n_samples Total number of samples
 * @param n_jobs Number of jobs to split samples over
 */
pdmpmt_block_ulong
pdmpmt_generate_sample_counts(size_t n_samples, unsigned int n_jobs)
{
  assert(n_samples && "n_samples must be positive");
  assert(n_jobs && "n_jobs must be positive");
  pdmpmt_block_ulong counts = pdmpmt_block_ulong_alloc(n_jobs);
  // sample counts split as evenly as possible, i.e. remainder 0 < k < n_jobs
  // is evenly distributed over the first k jobs. we cast after computing
  // result to reduce loss of precision as much as possible
  unsigned long base_count = (unsigned long) (n_samples / n_jobs);
  unsigned int n_rem = (unsigned int) (n_samples % n_jobs);
  // could rewrite this as a two loops that make a single pass
  for (unsigned int i = 0; i < n_jobs; i++)
    counts.data[i] = base_count;
  for (unsigned int i = 0; i < n_rem; i++)
    counts.data[i]++;
  return counts;
}

/**
 * Estimate pi by gathering `pdmpmt_*_unit_circle_samples` results.
 *
 * We sum all the counts of samples that fell in the unit circle, divide this
 * by the sum of the sample counts, and multiply by 4.
 *
 * @param circle_counts Block with counts of samples in unit circle
 * @param sample_counts Block with per-job total sample counts
 */
double
pdmpmt_mcpi_gather(
  pdmpmt_block_ulong circle_counts,
  pdmpmt_block_ulong sample_counts)
{
  assert(circle_counts.size && sample_counts.size);
  assert(circle_counts.size == sample_counts.size);
  size_t n_jobs = circle_counts.size;
  // number of samples inside the unit circle, total number of samples drawn
  size_t n_inside = 0;
  size_t n_total = 0;
  for (size_t i = 0; i < n_jobs; i++) {
    n_inside += circle_counts.data[i];
    n_total += sample_counts.data[i];
  }
  return 4 * ((double) n_inside / n_total);
}

#ifdef _OPENMP
/**
 * Parallel estimation of pi through Monte Carlo by using OpenMP directives.
 *
 * Implicit map-reduce using OpenMP to manage the thread pool. If `n_threads`
 * is set to `PDMPMT_AUTO_OMP_JOBS`, i.e. 0, OpenMP sets the thread count.
 *
 * @param n_samples Number of samples to draw
 * @param rng_type RNG type
 * @param n_threads Number of OpenMP threads to split work over
 * @param seed Seed value for the PRNG
 */
double
pdmpmt_rng_smcpi_ompm(
  size_t n_samples,
  pdmpmt_rng_type rng_type,
  unsigned int n_threads,
  unsigned long seed)
{
  if (n_threads)
    omp_set_num_threads(n_threads);
  // generate seeds used by jobs for generating samples + the sample counts
  pdmpmt_block_ulong seeds, sample_counts;
  seeds = pdmpmt_rng_generate_seeds(n_threads, rng_type, seed);
  sample_counts = pdmpmt_generate_sample_counts(n_samples, n_threads);
  // compute circle counts with OpenMP
  pdmpmt_block_ulong circle_counts = pdmpmt_block_ulong_alloc(n_threads);
// for MSVC, since its OpenMP version is quite old (2.0), must use signed var.
// furthermore, unlike when compiling C++ code, cannot use C99-style loop, even
// with C11 language specification (/std:c11) passed to compiler
#ifdef _MSC_VER
  int i;
// could use C99 loop for GCC/Clang, but this is less typing
#else
  unsigned int i;
#endif  // _MSC_VER
// MSVC complains about signed/unsigned mismatch in the loop condition and
// that the circle_counts->data[i] assignment of size_t to ulong loses data
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4018 4267)
  #pragma omp parallel for
  for (i = 0; i < n_threads; i++) {
    circle_counts.data[i] = pdmpmt_rng_unit_circle_samples(
      sample_counts.data[i], rng_type, seeds.data[i]
    );
PDMPMT_MSVC_WARNING_POP()
  }
  // get pi estimate, clean up, and return
  double pi_hat = pdmpmt_mcpi_gather(circle_counts, sample_counts);
  pdmpmt_block_ulong_free(&seeds);
  pdmpmt_block_ulong_free(&circle_counts);
  pdmpmt_block_ulong_free(&sample_counts);
  return pi_hat;
}
#endif  // _OPENMP
