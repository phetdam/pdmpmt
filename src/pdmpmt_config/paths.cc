/**
 * @file paths.cc
 * @author Derek Huang
 * @brief C++ source for pdmpmt config library path helpers
 * @copyright MIT License
 */

#include "pdmpmt/config/paths.hh"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <errhandlingapi.h>
#include <libloaderapi.h>
#else
// note: need to define _GNU_SOURCE for dladdr()
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif  // _GNU_SOURCE
#include <dlfcn.h>
#endif  // !defined(_WIN32)

#include <filesystem>
#ifdef _WIN32
#include <system_error>  // for Win32 library_path() implementation
#endif  // _WIN32

// on Windows we capture the DLL module handle on load via DllMain()
#ifdef _WIN32
namespace {

HINSTANCE module_handle;

}  // namespace

// note: DllMain() need not be extern "C"
BOOL WINAPI DllMain(HINSTANCE handle, DWORD reason, LPVOID /*reserved*/)
{
  switch (reason) {
  // on DLL load record the module handle
  case DLL_PROCESS_ATTACH:
    module_handle = handle;
    return TRUE;
  // no-op for other cases
  default:
    return TRUE;
  }
}
#endif  // _WIN32

namespace pdmpmt {
namespace config {

std::filesystem::path library_path()
{
#if defined(_WIN32)
  // note: use wide version so std::filesystem::path doesn't have to convert
  WCHAR path[MAX_PATH];
  auto len = GetModuleFileNameW(module_handle, path, MAX_PATH);
  // general error or buffer too small
  if (!len || len == MAX_PATH)
    throw std::system_error{
      static_cast<int>(GetLastError()), std::system_category(),
      "GetModuleFileNameW() error"
    };
  // ok, return path
  return {path, path + len};
#else
  // note: no checking of return since library_path() is within the DSO itself
  Dl_info info;
  // note: this reinterpret_cast only compiles for some implementations, e.g.
  // particular POSIX implementations, due to dlsym() requirements
  dladdr(reinterpret_cast<const void*>(&library_path), &info);
  return info.dli_fname;
#endif  // !defined(_WIN32)
}

std::filesystem::path library_dir()
{
  return library_path().parent_path();
}

}  // namespace config
}  // namespace pdmpmt
