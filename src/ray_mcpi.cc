/**
 * @file ray_mcpi.cc
 * @author Derek Huang
 * @brief C++ library for Monte Carlo pi estimation using Ray's C++ API
 * @copyright MIT License
 *
 * This file gets compiled into both a DSO and an executable for convenience
 * like the Ray `example.cc` sample. The Ray runtime will load DSOs at runtime
 * from given search directories to resolve `RAY_REMOTE` registered functions;
 * these functions do *not* necessarily need be visible.
 *
 * In a production setting, we would probably want headers for exported symbols
 * with `RAY_REMOTE` registration so any consuming executable can compile, with
 * the implementations of the free/member functions in a shared library.
 */

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include <ray/api.h>

#include "pdmpmt/mcpi.hh"
#include "pdmpmt/ray.hh"

// namespace is unnecessary in this context but would realistically be used
// TODO: consider if we need extern "C". both Itanium and VC140 ABIs are stable
// nowadays but it's still not great to rely on mangled names
namespace pdmpmt {
namespace remote {

/**
 * Count the number of samples that fall in the unit circle.
 *
 * This is a non-template function using the `std::mt19937` PRNG that wraps
 * the actual `detail::unit_circle_samples` template. The actual instantiation
 * of the `std::mt19937` is done by `detail::unit_circle_samples` since the
 * RNG instance is taken by value as a copy.
 *
 * @param n_samples Number of samples to take
 * @param seed Seed value for the `std::mt19937`
 */
auto unit_circle_samples(std::size_t n_samples, std::uint_fast32_t seed)
{
  std::mt19937 rng{seed};
  return pdmpmt::detail::unit_circle_samples(n_samples, rng);
}

// convenience type alias
using count_type = decltype(unit_circle_samples(0u, 0u));

// note: both DSO *and* executable need to register the function
// note: ray::internal::RegisterRemoteFunctions does not use the :: global
// qualifier so if this namespace was pdmpmt::ray::, name lookup would fail
RAY_REMOTE(unit_circle_samples);

}  // namespace remote
}  // namespace pdmpmt

// IMPORTANT: if libray_api.so linkage is not forced then at runtime one will
// get an error message about the function not being found
PDMPMT_FORCE_RAY_LIBRARY_LINKAGE();

// helpers + main() only need to be present for the executable not the DSO
#ifndef PDMPMT_BUILD_RAY_MCPI_REMOTE
namespace {

/**
 * Helper function to block and collect Ray object references into a vector.
 *
 * @tparam T type
 *
 * @param refs Ray object references
 */
template <typename T>
auto get(const std::vector<ray::ObjectRef<T>>& refs)
{
  std::vector<T> values(refs.size());
  // note: all object refs are assumed to contain values
  for (std::size_t i = 0u; i < refs.size(); i++)
    values[i] = *refs[i].Get();
  return values;
}

}  // namespace

/**
 * Main function.
 *
 * This closely mirrors the implementation of `mcpi_async`, with the major
 * difference being that Ray tasks + workers are used instead of `std::async`.
 */
int main(int argc, char** argv)
{
  // total number of Monte Carlo samples to take, seed, and task count
  // TODO: we can let the seed be configurable
  constexpr auto n_samples = 500000u;
  constexpr auto seed = 8765u;
  constexpr auto n_tasks = 500u;
  // initialize Ray runtime context
  // note: ray::Init() doesn't seem to work well with CLI arguments. even if
  // you pass --ray_head_args="--port 1234" you still get messages about the
  // GCS RPC client failing to connect to localhost:6379 (default Ray port is
  // 6379) despite us changing the port number to 1234 in this case
  pdmpmt::ray_runtime_context ctx;
  // print parameters
  std::cout <<
    "n_samples: " << n_samples << "\n" <<
    "seed: " << seed << "\n" <<
    "n_tasks: " << n_tasks << std::endl;
  // get seeds + sample counts for sub-sequences, each handled by an actor
  std::mt19937 rng{seed};
  auto seeds = pdmpmt::detail::generate_seeds(n_tasks, seed);
  auto tot_counts = pdmpmt::detail::generate_sample_counts(n_samples, n_tasks);
  // launch task invocations and collect object references
  std::vector<ray::ObjectRef<pdmpmt::remote::count_type>> refs(n_tasks);
  // note: you *need* to create a new task per Remote() invocation!
  for (auto i = 0u; i < n_tasks; i++)
    refs[i] = ray::Task(pdmpmt::remote::unit_circle_samples)
      .Remote(tot_counts[i], seeds[i]);
  // collect values from references, gather, and finish
  auto pi_hat = pdmpmt::detail::mcpi_gather(get(refs), tot_counts);
  std::cout << "pi_hat: " << pi_hat << std::endl;
  return EXIT_SUCCESS;
}
#endif  // PDMPMT_BUILD_RAY_MCPI_REMOTE
