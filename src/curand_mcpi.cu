/**
 * @file curand_mcpi.cu
 * @author Derek Huang
 * @brief CUDA C++ program computing pi using Monte Carlo with cuRAND
 */

#include <cstdlib>
#include <iostream>
#include <utility>

#include <curand.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/reduce.h>
#include <thrust/version.h>

#include "pdmpmt/common.h"
#include "pdmpmt/cuda_runtime.hh"

namespace {

/**
 * Return the `curandStatus_t` identifier string for the given error status.
 *
 * This covers all the cuRAND status values for cuRAND 10.3.3.
 *
 * @param err cuRAND error status
 */
constexpr auto curand_strerror(curandStatus err) noexcept
{
  switch (err) {
#define ERROR_CASE(x) case x: return #x;
  ERROR_CASE(CURAND_STATUS_SUCCESS)
  ERROR_CASE(CURAND_STATUS_VERSION_MISMATCH)
  ERROR_CASE(CURAND_STATUS_NOT_INITIALIZED)
  ERROR_CASE(CURAND_STATUS_ALLOCATION_FAILED)
  ERROR_CASE(CURAND_STATUS_TYPE_ERROR)
  ERROR_CASE(CURAND_STATUS_OUT_OF_RANGE)
  ERROR_CASE(CURAND_STATUS_LENGTH_NOT_MULTIPLE)
  ERROR_CASE(CURAND_STATUS_DOUBLE_PRECISION_REQUIRED)
  ERROR_CASE(CURAND_STATUS_LAUNCH_FAILURE)
  ERROR_CASE(CURAND_STATUS_PREEXISTING_FAILURE)
  ERROR_CASE(CURAND_STATUS_INITIALIZATION_FAILED)
  ERROR_CASE(CURAND_STATUS_ARCH_MISMATCH)
  ERROR_CASE(CURAND_STATUS_INTERNAL_ERROR)
  default:
    return "(unknown)";
#undef ERROR_CASE
  }
}

/**
 * cuRNAD error handler type.
 */
struct curand_error_handler_type {};

/**
 * Print an error message and exit if a cuRAND status is an error status.
 */
void operator<<(curand_error_handler_type /*handler*/, curandStatus_t status)
{
  if (status == CURAND_STATUS_SUCCESS)
    return;
  // handle error
  std::cerr << "cuRAND error: " << curand_strerror(status) << std::endl;
  std::exit(EXIT_FAILURE);
}

/**
 * Error handler global for cuRAND functions.
 */
constexpr curand_error_handler_type curand_check;

/**
 * cuRAND generator class.
 *
 * This provides a scoping mechanism for managing a cuRAND generator.
 */
class curand_generator {
public:
  /**
   * Default ctor.
   *
   * The created generator will be a MT19937 Mersenne Twister generator.
   */
  curand_generator() : curand_generator{CURAND_RNG_PSEUDO_MT19937} {}

  /**
   * Ctor.
   *
   * Create a generator of the specified type.
   *
   * @param type cuRAND generator type
   */
  curand_generator(curandRngType_t type) : type_{type}
  {
    curand_check << curandCreateGenerator(&gen_, type);
  }

  /**
   * Deleted copy ctor.
   */
  curand_generator(const curand_generator&) = delete;

  /**
   * Move ctor.
   */
  curand_generator(curand_generator&& other) noexcept
  {
    from(std::move(other));
  }

  /**
   * Dtor.
   */
  ~curand_generator()
  {
    destroy();
  }

  /**
   * Move assignment operator.
   */
  auto& operator=(curand_generator&& other) noexcept
  {
    destroy();
    from(std::move(other));
    return *this;
  }

  /**
   * Enable implicit conversion to `curandGenerator_t` for cuRAND interop.
   *
   * This can also be used to indicate if the generator is valid or not.
   */
  operator curandGenerator_t() const noexcept
  {
    return gen_;
  }

private:
  curandRngType_t type_;
  curandGenerator_t gen_{};

  /**
   * Move-initialize from another `curand_generator`.
   *
   * On completion the other `curand_generator` will have a null generator.
   */
  void from(curand_generator&& other) noexcept
  {
    gen_ = other.gen_;
    other.gen_ = nullptr;
  }

  /**
   * Destroy the cuRAND generator.
   *
   * This will never fail since the cuRAND generator wlll have been created.
   */
  void destroy() noexcept
  {
    if (gen_)
      curandDestroyGenerator(gen_);
  }
};

/**
 * 1D span for CUDA code.
 */
template <typename T>
class span {
public:
  /**
   * Ctor.
   *
   * Creates an empty span.
   */
  PDMPMT_XPU_FUNC
  span() noexcept = default;

  /**
   * Ctor.
   *
   * @param data Data buffer
   * @param size Buffer element count.
   */
  PDMPMT_XPU_FUNC
  span(T* data, std::size_t size) noexcept : data_{data}, size_{size} {}

