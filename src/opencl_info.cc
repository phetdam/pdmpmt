/**
 * @file opencl_info.cc
 * @author Derek Huang
 * @brief C++ program for querying OpenCL platform/device info
 * @copyright MIT License
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string_view>
#include <utility>

#include "pdmpmt/opencl.hh"
#include "pdmpmt/type_traits.hh"

namespace {

// program name and usage
const auto progname = std::filesystem::path{__FILE__}.stem().string();
const auto program_usage = "Usage: " + progname + " [-h] [--no-ext]\n"
  "\n"
  "Print information on available OpenCL platforms and devices.\n"
  "\n"
  "This inculdes platform and device names, supported OpenCL versions, the\n"
  "global memory sizes for each device, supported OpenCL extensions, etc.\n"
  "\n"
  "Options:\n"
  "  -h, --help         Print this usage\n"
  "  --no-ext           Do not print platform and device OpenCL extensions.\n"
  "                     OpenCL extensions are printed by default, but if there\n"
  "                     are multiple platforms and devices, printing all their\n"
  "                     OpenCL extensions can take up a lot of screen space.";

/**
 * Struct for the command-line options.
 *
 * @param print_usage `true` to print the promgra usage
 * @param print_ext `true` to show platform/device OpenGL extensions
 */
struct cli_options {
  bool print_usage = false;
  bool print_ext = true;
};

/**
 * Parse incoming command-line options.
 *
 * @param opts Command-line options to set
 * @param argc Argument count from` main()`
 * @param argv Argument vector from `main()`
 * @returns `true` if parsing succeeded, `false` otherwise
 */
bool parse_args(cli_options& opts, int argc, char** argv)
{
  // simple loop through arguments
  for (int i = 1; i < argc; i++) {
    // arg string view for convenience
    std::string_view arg{argv[i]};
    // print usage (break early)
    if (arg == "-h" || arg == "--help") {
      opts.print_usage = true;
      return true;
    }
    // don't show platform/device extensions
    else if (arg == "--no-ext")
      opts.print_ext = false;
    // unknown
    else {
      std::cerr << "Error: Unknown option " << arg << ". Try " << progname <<
        " --help for usage" << std::endl;
      return false;
    }
  }
  // done
  return true;
}

/**
 * Indentation helper.
 */
class indentation_type {
public:
  /**
   * Ctor.
   *
   * This uses the default indentation size of 2 spaces per level.
   *
   * @param levels Number of indentation levels
   */
  constexpr indentation_type(unsigned levels) noexcept
    : indentation_type(2u, levels)
  {}

   /**
    * Ctor.
    *
    * @param size Number of characters in one indentation level
    * @param levels NUmber of indentation levels
    */
  constexpr indentation_type(unsigned size, unsigned levels) noexcept
    : size_{size}, levels_{levels}
  {}

  // getters
  constexpr auto size() const noexcept { return size_; }
  constexpr auto levels() const noexcept { return levels_; }

private:
  unsigned size_;   // number of characters in one indentation level
  unsigned levels_;  // number of indentation levels
};

/**
 * Insert the correct number of spaces given the indentation object.
 *
 * @param out Output stream
 * @param ind Indentation object
 */
auto& operator<<(std::ostream& out, indentation_type ind)
{
  auto n = ind.size() * ind.levels();
  for (decltype(n) i = 0; i < n; i++)
    out.put(' ');
  return out;
}

/**
 * Customization point object that returns an indentation object.
 */
struct indentation_factory {
  constexpr indentation_type operator()(unsigned levels) const noexcept
  {
    return levels;
  }
};

// global for generating indentation objects
constexpr indentation_factory indent;

/**
 * Helper to format a range like a tuple.
 *
 * The returned string will have values formatted like `(v1, ... vn)`.
 *
 * @todo The range value type must also be streamable.
 *
 * @tparam R Rnage-like with *LegacyForwardIterator* iterators
 *
 * @param rng Range to format
 */
template <typename R>
auto format(R&& rng, pdmpmt::constraint_t<pdmpmt::is_range_v<R>> = 0)
{
  std::stringstream ss;
  // iterate
  ss << '(';
  for (auto it = std::begin(rng); it != std::end(rng); ++it) {
    if (!(it == std::begin(rng)))
      ss << ", ";
    ss << *it;
  }
  ss << ')';
  // convert to string. string is moved in C++20
  return std::move(ss).str();
}

}  // namespace

int main(int argc, char** argv)
{
  using pdmpmt::opencl::platform_info;
  using pdmpmt::opencl::device_info;
  // parse command-line arguments
  cli_options opts;
  if (!parse_args(opts, argc, argv))
    return EXIT_FAILURE;
  // print usage if specified
  if (opts.print_usage) {
    std::cout << program_usage << std::endl;
    return EXIT_SUCCESS;
  }
  // get the OpenCL platform IDs
  auto plat_ids = pdmpmt::opencl::platform_ids();
  // get info for each platform
  std::cout << "OpenCL platforms:" << std::endl;
  for (auto i = 0u; i < plat_ids.size(); i++) {
    // platform ID
    auto plat = plat_ids[i];
    std::cout << indent(1) << "Platform " << i << ":\n";
    // print platform name + supported OpenCL version
    std::cout <<
      indent(2) << "Name: " <<
        platform_info<CL_PLATFORM_NAME>(plat) << "\n" <<
      indent(2) << "Version: " <<
        platform_info<CL_PLATFORM_VERSION>(plat) << "\n";
    // print extensions if not disabled
    if (opts.print_ext)
      std::cout <<
        indent(2) << "Extensions: " <<
          platform_info<CL_PLATFORM_EXTENSIONS>(plat) << "\n";
    // get device IDs for the platform
    auto dev_ids = pdmpmt::opencl::device_ids(plat);
    std::cout << indent(2) << "OpenCL devices:" << std::endl;
    // TODO: print info for each platform device
    for (auto j = 0u; j < dev_ids.size(); j++) {
      // device ID
      auto dev = dev_ids[j];
      std::cout << indent(3) << "Device " << j << ":\n";
      // print device name + supported OpenCL version + other info
      std::cout <<
        indent(4) << "Name: " <<
          device_info<CL_DEVICE_NAME>(dev) << "\n" <<
        indent(4) << "Version: " <<
          device_info<CL_DEVICE_VERSION>(dev) << "\n" <<
        indent(4) << "Global memory: " <<
          device_info<CL_DEVICE_GLOBAL_MEM_SIZE>(dev) / (1 << 30) << "G\n" <<
        indent(4) << "Max compute units: " <<
          device_info<CL_DEVICE_MAX_COMPUTE_UNITS>(dev) << "\n" <<
        indent(4) << "Max work group size: " <<
          device_info<CL_DEVICE_MAX_WORK_GROUP_SIZE>(dev) << "\n" <<
        indent(4) << "Max work item sizes: " <<
          format(device_info<CL_DEVICE_MAX_WORK_ITEM_SIZES>(dev)) << "\n";
      // print extensions if not disabled
      if (opts.print_ext)
        std::cout <<
          indent(4) << "Extensions: " <<
            device_info<CL_DEVICE_EXTENSIONS>(dev) << "\n";
      // flush stdout
      std::cout << std::flush;
    }  // CL_DEVICE_EXTENSIONS
  }
  return EXIT_SUCCESS;
}
