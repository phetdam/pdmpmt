/**
 * @file cpu_info.hh
 * @author Derek Huang
 * @brief C++ header for x86 CPU features helper
 * @copyright MIT License
 */

#ifndef PDMPMT_CPU_INFO_HH_
#define PDMPMT_CPU_INFO_HH_

// for __cpuid() and __cpuidex()
#if defined(_WIN32)
#include <intrin.h>
// for GCC/Clang __get_cpuid_count()
#elif defined(__GNUC__)
#include <cpuid.h>
#endif  // !defined(_WIN32) && !defined(__GNUC__)

#include <climits>
#include <type_traits>

namespace pdmpmt {

// int type representing a 32-bit register for use with cpuid
#if defined(_WIN32)
using regint = int;
#else
using regint = unsigned;
#endif  // !defined(_WIN32)

/**
 * `cpuid` wrapper function call.
 *
 * This wrapper first calls `cpuid` with leaf 0, and if the requested leaf is
 * not supported, `false` is returned. Otherwise, CPUID is invoked with the
 * given leaf and subleaf, ensuring that ecx is always correctly set.
 *
 * On Windows this wraps a call to `__cpuid()` and then `__cpuidex()` while for
 * GCC/Clang the `__get_cpuid_count()` function is used.
 *
 * @param regs eax, ebx, ecx, edx registers
 * @param leaf CPUID leaf value
 * @param sub CPUID subleaf value
 * @returns `true` if `cpuid` leaf supported, `false` otherwise
 */
inline bool cpuid(regint (&regs)[4], regint leaf = 0, regint sub = 0) noexcept
{
#if defined(_WIN32)
  // get max supported leaf value and return if not supported
  __cpuid(regs, 0);
  if (regs[0] < leaf)
    return false;
  // otherwise use both leaf and subleaf
  __cpuidex(regs, leaf, sub);
  return true;
#elif defined(__GNUC__)
  return __get_cpuid_count(leaf, sub, &regs[0], &regs[1], &regs[2], &regs[3]);
#else
  return false;
#endif  // !defined(_WIN32)
}

namespace detail {

/**
 * Test that a specified bit is set in the given integral value.
 *
 * @tparam I Bit to test
 * @tparam T Integral type
 *
 * @param v Integral value to test
 */
template <unsigned I, typename T>
bool test(T v, std::enable_if_t<std::is_integral_v<T>>* = 0) noexcept
{
  // ensure I is within bounds
  static_assert(I <= CHAR_BIT * sizeof(T), "I must be <= CHAR_BIT * sizeof(T)");
  return !!(v & (T{1} << I));
}

}  // namespace detail

/**
 * CPU info structure.
 *
 * This currently holds feature flags returned by `cpuid` for various CPU
 * compute features, e.g. if the CPU supports FMA, AVX-512F, etc. Of course,
 * use of this class is only supported on x86 machines.
 */
class cpu_info {
public:
  /**
   * Ctor.
   *
   * This calls the `cpuid` instruction and sets the vendor string, max leaf
   * value, and other feature flag values from the returned register values.
   */
  cpu_info()
  {
    // eax, ebx, ecx, edx register values
    regint regs[4];
    // get max supported leaf and vendor string
    if (!cpuid(regs))
      return;
    // copy ebx, edx, ecx values into vendor string
    // note: reinterpret_cast to handle the byte ordering
    *reinterpret_cast<regint*>(vendor_) = regs[1];
    *reinterpret_cast<regint*>(vendor_ + 4u) = regs[3];
    *reinterpret_cast<regint*>(vendor_ + 8u) = regs[2];
    // update max leaf value
    max_leaf_ = regs[0];
    // if eax = 1 is supported get info
    if (max_leaf_ >= 1) {
      cpuid(regs, 1);
      eax_1_ = regs[0];
      ecx_1_ = regs[2];
      edx_1_ = regs[3];
    }
    // if eax = 7 is supported get info
    if (max_leaf_ >= 7) {
      cpuid(regs, 7);
      ebx_7_ = regs[1];
      ecx_7_ = regs[2];
      edx_7_ = regs[3];
      // max eax = 7 subleaf value
      auto max_sub_7 = regs[0];
      // if eax = 7, ecx = 1 is supported get info
      if (max_sub_7 >= 1) {
        cpuid(regs, 7, 1);
        eax_7_1_ = regs[0];
        edx_7_1_ = regs[2];
      }
    }
    // if eax = 0x24 is supported get AVX10 version
    if (max_leaf_ >= 0x24) {
      cpuid(regs, 0x24);
      // only want the lower 8 bits of ebx
      avx10_ver_ = static_cast<unsigned char>(regs[1] & 0xFF);
    }
  }

  /**
   * Return the null-terminated CPU vendor string.
   */
  auto vendor() const noexcept { return vendor_; }

