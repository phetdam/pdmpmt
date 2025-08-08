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

// compatibility
// TODO: refactor uses of the compatibility macros
#define PDMPMT_HAS_CC20 PDMPMT_HAS_CXX20

// X11
#ifdef __has_include
#if __has_include(<X11/Xlib.h>)
#define PDMPMT_HAS_X11 1
#endif  // __has_include(<X11/Xlib.h>)
#endif  // __has_include

#ifndef PDMPMT_HAS_X11
#define PDMPMT_HAS_X11 0
#endif  // PDMPMT_HAS_X11

// GLX
// note: usually GL/glx.h includes X11/Xlib.h itself
#ifdef __has_include
#if __has_include(<GL/glx.h>)
#define PDMPMT_HAS_GLX 1
#endif  // __has_include(<GL/glx.h>)
#endif  // __has_include

#ifndef PDMPMT_HAS_GLX
#define PDMPMT_HAS_GLX 0
#endif  // PDMPMT_HAS_GLX

#endif  // PDMPMT_FEATURES_H_
