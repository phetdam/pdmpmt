/**
 * @file paths_test.cc
 * @author Derek Huang
 * @brief `config/paths.hh` unit tests
 * @copyright MIT License
 */

#include "pdmpmt/config/paths.hh"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#endif  // !defined(_WIN32)

#include <gtest/gtest.h>

// dummy define for IntelliSense so VS Code/Visual Studio don't emit errors
#ifdef __INTELLISENSE__
#define PDMPMT_CONFIG_BUILD_PATH "/nonsense/path"
#endif  // __INTELLISENSE__

// PDMPMT_CONFIG_BUILD_PATH must be a string literal that gives the location of
// the pdmpmt_config library in the build tree. in CMake we obtain this via the
// $<TARGET_FILE:pdmpmt_config> generator expression
#ifndef PDMPMT_CONFIG_BUILD_PATH
#error "PDMPMT_CONFIG_BUILD_PATH must be defined"
#endif  // PDMPMT_CONFIG_BUILD_PATH

namespace {

/**
 * Test fixture for path helper tests.
 */
class ConfigPathsTest : public ::testing::Test {
protected:
  // path to pdmpmt_config in the build tree
  static inline std::filesystem::path library_path{PDMPMT_CONFIG_BUILD_PATH};
};

/**
 * Test that `library_path()` works as expected.
 */
TEST_F(ConfigPathsTest, LibraryPathTest)
{
  EXPECT_EQ(library_path, pdmpmt::config::library_path());
}

/**
 * Test that `library_dir()` works as expected.
 */
TEST_F(ConfigPathsTest, LibraryDirTest)
{
  EXPECT_EQ(library_path.parent_path(), pdmpmt::config::library_dir());
}

}  // namespace
