/**
 * @file cpu_info.cc
 * @author Derek Huang
 * @brief C++ program printing CPU info
 * @copyright MIT License
 *
 * This program calls uses the `cpuid` instruction on x86 systems to determine
 * the processor name, signature, and supported instruction set features. There
 * is the implicit assumption that all logical processors have the *same* CPU
 * features and are running on the *same* physical package or socket. In
 * practice even hybrid architectures like Alder Lake, which has both Golden
 * Cove and Gracemont cores, all cores have the same CPU features (e.g. AVX-512
 * is disabled on the Golden Cove cores in Alder Lake), as it would be bad if
 * the OS rescheduled a program executing AVX instructions onto a core that did
 * not support AVX instructions (a `SIGILL` would be generated). However, one
 * *could* theoretically have a server with multiple sockets where each CPU has
 * different instruction set capabilities, although this is very unlikely.
 *
 * Truly accurate `cpuid` reporting on multiprocessor systems would require
 * some knowledge of the hardware topology and `cpuid` would be called on each
 * physical core, for each physical CPU package (socket).
 */

#include <cstdlib>
#include <ios>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "pdmpmt/cpu_info.hh"
#include "pdmpmt/format.hh"

namespace {

}  // namespace

int main()
{
  // get CPU info + hypervisor info
  pdmpmt::cpu_info info;
  pdmpmt::cpu_virt_info vinfo;
  // print
  std::cout <<
    "Vendor: " << info.vendor() << "\n" <<
    "Signature: 0x" << pdmpmt::hex << info.signature() << "\n" <<
    "Max Leaf: 0x" << pdmpmt::hex << info.max_leaf() << std::endl;
  // if hypervisor info was collected, print hypervisor info
  if (vinfo) {
    std::cout <<
      "  Hypervisor: " << vinfo.vendor() << "\n" <<
      "  Interface: 0x" << pdmpmt::hex << vinfo.interface() << "\n" <<
      "  Max Leaf: 0x" << pdmpmt::hex << vinfo.max_leaf() << std::endl;
  }
  // print CPU features
  std::cout <<
    "Features:\n" <<
    "  FPU               " << info.fpu() << "\n" <<
    "  CMOV              " << info.cmov() << "\n" <<
    "  MMX               " << info.mmx() << "\n" <<
    "  SSE               " << info.sse() << "\n" <<
    "  SSE2              " << info.sse2() << "\n" <<
    "  SSE3              " << info.sse3() << "\n" <<
    "  PCLMULQDQ         " << info.pclmulqdq() << "\n" <<
    "  SSSE3             " << info.ssse3() << "\n" <<
    "  FMA               " << info.fma() << "\n" <<
    "  SSE4.1            " << info.sse4_1() << "\n" <<
    "  SSE4.2            " << info.sse4_2() << "\n" <<
    "  POPCNT            " << info.popcnt() << "\n" <<
    "  AES               " << info.aes() << "\n" <<
    "  AVX               " << info.avx() << "\n" <<
    "  F16C              " << info.f16c() << "\n" <<
    "  BMI1              " << info.bmi1() << "\n" <<
    "  AVX2              " << info.avx2() << "\n" <<
    "  BMI2              " << info.bmi2() << "\n" <<
    "  AVX-512F          " << info.avx512f() << "\n" <<
    "  AVX-512DQ         " << info.avx512dq() << "\n" <<
    "  AVX-512IFMA       " << info.avx512ifma() << "\n" <<
    "  AVX-512PF         " << info.avx512pf() << "\n" <<
    "  AVX-512ER         " << info.avx512er() << "\n" <<
    "  AVX-512CD         " << info.avx512cd() << "\n" <<
    "  SHA               " << info.sha() << "\n" <<
    "  AVX-512BW         " << info.avx512bw() << "\n" <<
    "  AVX-512VL         " << info.avx512vl() << "\n" <<
    "  AVX-512VBMI       " << info.avx512vbmi() << "\n" <<
    "  AVX-512VBMI2      " << info.avx512vbmi2() << "\n" <<
    "  GFNI              " << info.gfni() << "\n" <<
    "  VAES              " << info.vaes() << "\n" <<
    "  VPCLMULQDQ        " << info.vpclmulqdq() << "\n" <<
    "  AVX-512VNNI       " << info.avx512vnni() << "\n" <<
    "  AVX-512BITALG     " << info.avx512bitalg() << "\n" <<
    "  AVX-512VPOPCNTDQ  " << info.avx512vpopcntdq() << "\n" <<
    "  AMX-BF16          " << info.amxbf16() << "\n" <<
    "  AVX-512FP16       " << info.avx512fp16() << "\n" <<
    "  AMX-TILE          " << info.amxtile() << "\n" <<
    "  AMX-INT8          " << info.amxint8() << "\n" <<
    "  SHA-512           " << info.sha512() << "\n" <<
    "  AVX-VNNI          " << info.avxvnni() << "\n" <<
    "  AVX-512BF16       " << info.avx512bf16() << "\n" <<
    "  AMX-FP16          " << info.amxfp16() << "\n" <<
    "  AVX-IFMA          " << info.avxifma() << "\n" <<
    "  AVX10.1           " << info.avx10_1() << "\n" <<
    "  AVX10.2           " << info.avx10_2() << "\n" << std::flush;
  return EXIT_SUCCESS;
}