  /**
   * Ctor.
   *
   * Create a span from a Thrust device vector.
   *
   * @param vec Thrust device vector
   */
  span(thrust::device_vector<T>& vec) noexcept
    : data_{vec.data().get()}, size_{vec.size()}
  {}

  /**
   * Return the data pointer.
   */
  PDMPMT_XPU_FUNC
  auto data() const noexcept { return data_; }

  /**
   * Return the element count.
   */
  PDMPMT_XPU_FUNC
  auto size() const noexcept { return size_; }

  /**
   * Return the `i`th element in the span.
   */
  PDMPMT_XPU_FUNC
  auto& operator[](std::size_t i) noexcept
  {
    return data_[i];
  }

  /**
   * Return the `i`th element in the span.
   */
  PDMPMT_XPU_FUNC
  const auto& operator[](std::size_t i) const noexcept
  {
    return data_[i];
  }

  /**
   * Return an iterator to the first element in the span.
   */
  PDMPMT_XPU_FUNC
  auto begin() const noexcept
  {
    return data_;
  }

  /**
   * Return an iterator one past the last element in the span.
   */
  PDMPMT_XPU_FUNC
  auto end() const noexcept
  {
    return data_;
  }

private:
  T* data_{};
  std::size_t size_{};
};

/**
 * Check how many points fall within the quarter-unit circle.
 *
 * All dimensions are assumed to be 1D.
 *
 * @tparam T Floating type
 *
 * @param ns Number of samples
 * @param xs x-axis samples in (0, 1]
 * @param ys y-axis sampels in (0, 1]
 * @param cts Per-thread counts of points within the quarter-unit circle
 */
template <typename T>
__global__ void
unit_circle_check(const span<T> xs, const span<T> ys, span<unsigned> cts)
{
  // get number of samples and number of threads
  // note: assumes xs and ys have the same size
  auto ns = xs.size();
  auto nt = cts.size();
  // get the thread index
  auto ti = blockIdx.x * blockDim.x + threadIdx.x;
  // compute starting index
  // determine work unit size (last thread gets the remaining work)
  auto work_size = ns / nt;
  if (ti == nt - 1)
    work_size += ns % nt;
  // starting index offset
  auto offset = ns / nt * ti;
  // count number of points in the quarter-unit circle
  unsigned n_in = 0u;
  for (auto i = offset; i < offset + work_size; i++)
    n_in += (xs[i] * xs[i] + ys[i] * ys[i] <= 1);
  // update per-thread count vector
  cts[ti] = n_in;
}

}  // namespace

int main()
{
  // cuRAND version info
  std::cout << "cuRAND version: " <<
    CURAND_VER_MAJOR << "." << CURAND_VER_MINOR << "." << CURAND_VER_PATCH <<
    std::endl;
  // seed + sample count
  constexpr auto seed = 8888u;
  constexpr auto n_samples = 10'000'000u;
  // number of threads per block + number of blocks + total thread count
  constexpr auto n_block_threads = 256u;
  constexpr auto n_blocks = 2048u;
  constexpr auto n_threads = n_block_threads * n_blocks;
  // create cuRAND Mersenne Twister generator using best possible memory order
  curand_generator gen{CURAND_RNG_PSEUDO_MT19937};
  curand_check << curandSetPseudoRandomGeneratorSeed(gen, seed);
  curand_check << curandSetGeneratorOrdering(gen, CURAND_ORDERING_PSEUDO_BEST);
  // create device vectors for x and y point coordinates in (0, 1]
  thrust::device_vector<float> xs(n_samples);
  thrust::device_vector<float> ys(n_samples);
  // create device vector to hold per-thread counts of points in circle
  thrust::device_vector<unsigned> cts(n_threads);
  // create spans for device memory from the vectors
  span xs_view{xs};
  span ys_view{ys};
  span cts_view{cts};
  // fill device vectors with uniform values + block until completion
  curand_check << curandGenerateUniform(gen, xs_view.data(), xs_view.size());
  curand_check << curandGenerateUniform(gen, ys_view.data(), ys_view.size());
  cudaDeviceSynchronize();
  PDMPMT_CUDA_THROW_IF_ERROR();
  // launch kernel to count number of points in the quarter-unit circle
  unit_circle_check<<<n_blocks, n_block_threads>>>(xs_view, ys_view, cts_view);
  cudaDeviceSynchronize();
  PDMPMT_CUDA_THROW_IF_ERROR();
  // accumulate all values to get final count + compute pi
  auto n_in = thrust::reduce(thrust::device, cts.begin(), cts.end(), 0u);
  auto pi = (4. * n_in) / n_samples;
  // print
  std::cout << "pi (n_threads=" << n_threads << ", n_samples=" << n_samples <<
    ", seed=" << seed << "): " << pi << std::endl;
  return EXIT_SUCCESS;
}
