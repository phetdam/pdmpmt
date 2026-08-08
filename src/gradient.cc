/**
 * @file gradient.cc
 * @author Derek Huang
 * @brief C++ program generating a gradient between two RGBA colors
 * @copyright MIT License
 */

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pdmpmt/image.hh"

namespace {

// image width and height defaults
constexpr auto def_width = 800u;
constexpr auto def_height = 600u;
// program name and usage
const auto progname = std::filesystem::path{__FILE__}.stem().string();
const auto program_usage = "Usage: " + progname +
    " [-h] [-c COLOR1,COLOR2] [-o OUTPUT]\n"
  "\n"
  "Generates a raster image representing a gradient between two colors.\n"
  "\n"
  "Given two user-specified RGB[A] colors, the program generates a raster\n"
  "image representing the gradient between the two colors. If an output file\n"
  "is specified, then this image is written to that path in PPM format.\n"
  "\n"
  "For example, we could display the gradient between the green 2ca02c and the\n"
  "lavender 9467bd, both colors from the D3 Category10 palette:\n"
  "\n"
  "  " + progname + " -c 2ca02c,9467bd -o 2ca02c_9467bd.ppm\n"
  "\n"
  "Another example is the green 264d59 and warm yellow f9e07f gradient:\n"
  "\n"
  "  " + progname + " -c 264d59,f9e07f -o 264d59_f9e07f.ppm\n"
  "\n"
  "Yet another example is the icy blue a5cad2 and coral ff7b889 gradient:\n"
  "\n"
  "  " + progname + " -c a5cad2,ff7b89 -o a5cad2_ff7b89.ppm\n"
  "\n"
  "Options:\n"
  "  -h, --help           Print this usage\n"
  "\n"
  "  -c COLOR1,COLOR2, --colors COLOR1,COLOR2\n"
  "                       Comma-separated RGB[A] hex values specifying the two\n"
  "                       colors to form a gradient with. Each value must be\n"
  "                       a valid hex value, e.g. ff34df or ED13FD45, i.e. 6\n"
  "                       or 8 hex characters. If an alpha value is not given\n"
  "                       it will default to FF for no transparency.\n"
  "\n"
  "  -d DIMS, --dims DIMS\n"
  "                       Comma-separated values specifying the image width\n"
  "                       and height in pixels, default "
  "  -o OUTPUT, --output OUTPUT\n"
  "                       Path to write the resulting gradient image to. The\n"
  "                       image file will be in the PPM binary format.";

/**
 * Command-line options structure.
 *
 * @param help `true` to print program usage
 * @param color_1 First pixel color
 * @param color_2 Second pixel color
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param output PPM image output path
 */
struct cli_options {
  bool help = false;
  pdmpmt::pixel color_1 = {};
  pdmpmt::pixel color_2 = {};
  unsigned width = def_width;
  unsigned height = def_height;
  std::filesystem::path output = {};
};

/**
 * Advance to the next command-line argument position if possible.
 *
 * If the argument position points to the last command-line argument, a message
 * is emitted and `false` is returned. On success, `true` is returned.
 *
 * @param i Argument index to increment
 * @param argc Argument count from `main()`
 * @param msg Message to emit on error
 */
bool advance(int& i, int argc, std::string_view msg)
{
  // still have another command-line argument
  if (++i < argc)
    return true;
  // otherwise error
  std::cerr << "Error: " << msg << std::endl;
  return false;
}

/**
 * Split a string view into two string views at the comma.
 *
 * If there is more than 1 comma this is not considered an error.
 *
 * @param str String view to split with `','`
 */
auto split_comma(std::string_view str)
{
  // view + iterators
  auto begin = str.begin();
  auto mid = begin;
  auto end = str.end();
  // find first ','
  while (mid != end && *mid != ',')
    mid++;
  // no comma in string
  if (mid == end)
    throw std::runtime_error{std::string{str} + " missing ',' delimiter"};
  // split at ','
  return std::pair{
    std::string_view{begin, static_cast<std::size_t>(mid - begin)},
    std::string_view{mid + 1, static_cast<std::size_t>(end - mid - 1)}
  };
}

/**
 * Parse RGB[A] values into a `pixel` from the string view.
 *
 * If the alpha value is omitted `0xFF` is used by default.
 *
 * @param px Pixel to initialize
 * @param str String view to parse
 */
void parse(pdmpmt::pixel& px, std::string_view str)
{
  // view must be 6 or 8 characters long
  if (str.size() != 6 && str.size() != 8)
    throw std::runtime_error{std::string{str} + " must be 6 or 8 characters long"};
  // all values must be hex values
  for (auto c : str)
    if ((c < '0' || c > '9') && (c < 'a' || c > 'f') && (c < 'A' || c > 'F'))
      throw std::runtime_error{std::string{str} + " contains non-hex character"};
  // helper to convert hex char into unsigned char values
  auto x2b = [](char c)
  {
    // lowercase
    if (c >= 'a')
      return static_cast<unsigned char>(c - 'a' + 0xA);
    // uppercase
    if (c >= 'A')
      return static_cast<unsigned char>(c - 'A' + 0xA);
    // numeric
    return static_cast<unsigned char>(c - '0');
  };
  // init pixel
  px.r() = (x2b(str[0]) << 4) + x2b(str[1]);
  px.g() = (x2b(str[2]) << 4) + x2b(str[3]);
  px.b() = (x2b(str[4]) << 4) + x2b(str[5]);
  // note: alpha chars are optional
  if (str.size() == 8)
    px.a() = (x2b(str[6]) << 4) + x2b(str[7]);
}

/**
 * Parse an unsigned value from the string view.
 *
 * @param out Value to initialize
 * @param str String view to parse
 * @param maxv Maximum value accepted
 */
void parse(
  unsigned& out,
  std::string_view str,
  unsigned maxv = (std::numeric_limits<unsigned>::max)())
{
  // empty
  if (str.empty())
    throw std::runtime_error{"string view is empty"};
  // temporary to avoid partial effects
  auto v = 0u;
  // iterate
  for (auto i = 0u; i < str.size(); i++) {
    // current char
    auto c = str[i];
    // not numeric
    if (c < '0' || c > '9')
      throw std::runtime_error{
        std::string{str} + " does not represent an unsigned value"
      };
    // if would overflow or exceed limit, throw
    if (v * 10u < v)
      throw std::runtime_error{std::string{str} + " overflows an unsigned int"};
    if (v * 10u > maxv)
      throw std::runtime_error{
        std::string{str} + " exceeds limit " + std::to_string(maxv)
      };
    // multiply by 10 + add
    v *= 10u;
    v += static_cast<unsigned>(c - '0');
  }
  // set value
  out = v;
}

/**
 * Parse incoming command-line arguments.
 *
 * @param opts Command-line options to fill
 * @param argc Argument count from `main()`
 * @param argv Argument vector from `main()`
 * @returns `true` on success, `false` on error
 */
bool parse_args(cli_options& opts, int argc, char** argv)
{
  // iterate
  for (int i = 1; i < argc; i++) {
    std::string_view arg{argv[i]};
    // -h, --help
    if (arg == "-h" || arg == "--help") {
      opts.help = true;
      return true;
    }
    // -c, --colors
    else if (arg == "-c" || arg == "--colors") {
      if (!advance(i, argc, "-c, --colors missing required color values"))
        return false;
      // parse colors
      try {
        auto [view_1, view_2] = split_comma(argv[i]);
        parse(opts.color_1, view_1);
        parse(opts.color_2, view_2);
      }
      catch (const std::exception& exc) {
        std::cerr << "Error: -c, --colors: " << exc.what() << std::endl;
        return false;
      }
    }
    // -d, --dims
    else if (arg == "-d" || arg == "--dims") {
      if (!advance(i, argc, "-d, --dims missing required dimension values"))
        return false;
      // parse dimensions
      try {
        auto [view_1, view_2] = split_comma(argv[i]);
        // note: limits are a bit arbitrary
        parse(opts.width, view_1, 65536u);
        parse(opts.height, view_2, 65536u);
      }
      catch (const std::exception& exc) {
        std::cerr << "Error: -d, --dims: " << exc.what() << std::endl;
        return false;
      }
    }
    // -o, --output
    else if (arg == "-o" || arg == "--output") {
      if (!advance(i, argc, "-o, --output missing required path"))
        return false;
      opts.output = argv[i];
    }
    // unknown option
    else {
      std::cerr << "Error: Unknown option " << arg << std::endl;
      return false;
    }
  }
  // done
  return true;
}

/**
 * Return evenly sampled values from the interval.
 *
 * The endpoints of the interval are included in the returned values.
 *
 * @param x Left endpoint
 * @param y Right endpoint
 * @param size Number of values to sample > 1
 */
auto sample_interval(float x, float y, std::size_t size)
{
  // sampled values
  std::vector<float> values(size);
  // compute v(i) = x + i * (y - x) / (size - 1) + return
  for (auto i = 0u; i < size; i++)
    values[i] = x + i * (y - x) / (size - 1u);
  return values;
}

/**
 * Create linear gradient image.
 *
 * This linearly interpolates pixel intensities between the two colors.
 *
 * @param c1 Starting color
 * @param c2 Ending color
 * @param w Image width in pixels
 * @param h Image height in pixels
 */
auto linear_gradient(pdmpmt::pixel c1, pdmpmt::pixel c2, unsigned w, unsigned h)
{
  pdmpmt::image img{w, h};
  // for each image row
  for (auto i = 0u; i < h; i++) {
    // sample w points between c1, c2 colors + alpha
    auto rs = sample_interval(c1.r(), c2.r(), w);
    auto gs = sample_interval(c1.g(), c2.g(), w);
    auto bs = sample_interval(c1.b(), c2.b(), w);
    auto as = sample_interval(c1.a(), c2.a(), w);
    // update pixel values in row with appropriate truncation
    for (auto j = 0u; j < w; j++)
      img(i, j) = {
        static_cast<pdmpmt::pixel::byte>(rs[j]),
        static_cast<pdmpmt::pixel::byte>(gs[j]),
        static_cast<pdmpmt::pixel::byte>(bs[j]),
        static_cast<pdmpmt::pixel::byte>(as[j])
      };
  }
  return img;
}

}  // namespace

int main(int argc, char** argv)
{
  // parse command-line options
  cli_options opts;
  if (!parse_args(opts, argc, argv))
    return EXIT_FAILURE;
  // print usage
  if (opts.help) {
    std::cout << program_usage << std::endl;
    return EXIT_FAILURE;
  }
  // print info
  std::cout <<
    opts.width << " x " << opts.height << " linear gradient " <<
    opts.color_1 << " to " << opts.color_2 << std::endl;
  // create image
  auto img = linear_gradient(opts.color_1, opts.color_2, opts.width, opts.height);
  // write to file if appropriate + exit
  if (!opts.output.empty()) {
    std::cout << "writing image to " << opts.output << "... " << std::flush;
    std::ofstream fs{opts.output, fs.binary};
    fs << pdmpmt::ppm << img;
    std::cout << "done" << std::endl;
  }
  return EXIT_SUCCESS;
}
