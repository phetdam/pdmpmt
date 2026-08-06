/**
 * @file image_ppm_test.cc
 * @author Derek Huang
 * @brief C++ program unit testing the `image` class PPM read/write
 * @copyright MIT License
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

#include "pdmpmt/format.hh"
#include "pdmpmt/image.hh"

namespace {

// program name + usage
const auto progname = std::filesystem::path{__FILE__}.stem().string();
const std::string program_usage = "Usage: " + progname +
  " [-h] [-l] [-t TEST_NAME]\n"
  "\n"
  "Test PPM file read/write using the pdmpmt::image class.\n"
  "\n"
  "This program is a unit testing program for PPM read/write functionality,\n"
  "containing several string literals representing small but valid PPM images,\n"
  "possibly with comments, that the pdmpmt::image is expected to be able to\n"
  "initialize from, write to a stream, and then round-trip without error.\n"
  "\n"
  "Options:\n"
  "  -h, --help             Print this usage\n"
  "  -l, --list-tests       List the tests in this program\n"
  "\n"
  "  -t TEST_NAME, --test TEST_NAME\n"
  "                         Run the specified test case. If not specified,\n"
  "                         all the test cases are run sequentially.";

/**
 * Class representing a PPM image test case.
 *
 * This splits the PPM into two string literals representing the header and
 * image raster sections. Having two separate sections makes it easy to check
 * the RGB values read by the `image` class when initializing from a stream.
 *
 * The maximum color value is required to be 255 (0xFF). See
 * https://netpbm.sourceforge.net/doc/ppm.html for the PPM format.
 */
class ppm_image_test {
public:
  /**
   * Ctor.
   *
   * @param name Test name
   * @param header PPM image file header
   * @param image PPM image file raster
   */
  constexpr ppm_image_test(
    std::string_view name,
    std::string_view header,
    std::string_view image) noexcept
    : name_{name}, header_{header}, raster_{image}
  {}

  /**
   * Return the test name.
   */
  constexpr auto name() const noexcept { return name_; }

  /**
   * Return the PPM header.
   */
  constexpr auto header() const noexcept { return header_; }

  /**
   * Return the PPM image raster.
   */
  constexpr auto raster() const noexcept { return raster_; }

private:
  std::string_view name_;    // test name
  std::string_view header_;  // magic, width, height, max color, comments
  std::string_view raster_;  // RGB 3-byte triplets
};

// test images
constexpr ppm_image_test image_tests[] = {
  // 12 x 12 red square
  {
    "red_12x12",
    "P6\n"
    "12 12\n"
    "255\n",
    // note: each line contains 6 0xFF0000 triples
    {
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00",
      3u * 12u * 12u
    }
  },
  // 6 x 6 blue square w/ comments
  {
    "blue_6x6",
    "P6\n"
    "# mini_blue.ppm\n"
    "6    # width\n"
    "6    # height\n"
    "255  # max color\n"
    "# raster\n",
    // note: each line contains 4 0x0000FF triples
    {
"\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF"
"\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF"
"\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF"
"\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF"
"\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF"
"\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF",
      3u * 6u * 6u
    }
  },
  // 12 x 12 half red half green square w/ CRLF
  {
    "red_green_12x12",
    "P6\r\n"
    "12 12\r\n"
    "255\r\n",
    // note: each line contains 6 0xFF0000 or 0x0000FF triples
    {
// red
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
"\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00"
// green
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00"
"\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00",
      3u * 12u * 12u
    }
  }
};

/**
 * Command-line options struct.
 *
 * @param help `true` to print program usage
 * @param list_tests `true` to list test names
 * @param test_name Test name to run
 */
struct cli_options {
  bool help = false;
  bool list_tests = false;
  std::string_view test_name;
};

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
    // -l, --list-tests
    else if (arg == "-l" || arg == "--list-tests")
      opts.list_tests = true;
    // -t, --test-name
    else if (arg == "-t" || arg == "--test-name") {
      // no argument
      if (++i >= argc) {
        std::cerr << "Error: -t, --test-name missing required test name" <<
          std::endl;
        return false;
      }
      // read test name
      opts.test_name = argv[i];
    }
    // unknown
    else {
      std::cerr << "Error: Unknown option " << arg << ". Try " << progname <<
        " --help for usage" << std::endl;
      return false;
    }
  }
  // if test name provided ensure it exists
  if (!opts.test_name.empty()) {
    for (const auto& test : image_tests)
      if (test.name() == opts.test_name)
        return true;
    // test name not found
    std::cerr << "Error: -t, --test-name: Unknown test name " <<
      opts.test_name << std::endl;
    return false;
  }
  // done
  return true;
}

/**
 * Helper to print the test failure message.
 *
 * @param test PPM image test
 */
auto& fail_message(const ppm_image_test& test)
{
  return std::cout << "[  FAILED  ] " << test.name() << "\n";
}

