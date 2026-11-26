/**
 * @file features.h
 * @author Derek Huang
 * @brief C/C++ header for feature detection
 * @copyright MIT License
 */

#ifndef PDMPMT_FEATURES_H_
#define PDMPMT_FEATURES_H_

#include "pdmpmt/common.h"

// C++20
#ifdef PDMPMT_CPLUSPLUS
#if PDMPMT_CPLUSPLUS >= 202002L
#define PDMPMT_HAS_CXX20 1
#endif  // PDMPMT_CPLUSPLUS >= 202002L
#endif  // PDMPMT_CPLUSPLUS

#ifndef PDMPMT_HAS_CXX20
#define PDMPMT_HAS_CXX20 0
#endif  // PDMPMT_HAS_CXX20

// note: no "automatic" detection of external libraries via __has_include()
// because if there are multiple library versions, one or more of which are
// located in a system include location, but of an inappropriate version,
// __has_include() would locate headers for the version(s) in the system
// location(s), which may *not* be the what the target application desires.
// therefore, to prevent this "accidental" enablement, we require the build
// system to define feature macros. PDMATH_USE_<feature> macro definition will
// be used to enable, and PDMATH_HAS_<feature> will be defined by this header
// to 1 or 0 appropriately for downstream consumption.

// X11
#ifdef PDMPMT_USE_X11
#undef PDMPMT_HAS_X11
#define PDMPMT_HAS_X11 1
#endif  // PDMPMT_USE_X11

#ifndef PDMPMT_HAS_X11
#define PDMPMT_HAS_X11 0
#endif  // PDMPMT_HAS_X11

// OpenGL
#ifdef PDMPMT_USE_GL
#undef PDMPMT_HAS_GL
#define PDMPMT_HAS_GL 1
#endif  // PDMPMT_USE_GL

#ifndef PDMPMT_HAS_GL
#define PDMPMT_HAS_GL 0
#endif  // PDMPMT_HAS_GL

// GLU
#ifdef PDMPMT_USE_GLU
#undef PDMPMT_HAS_GLU
#define PDMPMT_HAS_GLU 1
#endif  // PDMPMT_USE_GLU

#ifndef PDMPMT_HAS_GLU
#define PDMPMT_HAS_GLU 0
#endif  // PDMPMT_HAS_GLU

// GLX
// note: usually GL/glx.h includes X11/Xlib.h itself
#ifdef PDMPMT_USE_GLX
#undef PDMPMT_HAS_GLX
#define PDMPMT_HAS_GLX 1
#endif  // PDMPMT_USE_GLX

#ifndef PDMPMT_HAS_GLX
#define PDMPMT_HAS_GLX 0
#endif  // PDMPMT_HAS_GLX

// GoogleTest
#ifdef PDMPMT_USE_GTEST
#undef PDMPMT_HAS_GTEST
#define PDMPMT_HAS_GTEST 1
#endif  // PDMPMT_USE_GTEST

#ifndef PDMPMT_HAS_GTEST
#define PDMPMT_HAS_GTEST 0
#endif  // PDMPMT_HAS_GTEST

// GoogleMock
// note: some Linux platforms distribute GoogleTest and GoogleMock separately
// so we have a separate feature macro for GoogleMock
#ifdef PDMPMT_USE_GMOCK
#undef PDMPMT_HAS_GMOCK
#define PDMPMT_HAS_GMOCK 1
#endif  // PDMPMT_USE_GMOCK

#ifndef PDMPMT_HAS_GMOCK
#define PDMPMT_HAS_GMOCK 0
#endif  // PDMPMT_HAS_GMOCK

// Thrust
#ifdef PDMPMT_USE_THRUST
#undef PDMPMT_HAS_THRUST
#define PDMPMT_HAS_THRUST 1
#endif  // PDMPMT_USE_THRUST

#ifndef PDMPMT_HAS_THRUST
#define PDMPMT_HAS_THRUST 0
#endif  // PDMPTM_HAS_THRUST

// CUDA
// note: cuda.h is the driver API header while cuda_runtime_api.h is the
// runtime API header. usually cuda_runtime.h is used over cuda_runtime_api.h
#ifdef PDMPMT_USE_CUDA
#undef PDMPMT_HAS_CUDA
#define PDMPMT_HAS_CUDA 1
#endif  // PDMPMT_USE_CUDA

#ifndef PDMPMT_HAS_CUDA
#define PDMPMT_HAS_CUDA 0
#endif  // PDMPMT_HAS_CUDA

// MMX
// MSVC-specific logic
#if defined(_MSC_VER)
// no specific detection logic so assume if SSE is available that MMX is too.
// for x86 targets _M_IX86_FP is defined and we test if >= 1
#if defined(_M_IX86_FP)
#if _M_IX86_FP >= 1
#define PDMPMT_HAS_MMX 1
#endif  // _M_IX86_FP >= 1
// for x64, SSE2 is the default instruction set, so just test for _M_AMD64
#elif defined(_M_AMD64)
#define PDMPMT_HAS_MMX 1
#endif  // !defined(_M_IX86_FP) && !defined(_M_AMD64)
// GCC/Clang
#elif defined(__MMX__)
#define PDMPMT_HAS_MMX 1
#endif  // !defined(_MSC_VER) && !defined(__MMX__)

#ifndef PDMPMT_HAS_MMX
#define PDMPMT_HAS_MMX 0
#endif  // PDMPMT_HAS_MMX

