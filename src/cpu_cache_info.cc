/**
 * @file cpu_cache_info.cc
 * @author Derek Huang
 * @brief C++ program printing CPU cache information for each logical CPU
 * @copyright MIT License
 *
 * To handle NUMA architectures like Intel's Alder Lake this program groups the
 * CPU identifiers together when displaying cache information for succinctness.
 */

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
// for sched_(set|get)affinity and other GNU sched.h extensions
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif  // _GNU_SOURCE
#include <sched.h>
#include <sys/sysinfo.h>
#endif  // !defined(_WIN32)

#ifdef _WIN32
#include <cerrno>
#endif  // _WIN32
#include <cstdlib>
#include <iostream>
#include <system_error>

#include "pdmpmt/cpu_cache_info.hh"
#include "pdmpmt/warnings.h"

namespace {

/**
 * Simple RAII wrapper for a dynamically-allocated `cpu_set_t` or `DWORD_PTR`.
 *
 * Note that on Windows the allocation size is always fixed to 64.
 */
class unique_cpu_set {
public:
  /**
   * Return the CPU set corresponding to the current CPU affinity mask.
   *
   * This uses `sched_getaffinity()` for the current thread on Linux and the
   * process affinity mask from `GetProcessAffinityMask()` on Windows.
   */
  static auto current()
  {
    // zeroed CPU set
    unique_cpu_set cpus;
#if defined(_WIN32)
    // get process + system (unused) CPU affinity masks for current group
    DWORD_PTR proc_mask;
    DWORD_PTR sys_mask;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask))
      throw std::system_error{
        {static_cast<int>(GetLastError()), std::system_category()},
        "GetProcessAffinityMask() on current process"
      };
    // update using process mask
    cpus.set_ = proc_mask;
#else
    // get CPU affinity for current thread
    if (sched_getaffinity(0, cpus.alloc_size_, cpus.set_) < 0)
      throw std::system_error{
        {errno, std::system_category()},
        "sched_getaffinity() on current thread"
      };
#endif  // !defined(_WIN32)
  }
  /**
   * Default ctor.
   *
   * This constructs a zeroed CPU set using the value of `get_nprocs_conf()` on
   * Linux and the value of `dwNumberOfProcessors` from the `SYSTEM_INFO`
   * populated by `GetSystemInfo()` on Windows.
   */
  unique_cpu_set()
  {
#if defined(_WIN32)
    // get CPU count, allocation size, and zero
    size_ = []
    {
      SYSTEM_INFO info;
      GetSystemInfo(&info);
      return info.dwNumberOfProcessors;
    }();
    alloc_size_ = sizeof(set_);
    set_ = 0u;
#else
    // get CPU count + set allocation size
    size_ = get_nprocs_conf();
    // note: we know that get_nprocs_conf() will be less than INT_MAX
    alloc_size_ = CPU_ALLOC_SIZE(static_cast<int>(size_));
    // allocate, handle error, zero
    set_ = CPU_ALLOC(size_);
    if (!set_)
      throw std::system_error{{errno, std::system_category()}, "CPU_ALLOC()"};
    CPU_ZERO_S(alloc_size_, set_);
#endif  // !defined(_WIN32)
  }

  /**
   * Deleted copy ctor.
   */
  unique_cpu_set(const unique_cpu_set&) = delete;

  /**
   * Move ctor.
   */
  unique_cpu_set(unique_cpu_set&& other) noexcept
  {
    from(std::move(other));
  }

  /**
   * Move assignment operator.
   */
  auto& operator=(unique_cpu_set&& other) noexcept
  {
    destroy();
    from(std::move(other));
    return *this;
  }

  /**
   * Dtor.
   */
  ~unique_cpu_set()
  {
    destroy();
  }

  /**
   * Return the raw CPU set.
   *
   * Since this varies across platforms it should be treated opaquely.
   */
  auto set() const noexcept { return set_; }

  /**
   * Return the CPU set allocation size in bytes.
   */
  auto alloc_size() const noexcept { return alloc_size_; }

  /**
   * Return the number of CPUs represented by the set.
   */
  auto size() const noexcept { return size_; }

  /**
   * Set the corresponding entry for the given CPU index.
   *
   * Nothing is done if the index value is out of range.
   */
  auto& set(std::size_t cpu) noexcept
  {
    if (cpu < size_) {
#if defined(_WIN32)
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4334)
      set_ |= (1u << cpu);
PDMPMT_MSVC_WARNING_POP()
#else
      CPU_SET_S(cpu, alloc_size_, set_);
#endif  // !defined(_WIN32)
    }
    return *this;
  }

  /**
   * Unset the corresponding entry for the given CPU index.
   *
   * Nothing is done if the index value is out of range.
   */
  auto& unset(std::size_t cpu) noexcept
  {
    if (cpu < size_) {
#if defined(_WIN32)
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4319)
      set_ &= ~(1u << cpu);
PDMPMT_MSVC_WARNING_POP()
#else
      CPU_CLR_S(cpu, alloc_size_, set_);
#endif  // !defined(_WIN32)
    }
    return *this;
  }

