/**
 * @file cuda_info.cc
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

namespace {

// program name and usage
const auto program_name = std::filesystem::path{__FILE__}.stem().string();
const std::string program_usage{
  "Usage: " + program_name + " [-h] [-e]\n"
  "\n"
  "Print info on the system's available CUDA devices.\n"
  "\n"
  "By default an abbreviated summary is printed. To include extra properties,\n"
  "specify the -e, --extended option for more properties.\n"
  "\n"
  "Options:\n"
  "  -h, --help        Print this usage\n"
  "  -e, --extended    Print additional device properties beyond the summary"
};

/**
 * Struct for program arguments.
 */
struct cli_options {
  bool print_usage = false;
  bool extended = false;
};

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
    // string view for convenience
    std::string_view arg{argv[i]};
    // help option (break early)
    if (arg == "-h" || arg == "--help") {
      opts.print_usage = true;
      return true;
    }
    // extended opton
    else if (arg == "-e" || arg == "--extended")
      opts.extended = true;
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

/**
 * Type that adds whitespace when streamed.
 */
class indentation {
public:
  /**
   * Ctor.
   *
   * @param size Indentation size
   */
  indentation(unsigned size) noexcept : size_{size} {}

  /**
   * Return the indentation size in characters.
   */
  auto size() const noexcept { return size_; }

private:
  unsigned size_;
};

/**
 * Insertion operator that causes whitespace indentation to be written.
 *
 * @param out Stream to write to
 * @param indent Indentation object
 */
auto& operator<<(std::ostream& out, const indentation& indent)
{
  for (decltype(indent.size()) i = 0; i < indent.size(); i++)
    out.put(' ');
  return out;
}

/**
 * Options type for printing CUDA device properties.
 */
class cuda_prop_print_options {
public:
  /**
   * Default ctor.
   */
  cuda_prop_print_options() noexcept : indent_{2u}, extended_{false} {}

  /**
   * Return the indentation level for each property.
   */
  auto indent() const noexcept { return indent_; }

  /**
   * Indicate if extended properties should also be printed.
   */
  auto extended() const noexcept { return extended_; }

  /**
   * Set the indentation level for each property.
   *
   * @param value New indentation value
   */
  auto& indent(indentation value) noexcept
  {
    indent_ = value;
    return *this;
  }

  /**
   * Set whether or not extended properties should be printed.
   *
   * @param value New extended print value
   */
  auto& extended(bool value) noexcept
  {
    extended_ = value;
    return *this;
  }

private:
  indentation indent_;
  bool extended_;
};

/**
 * Write some CUDA device properties.
 *
 * The first following properties are printed (in the given order):
 *
 * - name
 * - uuid
 * - totalGlobalMem (in G)
 * - sharedMemPerBlock (in K)
 * - regsPerBlock
 * - maxThreadPerBlock
 * - warpSize
 *
 * Additional properties are printed as determined by the display options.
 *
 * @param out Stream to write to
 * @param props CUDA device properties
 * @param opts Display options
 */
void write(
  std::ostream& out,
  const cudaDeviceProp& props,
  const cuda_prop_print_options& opts = {})
{
  // indentation
  auto indent = opts.indent();
  // print basic properties
  out <<
    indent << "Name: " << props.name << '\n' <<
    indent << "UUID: " << props.uuid << '\n' <<
    indent << "Memory: " << props.totalGlobalMem / (1 << 30) << "G\n" <<
    indent << "Memory per block: " << props.sharedMemPerBlock / (1 << 10) <<
      "K\n" <<
    indent << "Registers per block: " << props.regsPerBlock << '\n' <<
    indent << "Max threads per block: " << props.maxThreadsPerBlock << '\n' <<
    indent << "Warp size: " << props.warpSize << std::endl;
  // print extended properties if requested
  if (opts.extended()) {
    out <<
      indent << "Compute capability: " << props.major << '.' << props.minor <<
        '\n' <<
      indent << "Multiprocessor count: " << props.multiProcessorCount << '\n' <<
      indent << "Max thread block dims: (" <<
        props.maxThreadsDim[0] << ", " <<
        props.maxThreadsDim[1] << ", " <<
        props.maxThreadsDim[2] << ")" << std::endl;
  }
}

/**
 * Write some CUDA device properties to standard output.
 *
 * See the previous `write` overload for details.
 *
 * @param props CUDA device properties
 * @param opts Display options
 */
void write(const cudaDeviceProp& props, const cuda_prop_print_options& opts = {})
{
  write(std::cout, props, opts);
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
  // number of devices
  auto n_devices = pdmpmt::cuda_device_count();
  // display options
  auto disp_opts = cuda_prop_print_options{}
    .indent(2u)
    .extended(opts.extended);
  // for all available devices
  for (auto i = 0; i < n_devices; i++) {
    std::cout << "Device " << i << ":\n";
    write(pdmpmt::cuda_get_device_props(i), disp_opts);
  }
  return EXIT_SUCCESS;
}
