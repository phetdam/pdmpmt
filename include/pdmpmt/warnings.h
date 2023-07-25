/**
 * @file warnings.h
 * @author Derek Huang
 * @brief C/C++ header for warnings helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_WARNINGS_H_
#define PDMPMT_WARNINGS_H_

#include "pdmpmt/common.h"

// macros for using _Pragma without needing to explicitly quote
#define PDMPMT_PRAGMA_I(x) _Pragma(#x)
#define PDMPMT_PRAGMA(x) PDMPMT_PRAGMA_I(x)

// helper macros for disabling and re-enabling MSVC warnings
#if defined(_MSC_VER)
/**
 * Push warning state.
 */
#define PDMPMT_MSVC_WARNING_PUSH() __pragma(warning(push))

/**
 * Disable specified MSVC warnings.
 *
 * @param wnos Warning number(s), e.g. 4242
 */
#define PDMPMT_MSVC_WARNING_DISABLE(wnos) __pragma(warning(disable: wnos))

/**
 * Pop warning state.
 */
#define PDMPMT_MSVC_WARNING_POP() __pragma(warning(pop))
#else
#define PDMPMT_MSVC_WARNING_PUSH()
#define PDMPMT_MSVC_WARNING_DISABLE(wnos)
#define PDMPMT_MSVC_WARNING_POP()
#endif  // !defined(_MSC_VER)

// helper macros for disabling and re-enabling GCC/Clang warnings
#if defined(__GNUC__)
/**
 * Push warning state.
 */
#define PDMPMT_GNU_WARNING_PUSH() PDMPMT_PRAGMA(GCC diagnostic push)

/**
 * Disable specified GCC/Clang warning.
 *
 * @param wname GCC/Clang warning name without -W, e.g. self-move, narrowing
 */
#define PDMPMT_GNU_WARNING_DISABLE(wname) \
  PDMPMT_PRAGMA(GCC diagnostic ignored PDMPMT_STRINGIFY(PDMPMT_CONCAT(-W, wname)))

/**
 * Pop warning state.
 */
#define PDMPMT_GNU_WARNING_POP() PDMPMT_PRAGMA(GCC diagnostic pop)
#else
#define PDMPMT_GNU_WARNING_PUSH()
#define PDMPMT_GNU_WARNING_DISABLE(wname)
#define PDMPMT_GNU_WARNING_POP()
#endif  // !defined(__GNUC__)

// helper macros for disabling and re-enabling Clang warnings. these are useful
// when there are Clang-specific warnings that have no GCC equivalent.
#if defined(__clang__)
/**
 * Push warning state.
 */
#define PDMPMT_CLANG_WARNING_PUSH() PDMPMT_PRAGMA(clang diagnostic push)

/**
 * Disable specified Clang warning.
 *
 * @param wname Clang warning name without -W, e.g. unused-lambda-capture
 */
#define PDMPMT_CLANG_WARNING_DISABLE(wname) \
  PDMPMT_PRAGMA(clang diagnostic ignored PDMPMT_STRINGIFY(PDMPMT_CONCAT(-W, wname)))

/**
 * Pop warning state.
 */
#define PDMPMT_CLANG_WARNING_POP() PDMPMT_PRAGMA(clang diagnostic pop)
#else
#define PDMPMT_CLANG_WARNING_PUSH()
#define PDMPMT_CLANG_WARNING_DISABLE(wname)
#define PDMPMT_CLANG_WARNING_POP()
#endif  // @defined(__clang__)

#endif  // PDMPMT_WARNINGS_H_
