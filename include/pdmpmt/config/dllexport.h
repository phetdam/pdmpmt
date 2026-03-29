/**
 * @file config/dllexport.h
 * @author Derek Huang
 * @brief C/C++ header for config library symbol export macros
 * @copyright MIT License
 */

#ifndef PDMPMT_CONFIG_DLLEXPORT_H_
#define PDMPMT_CONFIG_DLLEXPORT_H_

// note: pdmpmt_config is always built as a shared library and will be compiled
// with PDMPMT_BUILD_CONFIG defined to control symbol export

// Windows DLL export
#if defined(_WIN32)
#if defined(PDMPMT_BUILD_CONFIG)
#define PDMPMT_CONFIG_PUBLIC __declspec(dllexport)
#else
#define PDMPMT_CONFIG_PUBLIC __declspec(dllimport)
#endif  // !defined(PDMPMT_BUILD_CONFIG)
// GCC/Clang default symbol visibility
#elif defined(__GNUC__)
#define PDMPMT_CONFIG_PUBLIC __attribute__((visibility("default")))
// otherwise just leave empty
#else
#define PDMPMT_CONFIG_PUBLIC
#endif  // !defined(_WIN32)

#endif  // PDMPMT_CONFIG_DLLEXPORT_H_
