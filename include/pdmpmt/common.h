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
#elif defined(__cplusplus)
#define PDMPMT_CPLUSPLUS __cplusplus
#endif  // !defined(_MSC_VER) && !defined(__cplusplus)

// C++11 noexcept macro
#ifdef PDMPMT_CPLUSPLUS
#if PDMPMT_CPLUSPLUS >= 201103L
#define PDMPMT_NOEXCEPT noexcept
#endif  // PDMPMT_CPLUSPLUS >= 201103L
#endif  // PDMPMT_CPLUSPLUS

#ifndef PDMPMT_NOEXCEPT
#define PDMPMT_NOEXCEPT
#endif  // PDMPMT_NOEXCEPT

#endif  // PDMPMT_COMMON_H_
