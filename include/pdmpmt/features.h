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

// TODO: use CXX macros and remove CC ones
#ifndef PDMPMT_HAS_CXX20
#define PDMPMT_HAS_CXX20 0
#endif  // PDMPMT_HAS_CXX20

// compatibility
// TODO: refactor uses of the compatibility macros
#define PDMPMT_HAS_CC20 PDMPMT_HAS_CXX20

#endif  // PDMPMT_FEATURES_H_
