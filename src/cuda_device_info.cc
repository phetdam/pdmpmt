/**
 * @file cuda_device_info.cc
 * @author Derek Huang
 * @brief C++ program to query CUDA device information
 * @copyright MIT License
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include "pdmpmt/cuda_runtime.hh"
#include "pdmpmt/warnings.h"

namespace {

// program name and usage
const auto program_name = std::filesystem::path{__FILE__}.stem().string();
const std::string program_usage{
  "Usage: " + program_name + " [-h]\n"
  "\n"
  "Print info on the system's available CUDA devices.\n"
  "\n"
  "Options:\n"
  "  -h, --help      Print this usage"
};

/**
 * Struct for program arguments.
 */
struct cli_options {
  bool print_usage = false;
};

//
// note:
//
// this will be removed later when we add more command-line options. MSVC
// reports that the i++ is unreachable because -h, --help always results in the
// loop being broken before increment and -h, --help is the only allowed option
// currently. C4702 will go away once more options are added.
//
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4702)  // C4702: unreachable code
/**
 * Parse incoming command-line arguments.
 *
 * @param opts Options struct to populate
 * @param argc Argument count from `main`
 * @param argv Argument vector from `main`
 * @returns `true` on success, `false` on error
 */
bool parse_args(cli_options& opts, int argc, char* argv[])
{
  // iterate through arguments
  for (int i = 1; i < argc; i++) {
PDMPMT_MSVC_WARNING_POP()
    // string view for convenience
    std::string_view arg{argv[i]};
    // help option (break early)
    if (arg == "-h" || arg == "--help") {
      opts.print_usage = true;
      return true;
    }
    // unknown
    else {
      std::cerr << "Error: Unknown argument " << arg << ". Try " <<
        program_name << " --help for usage" << std::endl;
      return false;
    }
  }
  // done
  return true;
}

}  // namespace

int main(int argc, char* argv[])
{
  // parse command-line arguments
  cli_options opts;
  if (!parse_args(opts, argc, argv))
    return EXIT_FAILURE;
  // print usage if requested
  if (opts.print_usage) {
    std::cout << program_usage << std::endl;
    return EXIT_SUCCESS;
  }
  // print some info
  // TODO: add actual logic to loop through and get device props
  std::cout <<
    "CUDA driver version: " << pdmpmt::cuda_driver_version() << '\n' <<
    "CUDA runtime version: " << pdmpmt::cuda_runtime_version() << '\n' <<
    "CUDA device count: " << pdmpmt::cuda_device_count() << std::endl;;
  return EXIT_SUCCESS;
}
