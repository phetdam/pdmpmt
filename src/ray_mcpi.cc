/**
 * @file ray_mcpi.cc
 * @author Derek Huang
 * @brief C++ library for Monte Carlo pi estimation using Ray's C++ API
 * @copyright MIT License
 *
 * This file gets compiled into both a DSO and an executable because Ray
 * resolves the registered `RAY_REMOTE` functions by loading DSOs from given
 * search directories and scanning them for visible symbols. In a more
 * productionized setting, we would probably want headers for exported symbols
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

// when compiling as DSO, we need to ensure symbols are appropriately exported
#if defined(PDMPMT_BUILD_RAY_MCPI_REMOTE)
// symbol export macro. although Ray isn't supported on Windows yet, it's not
// hard for us to be consistent here. we need this anyways since it's possible
// to compile an ELF DSO with default hidden symbol visibility
#if defined(_MSC_VER)
#define PDMPMT_RAY_MCPI_EXPORT __declspec(dllexport)
#else
#define PDMPMT_RAY_MCPI_EXPORT __attribute__((visibility("default")))
#endif  // !defined(_MSC_VER)
// otherwise, it's fine to keep the visibility hidden
#else
// nothing special to do with MSVC
// TODO: may affect name mangling if we don't have the same __declspec?
#if defined(_MSC_VER)
#define PDMPMT_RAY_MCPI_EXPORT
#else
#define PDMPMT_RAY_MCPI_EXPORT __attribute__((visibility("hidden")))
#endif  // !defined(_MSC_VER)
#endif  // !defined(PDMPMT_BUILD_RAY_MCPI_REMOTE)

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
  return pdmpmt::detail::unit_circle_samples(n_samples, std::mt19937{seed});
}

// convenience type alias
using count_type = decltype(unit_circle_samples(0u, 0u));

// note: both DSO *and* executable need to register the function
// note: ray::internal::RegisterRemoteFunctions does not use the :: global
// qualifier so if this namespace was pdmpmt::ray::, name lookup would fail
RAY_REMOTE(unit_circle_samples);

}  // namespace remote
}  // namespace pdmpmt

// note: even though helpers + main() are not necessary for the DSO, if these
// are removed, the linker will place the RAY_REMOTE functions at different
// addresses which will cause the Ray runtime to fail to look the functions up.
// this is because Ray function registration maps addresses to names.
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
  // note: ray::Init() doesn't seem to work well with CLI arguments
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