  /**
   * Return the maximum supported `cpuid` leaf value.
   */
  auto max_leaf() const noexcept { return max_leaf_; }

  /**
   * Return the CPU signature.
   *
   * This is the value of `eax` after `cpuid` is called with `eax` = 1.
   */
  auto signature() const noexcept { return eax_1_; }

  /**
   * `fpu` feature flag indicating if an onboard x87 FPU is available.
   */
  bool fpu() const noexcept { return detail::test<0>(edx_1_); }

  /**
   * `cmov` feature flag indicating if conditional move is available.
   */
  bool cmov() const noexcept { return detail::test<15>(edx_1_); }

  /**
   * `mmx` feature flag indicating if MMX instructions are available.
   */
  bool mmx() const noexcept { return detail::test<23>(edx_1_); }

  /**
   * `sse` feature flag indicating if SSE instructions are available.
   */
  bool sse() const noexcept { return detail::test<25>(edx_1_); }

  /**
   * `sse2` feature flag indicating if SSE2 instructions are available.
   */
  bool sse2() const noexcept { return detail::test<26>(edx_1_); }

  /**
   * `sse3` feature flag indicating if SSE3 instructions are available.
   */
  bool sse3() const noexcept { return detail::test<0>(ecx_1_); }

  /**
   * `pclmulqdq` feature flag indicating if `PCLMULQDQ` is available.
   */
  bool pclmulqdq() const noexcept { return detail::test<1>(ecx_1_); }

  /**
   * `ssse3` feature flag indicating if SSSE3 instructions are available.
   */
  bool ssse3() const noexcept { return detail::test<9>(ecx_1_); }

  /**
   * `fma` feature flag indicating if FMA3 instructions are available.
   */
  bool fma() const noexcept { return detail::test<12>(ecx_1_); }

  /**
   * `sse4.1` feature flag indicating if SSE4.1 instructions are available.
   */
  bool sse4_1() const noexcept { return detail::test<19>(ecx_1_); }

  /**
   * `sse4.2` feature flag indicating if SSE4.2 instructions are available.
   */
  bool sse4_2() const noexcept { return detail::test<20>(ecx_1_); }

  /**
   * `popcnt` feature flag indicating if `POPCNT` is available.
   */
  bool popcnt() const noexcept { return detail::test<23>(ecx_1_); }

  /**
   * `aes-ni` feature flag indicating if AES instructions are available.
   */
  bool aes() const noexcept { return detail::test<25>(ecx_1_); }

  /**
   * `avx` feature flag indicating if AVX instructions are available.
   */
  bool avx() const noexcept { return detail::test<28>(ecx_1_); }

  /**
   * `f16c` feature flag indicating if FP16 conversions are available.
   */
  bool f16c() const noexcept { return detail::test<29>(ecx_1_); }

  /**
   * `hypervisor` feature flag indicating if running under a hypervisor.
   */
  bool hypervisor() const noexcept { return detail::test<31>(ecx_1_); }

  /**
   * `bmi1` feature flag indicating if BMI1 instructions are available.
   */
  bool bmi1() const noexcept { return detail::test<3>(ebx_7_); }

  /**
   * `avx2` feature flag indicating if AVX2 instructions are available.
   */
  bool avx2() const noexcept { return detail::test<5>(ebx_7_); }

  /**
   * `bmi2` feature flag indicating if BMI2 instructions are available.
   */
  bool bmi2() const noexcept { return detail::test<8>(ebx_7_); }

  /**
   * `avx512-f` feature flag indicating if AVX-512F instructions are available.
   */
  bool avx512f() const noexcept { return detail::test<16>(ebx_7_); }

  /**
   * `avx512-dq` flag indicating if AVX-512DQ instructions are available.
   */
  bool avx512dq() const noexcept { return detail::test<17>(ebx_7_); }

  /**
   * `avx512-ifma` flag indicating if AVX-512IFMA instructions are available.
   */
  bool avx512ifma() const noexcept { return detail::test<21>(ebx_7_); }

  /**
   * `avx512-pf` flag indicating if AVX-512PF instructions are available.
   */
  bool avx512pf() const noexcept { return detail::test<26>(ebx_7_); }

  /**
   * `avx512-er` flag indicating if AVX-512ER instructions are available.
   */
  bool avx512er() const noexcept { return detail::test<27>(ebx_7_); }

  /**
   * `avx512-cd` flag indicating if AVX-512CD instructions are available.
   */
  bool avx512cd() const noexcept { return detail::test<28>(ebx_7_); }

  /**
   * `sha` flag indicating if SHA-1 and SHA-256 instructions are available.
   */
  bool sha() const noexcept { return detail::test<29>(ebx_7_); }

  /**
   * `avx512-bw` flag indicating if AVX-512BW instructions are available.
   */
  bool avx512bw() const noexcept { return detail::test<30>(ebx_7_); }