// SSE
// MSVC-specific logic
#if defined(_MSC_VER)
// for x86 targets _M_IX86_FP is defined and if >= 1 SSE is available
#if defined(_M_IX86_FP)
#if _M_IX86_FP >= 1
#define PDMPMT_HAS_SSE 1
#endif  // _M_IX86_FP >= 1
// for x64, SSE2 is the default instruction set, so just test for _M_AMD64
#elif defined(_M_AMD64)
#define PDMPMT_HAS_SSE 1
#endif  // !defined(_M_IX86_FP) && !defined(_M_AMD64)
// GCC/Clang
#elif defined(__SSE__)
#define PDMPMT_HAS_SSE 1
#endif  // !defined(_MSC_VER) && !defined(__SSE__)

#ifndef PDMPMT_HAS_SSE
#define PDMPMT_HAS_SSE 0
#endif  // PDMPMT_HAS_SSE

// SSE2
// MSVC-specific logic
#if defined(_MSC_VER)
// for x86 targets _M_IX86_FP is defined and if >= 2 SSE2 is available
#if defined(_M_IX86_FP)
#if _M_IX86_FP >= 2
#define PDMPMT_HAS_SSE2 1
#endif  // _M_IX86_FP >= 2
// for x64, SSE2 is the default instruction set, so just test for _M_AMD64
#elif defined(_M_AMD64)
#define PDMPMT_HAS_SSE2 1
#endif  // !defined(_M_IX86_FP) && !defined(_M_AMD64)
// GCC/Clang
#elif defined(__SSE2__)
#define PDMPMT_HAS_SSE2 1
#endif  // !defined(_MSC_VER) && !defined(__SSE2__)

// SSE3
// MSVC has no explicit SSE3 detection so assume AVX implies it
#if defined(_MSC_VER)
#ifdef __AVX__
#define PDMPMT_HAS_SSE3 1
#endif  // __AVX__
// GCC/Clang
#elif defined(__SSE3__)
#define PDMPMT_HAS_SSE3 1
#endif  // !defined(_MSC_VER) && !defined(__SSE3__)

#ifndef PDMPMT_HAS_SSE3
#define PDMPMT_HAS_SSE3 0
#endif  // PDMPMT_HAS_SSE3

// SSE4.1
// MSVC has no explicit SSE4.1 detection so assume AVX implies it
#if defined(_MSC_VER)
#ifdef __AVX__
#define PDMPMT_HAS_SSE4_1 1
#endif  // __AVX__
// GCC/Clang
#elif defined(__SSE4_1__)
#define PDMPMT_HAS_SSE4_1 1
#endif  // !defined(_MSC_VER) && !defined(__SSE4_1__)

#ifndef PDMPMT_HAS_SSE4_1
#define PDMPMT_HAS_SSE4_1 0
#endif  // PDMPMT_HAS_SSE4_1

// SSE4.2
// MSVC has no explicit SSE4.2 detection so assume AVX implies it
#if defined(_MSC_VER)
#ifdef __AVX__
#define PDMPMT_HAS_SSE4_2 1
#endif  // __AVX__
// GCC/Clang
#elif defined(__SSE4_2__)
#define PDMPMT_HAS_SSE4_2 1
#endif  // !defined(_MSC_VER) && !defined(__SSE4_2__)

#ifndef PDMPMT_HAS_SSE4_2
#define PDMPMT_HAS_SSE4_2 0
#endif  // PDMPMT_HAS_SSE4_2

// AVX
#if defined(__AVX__)
#define PDMPMT_HAS_AVX 1
#else
#define PDMPMT_HAS_AVX 0
#endif  // !defined(__AVX__)

// AVX2
#if defined(__AVX2__)
#define PDMPMT_HAS_AVX2 1
#else
#define PDMPMT_HAS_AVX2 0
#endif  // !defined(__AVX2__)

// FMA
// MSVC has no explicit FMA detection so assume AVX2 implies it
#if defined(_MSC_VER)
#ifdef __AVX2__
#define PDMPMT_HAS_FMA 1
#endif  // __AVX2__
// GCC/Clang
#elif defined(__FMA__)
#define PDMPMT_HAS_FMA 1
#endif  // !defined(_MSC_VER) && !defined(__FMA__)

#ifndef PDMPMT_HAS_FMA
#define PDMPMT_HAS_FMA 0
#endif  // PDMPMT_HAS_FMA

// AVX-512BW
#if defined(__AVX512BW__)
#define PDMPMT_HAS_AVX512BW 1
#else
#define PDMPMT_HAS_AVX512BW 0
#endif  // !defined(__AVX512BW__)

// AVX-512CD
#if defined(__AVX512CD__)
#define PDMPMT_HAS_AVX512CD 1
#else
#define PDMPMT_HAS_AVX512CD 0
#endif  // !defined(__AVX512CD__)

// AVX-512DQ
#if defined(__AVX512DQ__)
#define PDMPMT_HAS_AVX512DQ 1
#else
#define PDMATRH_HAS_AVX512DQ 0
#endif  // !defined(__AVX512DQ__)

// AVX-512F
#if defined(__AVX512F__)
#define PDMPMT_HAS_AVX512F 1
#else
#define PDMPMT_HAS_AVX512F 0
#endif  // !defined(__AVX512F__)

// AVX-512VL
#if defined(__AVX512VL__)
#define PDMPMT_HAS_AVX512VL 1
#else
#define PDMPMT_HAS_AVX512VL 0
#endif  // !defined(__AVX512VL__)

#endif  // PDMPMT_FEATURES_H_
