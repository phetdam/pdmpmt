/**
 * @file cpp/common.h
 * @author Derek Huang
 * @brief C++ header for common C++ helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_CPP_COMMON_H_
#define PDMPMT_CPP_COMMON_H_

// correct __cplusplus on Windows without compiling with /Zc:__cplusplus
#if defined(_MSC_VER)
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

#endif  // PDMPMT_CPP_COMMON_H_
