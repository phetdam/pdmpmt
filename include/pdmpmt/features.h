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
#define PDMPMT_HAS_CC20 1
#endif  // PDMPMT_CPLUSPLUS >= 202002L
#endif  // PDMPMT_CPLUSPLUS

#ifndef PDMPMT_HAS_CC20
#define PDMPMT_HAS_CC20 0
#endif  // PDMPMT_HAS_CC20

#endif  // PDMPMT_FEATURES_H_