/**
 * Check an `image` against the PPM image test raster.
 *
 * Information on mismatched pixels will be printed to `stdout`.
 *
 * @param test PPM image test
 * @param img Image to check
 * @returns `true` on equality, `false` on mismatch
 */
bool check_image(const ppm_image_test& test, const pdmpmt::image& img)
{
  // reference to test image raster
  auto rst = test.raster();
  // check bytes + count mismatched pixel channels
  auto bad_pxv = 0u;
  for (auto i = 0u; i < img.size(); i++) {
    // note: can extend lifetime of non-copyable pixel::view with const ref
    const auto& px = img(i);
    // get PPM bytes
    pdmpmt::pixel::byte ppm[3];
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4365)
    ppm[0] = rst[3u * i];
    ppm[1] = rst[3u * i + 1];
    ppm[2] = rst[3u * i + 2];
PDMPMT_MSVC_WARNING_POP()
    // if any byte is mismatched
    if (px != pdmpmt::pixel{ppm[0], ppm[1], ppm[2]}) {
      // print banner on first failure
      if (!bad_pxv)
        fail_message(test);
      // get pixel bytes
      pdmpmt::pixel::byte pxb[3];
      pxb[0] = px.r();
      pxb[1] = px.g();
      pxb[2] = px.b();
      // report + increment
      std::cout << "  img(" << i << ") != ppm[" << i << "] (" <<
        pdmpmt::hex_u << pxb << " != " << pdmpmt::hex_u << ppm << ")\n";
      bad_pxv++;
    }
    // ensure flush + error if bad_pxv
    std::cout << std::flush;
    if (bad_pxv) {
      std::cout << "  mismatched pixels: " << bad_pxv << std::endl;
      return false;
    }
  }
  // otherwise bytes match
  return true;
}

/**
 * Test that the `image` class can round-trip a PPM image correctly.
 *
 * Below are the test steps:
 *
 * 1. Write `ppm_image_test` header and raster into binary `std::stringstream`
 * 2. Initialize `image` from `std::stringstream`
 * 3. Compare `image` buffer against raster
 * 4. If matching, reset stream and write image contents to stream
 * 5. Initialize image from stream
 * 6. Compare new `image` buffer against raster
 *
 * @param test PPM image test
 * @returns `true` on success, `false` on failure
 */
bool test_ppm(const ppm_image_test& test)
{
  std::cout << "[ RUN      ] " << test.name() << std::endl;
  // stream for read/write
  std::stringstream ss{ss.in | ss.out | ss.binary};
  ss << test.header() << test.raster() << std::flush;
  // create image from PPM
  pdmpmt::image img{ss, pdmpmt::ppm};
  // check image size
  // note: raster is number of pixels * 3
  if (3u * img.size() != test.raster().size()) {
    fail_message(test) <<
      "image pixel count " << img.size() << " != raster pixel count " <<
      (test.raster().size() / 3u) << std::endl;
    return false;
  }
  // check image bytes
  if (!check_image(test, img))
    return false;
  // matching, so reset stream, and write PPM image
  ss = std::stringstream{ss.in | ss.out | ss.binary};
  ss << pdmpmt::ppm << img;
  // seek to beginning + re-initialize from PPM
  ss.seekg(0);
  ss >> pdmpmt::ppm >> img;
  // check image bytes
  if (!check_image(test, img))
    return false;
  // print success
  std::cout << "[       OK ] " << test.name() << std::endl;
  return true;
}

}  // namespace

int main(int argc, char** argv)
{
  // parse command-line arguments
  cli_options opts;
  if (!parse_args(opts, argc, argv))
    return EXIT_FAILURE;
  // print usage
  if (opts.help) {
    std::cout << program_usage << std::endl;
    return EXIT_SUCCESS;
  }
  // print test list
  if (opts.list_tests) {
    for (const auto& test : image_tests)
      std::cout << test.name() << std::endl;
    return EXIT_SUCCESS;
  }
  // run a single test
  if (!opts.test_name.empty()) {
    // look for test with matching name
    std::cout << "Running 1 PPM test...\n" << std::endl;
    for (const auto& test : image_tests)
      if (opts.test_name == test.name())
        return (test_ppm(test)) ? EXIT_SUCCESS : EXIT_FAILURE;
    // somehow couldn't find the test
    std::cout << "Error: Unexpectedly failed to find test " << opts.test_name <<
      std::endl;
    return EXIT_FAILURE;
  }
  // running all tests
  auto n_tests = std::size(image_tests);
  std::cout << "Running " << n_tests << " PPM tests...\n" << std::endl;
  // count failures
  auto failed = 0u;
  for (const auto& test : image_tests)
    failed += !test_ppm(test);
  // get rounded pass percentage
  auto pass_rate = 10000 * (1 - 1. * failed / n_tests) / 100.;
  // print summary
  std::cout << "\n" <<
    pass_rate << "% tests passed, " << failed << " tests failed out of " <<
    n_tests << std::endl;
  // exit
  return (!failed) ? EXIT_SUCCESS : EXIT_FAILURE;
}
