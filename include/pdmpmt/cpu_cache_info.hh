/**
 * @file cpu_cache_info.hh
 * @author Derek Huang
 * @brief C++ header for x86 cache info helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_CPU_CACHE_INFO_HH_
#define PDMPMT_CPU_CACHE_INFO_HH_

// TODO: should we add a check to see if the machine is x86?
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__)
#include <cpuid.h>
#endif  // !defined(_MSC_VER) && !defined(__GNUC__)

#include <cstring>

namespace pdmpmt {

/**
 * Type alias for a 32-bit register type.
 */
#if defined(_MSC_VER)
using regint32 = int;
#else
using regint32 = unsigned;
#endif  // !defined(_MSC_VER)

/**
 * `cpuid` wrapper that enables specifying the function subleaf as necessary.
 *
 * This ensures that `ecx` is always cleared to deal with the fact that MSVC's
 * `__cpuid()` clears `ecx` while GCC's `__get_cpuid()` does not.
 *
 * @param regs Array of 4 integers representing `eax`, `ebx, `ecx, `edx`
 * @param leaf `cpuid()` leaf value
 * @param sub `cpuid()` subleaf value for leaves that take subleafs
 *
 * @returns `true` if `cpuid` leaf is supported, `false` otherwise
 */
bool cpuid(regint32 (&regs)[4], regint32 leaf = 0, regint32 sub = 0)
{
#if defined(_MSC_VER)
  __cpuidex(regs, leaf, sub);
  return true;
#else
  return !!__get_cpuid_count(leaf, sub, &regs[0], &regs[1], &regs[2], &regs[3]);
#endif  // !defined(_MSC_VER)
}

/**
 * CPU cache information class.
 *
 * This holds L1/L2/L3 instruction/data/unified cache information returned by
 * the `cpuid` instruction with a leaf value of 4 (Intel only).
 */
class cpu_cache_info {
public:
  /**
   * Cache info class.
   *
   * This holds information about the cache line size, associativity, etc.
   */
  class entry {
  public:
    /**
     * Default ctor.
     *
     * All values are zero.
     */
    constexpr entry() noexcept = default;

    /**
     * Ctor.
     *
     * @param line_size Cache line size in bytes
     * @param parts Physical cache line partitions (lines sharing a tag)
     * @param assoc Cache associativity
     * @param sets Number of cache sets (1 for fully associative caches)
     */
    constexpr entry(
      unsigned line_size,
      unsigned parts,
      unsigned assoc,
      unsigned sets) noexcept
      : line_size_{line_size}, parts_{parts}, assoc_{assoc}, sets_{sets}
    {}

    /**
     * Return the cache line size given the specified unit.
     *
     * If the unit is non-unity the return value will be `double`.
     *
     * @tparam U Number of bytes in selected unit, e.g. `1024`
     */
    template <unsigned U = 1u>
    constexpr auto line_size() const noexcept
    {
      check_unit<U>();
      if constexpr (U == 1u)
        return line_size_;
      else
        return line_size_ / static_cast<double>(U);
    }

    /**
     * Return cache line partition size.
     *
     * This is the number of cache lines that share a cache address tag.
     */
    constexpr auto parts() const noexcept { return parts_; }

    /**
     * Return the cache associativity.
     */
    constexpr auto assoc() const noexcept { return assoc_; }

    /**
     * Return the number of cache sets.
     *
     * For fully-associative caches this returns 1.
     */
    constexpr auto sets() const noexcept { return sets_; }

    /**
     * Return the overall cache size given the specified unit.
     *
     * This is calculated by multiplying cache line size, number of physical
     * cache line partitions in the cache, cache associativity, and cache sets.
     *
     * @tparam U Number of bytes in selected unit, e.g. `1024`
     */
    template <unsigned U = 1u>
    constexpr auto size() const noexcept
    {
      check_unit<U>();
      auto unit = []
      {
        if constexpr (U == 1u)
          return U;
        else
          return static_cast<double>(U);
      }();
      return (line_size_ * parts_ * assoc_ * sets_) / unit;
    }

    /**
     * Indicate if the cache is fully associative.
     */
    constexpr bool full_assoc() const noexcept
    {
      return assoc_ == 1u;
    }

  private:
    unsigned line_size_{};  // cache line size in bytes
    unsigned parts_{};      // cache line partition size (lines sharing a tag)
    unsigned assoc_{};      // cache associativity
    unsigned sets_{};       // number of cache sets (1 for fully associative)

    /**
     * Ensure the given non-type template value unit is positive.
     *
     * @tparam N Integral value
     */
    template <unsigned N>
    constexpr void check_unit() const noexcept
    {
      static_assert(N > 0, "number of bytes in unit must be positive");
    }
  };

