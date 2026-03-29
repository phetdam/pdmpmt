/**
 * @file config/paths.hh
 * @author Derek Huang
 * @brief C++ header for pdmpmt config library path helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_CONFIG_PATHS_HH_
#define PDMPMT_CONFIG_PATHS_HH_

#include <filesystem>

#include "pdmpmt/config/dllexport.h"

namespace pdmpmt {
namespace config {

/**
 * Return the path to the `pdmpmt_config` library shared object.
 *
 * This is an absolute path to where the config library was loaded from.
 */
PDMPMT_CONFIG_PUBLIC
std::filesystem::path library_path();

/**
 * Return the directory where the `pdmpmt_config` shared library resides in.
 *
 * THis is an absolute path to the directory containing the shared library.
 */
PDMPMT_CONFIG_PUBLIC
std::filesystem::path library_dir();

}  // namespace config
}  // namespace pdmpmt

#endif  // PDMPMT_CONFIG_PATHS_HH_
