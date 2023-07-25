/**
 * @file dllexport.h
 * @author Derek Huang
 * @brief C/C++ header for DLL export macros
 * @copyright MIT License
 */

#ifndef PDMPMT_DLLEXPORT_H_
#define PDMPMT_DLLEXPORT_H_

// build as shared by default
#if defined(_WIN32) && !defined(PDMPMT_STATIC)
// all DLLs should be built with PDMPMT_BUILD_DLL defined
#if defined(PDMPMT_BUILD_DLL)
#define PDMPMT_PUBLIC __declspec(dllexport)
#else
#define PDMPMT_PUBLIC __declspec(dllimport)
#endif  // !defined(PDMPMT_BUILD_DLL)
#else
#define PDMPMT_PUBLIC
#endif  // !defined(_WIN32) || defined(PDMPMT_STATIC)

#endif  // PDMPMT_DLLEXPORT_H_