  /**
   * Ctor.
   *
   * This calls `cpuid` to query CPU cache information. If leaf 4 cannot be
   * used, e.g. if `eax` contains a value less than `4` after `cpuid 0`, then
   * all available cache sizes are left as zero.
   *
   * Note that on some platforms L1/L2 results may seem unstable but this is
   * *not* a bug in the code. Rather, it is due to some platforms, e.g. Alder
   * Lake processors, may have a mix of cores with different specifications.
   * Using Alder Lake processors as an example, they have a mix of Golden Cove
   * performance (P) cores and Gracemont efficiency (E) cores, which have
   * different L1I/L1D cache sizes, and even different L2 cache topologies,
   * where each P core has its own L2 cache, while each E core cluster of four
   * E cores shares a larger L2 cache. The L3 cache is shared by all cores.
   *
   * Therefore, depending on which core the `cpuid` instruction is run,
   * different values may be returned for the L1I, L1D, and L2C cache info. It
   * may be advisable to first fix the calling thread's CPU affinity mask
   * before constructing `cpu_cache_info` instances for predictable results.
   */
  cpu_cache_info() noexcept
  {
    // query maximum leaf value + break if unsupported
    regint32 regs[4];
    if (!cpuid(regs))
      return;
    // break if eax is less than 4
    if (regs[0] < 4)
      return;
    // determine vendor ID
    // note: use reinterpret_cast to ensure endianness conversion is handled
    // note: yes, the order is ebx, edx, ecx
    *reinterpret_cast<regint32*>(vendor_) = regs[1];
    *reinterpret_cast<regint32*>(vendor_ + 4u) = regs[3];
    *reinterpret_cast<regint32*>(vendor_ + 8u) = regs[2];
    // if AMD, use 0x8000001D leaf, otherwise assume Intel[-like] and use 4
    if (!std::strcmp(vendor_, "AuthenticAMD"))
      leaf_ = 0x8000001Du;
    else
      leaf_ = 4u;
    // call cpuid with leaf + increasing subleaf until there are no more caches
    regint32 subleaf = 0;
    while (cpuid(regs, leaf_, subleaf++) && !!(regs[0] & 0x1F)) {
      // get cache type: 1 for data, 2 for instruction, 3 for combined/unified
      auto type = (regs[0] & 0x1F);
      // get cache level
      auto level = ((regs[0] >> 5) & 0x7);
      // create cache entry
      entry ent{
        // cache line size in bytes
        (regs[1] & 0xFFF) + 1u,
        // cache lines sharing an address tag
        ((regs[1] >> 12) & 0x3FF) + 1u,
        // ways of cache associativity
        ((regs[1] >> 22) & 0x3FF) + 1u,
        // number of cache sets (1 if fully associative)
        !!(regs[0] & 0x200) ? 1u : (regs[2] + 1u)
      };
      // update member based on cache type and level
      switch (level) {
      // L1 cache
      case 1:
        // data
        if (type == 1)
          l1d_ = ent;
        // instruction
        else if (type == 2)
          l1i_ = ent;
        break;
      // L2 unified cache
      case 2:
        if (type == 3)
          l2c_ = ent;
        break;
      // L3 unified cache
      case 3:
        if (type == 3)
          l3c_ = ent;
        break;
      // L4 unified cache
      case 4:
        if (type == 3)
          l4c_ = ent;
        break;
      }
    }
    // done
  }

  /**
   * Return the leaf value used for querying CPU cache information.
   *
   * For Intel CPUs this should be 4 while for AMD this should be `0x8000001D`.
   * If the maximum `cpuid` leaf is less than 4, zero is returned.
   */
  auto leaf() const noexcept { return leaf_; }

  /**
   * Return the null-terminated CPU vendor string.
   */
  auto vendor() const noexcept { return vendor_; }

  /**
   * Return L1 instruction cache information.
   */
  auto& l1i() const noexcept { return l1i_; }

  /**
   * Return L1 data cache information.
   *
   * This is usually what people are interested in when it comes to L1 size.
   */
  auto& l1d() const noexcept { return l1d_; }

  /**
   * Return L2 combined cache information.
   */
  auto& l2c() const noexcept { return l2c_; }

  /**
   * Return L3 combined cache information.
   */
  auto& l3c() const noexcept { return l3c_; }

  /**
   * Return L4 combined cache information.
   */
  auto& l4c() const noexcept { return l4c_; }

private:
  unsigned leaf_{};    // leaf value used to get cache info
  char vendor_[13]{};  // null-terminated vendor string
  entry l1i_;          // L1 instruction cache
  entry l1d_;          // L1 data cache
  entry l2c_;          // L2 unified cache
  entry l3c_;          // L3 unified cache
  entry l4c_;          // L4 unified cache (uncommon)
};

}  // namespace pdmpmt

#endif  // PDMPMT_CPU_CACHE_INFO_HH_