private:
#if defined(_WIN32)
  DWORD_PTR set_;           // CPU set mask
#else
  cpu_set_t* set_;          // CPU set pointer
#endif  // !defined(_WIN32)
  std::size_t alloc_size_;  // cpu_set_t allocation size in bytes
  std::size_t size_;        // CPUs represented by the set

  /**
   * Initialize via move from another `unique_cpu_set`.
   *
   * The moved-from `unique_cpu_set` will be completely zeroed..
   */
  void from(unique_cpu_set&& other) noexcept
  {
    set_ = other.set_;
    alloc_size_ = other.alloc_size_;
    size_ = other.size_;
#if defined(_WIN32)
    other.set_ = 0u;
#else
    other.set_ = nullptr;
#endif  // !defined(_WIN32)
    other.alloc_size_ = 0u;
    other.size_ = 0u;
  }

  /**
   * Deallocate the `cpu_set_t` if the pointer is not `nullptr`.
   *
   * On Windows this has no effect as the `DWORD_PTR` is a scalar.
   */
  void destroy() noexcept
  {
#ifndef _WIN32
    if (set_)
      CPU_FREE(set_);
#endif  // _WIN32
  }
};

}  // namespace

int main()
{
  // zeroed CPU set for all logical processors
  unique_cpu_set cpus;
  // for each logical processor
  for (auto i = 0u; i < cpus.size(); i++) {
    // set thread/process affinity to a specified processor
    cpus.set(i);
#if defined(_WIN32)
    if (!SetProcessAffinityMask(GetCurrentProcess(), cpus.set()))
      throw std::system_error{
        static_cast<int>(GetLastError()),
        std::system_category(),
        "SetProcessAffinityMask() on current process"
      };
#else
    if (sched_setaffinity(0, cpus.alloc_size(), cpus.set()) < 0)
      throw std::system_error{
        {errno, std::system_category()},
        "sched_setaffinity() on current thread"
      };
#endif  // !defined(_WIN32)
    // get + print CPU cache info
    pdmpmt::cpu_cache_info info;
    std::cout <<
      "Logical CPU " << i << ":\n" <<
      "  L1I: " << info.l1i().size<1024>() << "K\n" <<
      "  L1D: " << info.l1d().size<1024>() << "K\n" <<
      // note: L2 cache topology is not reflected on e.g. NUMA architectures
      // like Alder Lake processors where L2 cache topology is more intricate.
      // L3 caches are also typically shared by all physical/logical CPUs
      "  L2C: " << info.l2c().size<(1 << 20)>() << "M\n" <<
      "  L3C: " << info.l3c().size<(1 << 20)>() << "M\n" << std::flush;
    // unset for next iteration
    cpus.unset(i);
  }
  return EXIT_SUCCESS;
}
