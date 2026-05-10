/**
 * @file simd.cc
 * @author Derek Huang
 * @brief C++ program listing supported CPU SIMD features
 * @copyright MIT License
 */

#include <cstdlib>
#include <iostream>

#include "pdmpmt/cpu_info.hh"

int main()
{
  pdmpmt::cpu_info info;
  std::cout <<
    "MMX          " << info.mmx() << "\n" <<
    "SSE          " << info.sse() << "\n" <<
    "SSE2         " << info.sse2() << "\n" <<
    "SSE3         " << info.sse3() << "\n" <<
    "SSSE3        " << info.ssse3() << "\n" <<
    "SSE4.1       " << info.sse4_1() << "\n" <<
    "SSE4.2       " << info.sse4_2() << "\n" <<
    "AVX          " << info.avx() << "\n" <<
    "FMA          " << info.fma() << "\n" <<
    "AVX2         " << info.avx2() << "\n" <<
    "AVX-VNNI     " << info.avxvnni() << "\n" <<
    "AVX-512F     " << info.avx512f() << "\n" <<
    "AVX-512BW    " << info.avx512bw() << "\n" <<
    "AVX-512CD    " << info.avx512cd() << "\n" <<
    "AVX-512DQ    " << info.avx512dq() << "\n" <<
    "AVX-512VL    " << info.avx512vl() << "\n" <<
    "AVX-512BF16  " << info.avx512bf16() << "\n" <<
    "AVX-512FP16  " << info.avx512fp16() << "\n" << std::flush;
  return EXIT_SUCCESS;
}
