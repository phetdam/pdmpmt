/**
 * @file common.h
 * @author Derek Huang
 * @brief C/C++ header for common helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_COMMON_H_
#define PDMPMT_COMMON_H_

// extern "C" scoping macros
#if defined(__cplusplus)
#define PDMPMT_EXTERN_C_BEGIN extern "C" {
#define PDMPMT_EXTERN_C_END }
#else
#define PDMPMT_EXTERN_C_BEGIN
#define PDMPMT_EXTERN_C_END
#endif  // !defined(__cplusplus)

// stringification macros
#define PDMPMT_STRINGIFY_I(x) #x
#define PDMPMT_STRINGIFY(x) PDMPMT_STRINGIFY_I(x)

// concatenation macros
#define PDMPMT_CONCAT_I(x, y) x ## y
#define PDMPMT_CONCAT(x, y) PDMPMT_CONCAT_I(x, y)

// allow C++-like use of inline in C code
#if defined(__cplusplus)
#define PDMPMT_INLINE inline
#else
#define PDMPMT_INLINE static inline
#endif  // !defined(__cplusplus)

// macro to indicate extern inline, i.e. never static inline
#define PDMPMT_EXTERN_INLINE inline

// correct __cplusplus for MSVC without compiling with /Zc:__cplusplus
#if defined(_MSVC_LANG)
#define PDMPMT_CPLUSPLUS _MSVC_LANG
#else
#define PDMPMT_CPLUSPLUS __cplusplus
#endif  // !defined(_MSC_VER)

// indicate if we are compiling under C++20 or above
#if PDMPMT_CPLUSPLUS >= 202002L
#define PDMPMT_HAS_CPP20 1
#else
#define PDMPMT_HAS_CPP20 0
#endif  // PDMPMT_CPLUSPLUS < 202002L

// indicate if we are compiling using exactly C++20
#if PDMPMT_CPLUSPLUS == 202002L
#define PDMPMT_CPP20_EXACT 1
#else
#define PDMPMT_CPP20_EXACT 0
#endif  // PDMPMT_CPLUSPLUS != 202002L

#endif  // PDMPMT_COMMON_H_