  /**
   * `avx512-vl` flag indicating if AVX-512VL instructions are available.
   */
  bool avx512vl() const noexcept { return detail::test<31>(ebx_7_); }

  /**
   * `avx512-vbmi` flag indicating if AVX-512VBMI instructions are available.
   */
  bool avx512vbmi() const noexcept { return detail::test<1>(ecx_7_); }

  /**
   * `avx512-vbmi2` flag indicating if AVX-512VBMI2 instructions are available.
   */
  bool avx512vbmi2() const noexcept { return detail::test<6>(ecx_7_); }

  /**
   * `gfni` flag indicating if Galois field new instructions are available.
   */
  bool gfni() const noexcept { return detail::test<8>(ecx_7_); }

  /**
   * `vaes` flag indicating if vector AES instructions are available.
   */
  bool vaes() const noexcept { return detail::test<9>(ecx_7_); }

  /**
   * `vpclmulqdq` flag to check if `VPCLMULQDQ` is available.
   */
  bool vpclmulqdq() const noexcept { return detail::test<10>(ecx_7_); }

  /**
   * `avx512-vnni` flag indicating if AVX-512VNNI instructions are available.
   */
  bool avx512vnni() const noexcept { return detail::test<11>(ecx_7_); }

  /**
   * `avx512-bitalg` flag indicating if AVX-512 BITALG is available.
   */
  bool avx512bitalg() const noexcept { return detail::test<12>(ecx_7_); }

  /**
   * `avx512-vpopcntdq` flag indicating if AVX-512 `VPOPCNTDQ` is available.
   */
  bool avx512vpopcntdq() const noexcept { return detail::test<14>(ecx_7_); }

  // TODO: could add more AVX-512 testers, e.g. for 4-register VNNI

  /**
   * `amx-bf16` flag indicating if `bfloat16` AMX instructions are available.
   */
  bool amxbf16() const noexcept { return detail::test<22>(edx_7_); }

  /**
   * `avx512-fp16` flag indicating if AVX-512 FP16 instructions are available.
   */
  bool avx512fp16() const noexcept { return detail::test<23>(edx_7_); }

  /**
   * `amx-tile` flag indicating if AMX tile load/store is available.
   */
  bool amxtile() const noexcept { return detail::test<24>(edx_7_); }

  /**
   * `amx-int8` flag indicating if AMX int8 instructions are available.
   */
  bool amxint8() const noexcept { return detail::test<25>(edx_7_); }

  /**
   * `sha512` feature flag indicating if SHA-512 instructions are available.
   */
  bool sha512() const noexcept { return detail::test<0>(eax_7_1_); }

  /**
   * `avx-vnni` feature flag indicating if AVX-VNNI instructions are available.
   */
  bool avxvnni() const noexcept { return detail::test<4>(eax_7_1_); }

  /**
   * `avx512-bf16` flag indicating if AVX-512BF16 instructions are available.
   */
  bool avx512bf16() const noexcept { return detail::test<5>(eax_7_1_); }

  /**
   * `amx-fp16` flag indicating if AMX FP16 instructions are available.
   */
  bool amxfp16() const noexcept { return detail::test<21>(eax_7_1_); }

  /**
   * `avx-ifma` flag indicating if AVX-IFMA instructions are available.
   */
  bool avxifma() const noexcept { return detail::test<23>(eax_7_1_); }

  /**
   * `avx10` flag indicating if AVX10 instructions are available.
   */
  bool avx10() const noexcept { return detail::test<19>(edx_7_1_); }

  /**
   * Indicate if AVX10.1 instructions are available.
   */
  bool avx10_1() const noexcept { return detail::test<0>(avx10_ver_); }

  /**
   * Indicate if AVX10.2 instructions are available.
   */
  bool avx10_2() const noexcept { return detail::test<1>(avx10_ver_); }

private:
  char vendor_[13]{};          // null-terminated vendor string
  regint max_leaf_{};          // maximum supported leaf value
  regint eax_1_{};             // eax = 1 cpuid eax info
  regint ecx_1_{};             // eax = 1 cpuid ecx info
  regint edx_1_{};             // eax = 1 cpuid edx info
  regint ebx_7_{};             // eax = 7, ecx = 0 cpuid ebx info
  regint ecx_7_{};             // eax = 7, ecx = 0 cpuid ecx info
  regint edx_7_{};             // eax = 7, ecx = 0 cpuid edx info
  regint eax_7_1_{};           // eax = 7, ecx = 1 cpuid eax info
  regint edx_7_1_{};           // eax = 7, ecx = 1 cpuid ecx info
  unsigned char avx10_ver_{};  // eax = 24, ecx = 0 cpuid AVX10 version
};

}  // namespace pdmpmt

#endif  // PDMPMT_CPU_INFO_HH_
