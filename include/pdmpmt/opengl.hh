/**
 * @file opengl.hh
 * @author Derek Huang
 * @brief C++ header for OpenGL helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_OPENGL_HH_
#define PDMPMT_OPENGL_HH_

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

// note: on Windows the path is actually gl/GL.h
#include <GL/gl.h>

#include "pdmpmt/features.h"

namespace pdmpmt {
namespace opengl {

/**
 * Return a string literal message for the given OpenGL error value.
 *
 * @param err OPenGL error value
 */
constexpr auto strerror(GLenum err) noexcept
{
  switch (err) {
#define PDMPMT_OPENGL_ERROR_CASE(x) case x: return #x;
  PDMPMT_OPENGL_ERROR_CASE(GL_NO_ERROR)
  PDMPMT_OPENGL_ERROR_CASE(GL_INVALID_ENUM)
  PDMPMT_OPENGL_ERROR_CASE(GL_INVALID_VALUE)
  PDMPMT_OPENGL_ERROR_CASE(GL_INVALID_OPERATION)
  PDMPMT_OPENGL_ERROR_CASE(GL_STACK_OVERFLOW)
  PDMPMT_OPENGL_ERROR_CASE(GL_STACK_UNDERFLOW)
  PDMPMT_OPENGL_ERROR_CASE(GL_OUT_OF_MEMORY)
#undef PDMPMT_OPENGL_ERROR_CASE
  default:
    return "(unknown)";
  }
}

/**
 * Exception representing a standaard OpenGL error from `glGetError().`
 */
class exception : std::runtime_error {
public:
  /**
   * Ctor.
   *
   * This uses the value returned by a `glGetError()` call, which is either the
   * last OpenGL error or `GL_NO_ERROR` if no error is on the error stack.
   */
  exception() : exception{glGetError()} {}

  /**
   * Ctor.
   *
   * This uses the value returned by a `glGetError()` call, which is either th
   * last OpenGL error or `GL_NO_ERROR` if no error is on the error stack.
   *
   * @param msg User-defined message prefix
   */
  exception(const std::string& msg) : exception{glGetError(), msg} {}

  /**
   * Ctor.
   *
   * @param err OpenGL error value, e.g. `GL_INVALID_ENUM`
   * @param msg User-defined message prefix
   */
  exception(GLenum err, const std::string& msg)
    : std::runtime_error{
        [err, &msg]
        {
          std::stringstream ss;
          ss << "OpenGL error: " << strerror(err) << ": " << msg;
          return ss.str();
        }()
      }
  {}

  /**
   * Ctor.
   *
   * @param err OpenGL error value, e.g. `GL_INVALID_ENUM`
   */
  exception(GLenum err)
    : std::runtime_error{"OpenGL error: " + std::string{strerror(err)}}
  {}

  /**
   * Return the OpenGL error value.
   */
  auto err() const noexcept { return err_; }

private:
  GLenum err_;
};

/**
 * Class managing OpenGL version info.
 *
 * @note Since the values returned by `glGetString` are allocated by the OpenGL
 *  runtime itself, the lifetime of the `string_view` objects is tied to the
 *  lifetime of the current OpenGL context itself.
 */
class version {
public:
  /**
   * Default ctor.
   *
   * This creates the version object from `glGetString(GL_VERSION)`.
   *
   * @todo The parsing logic should be split into a separate function.
   */
  version()
  {
    // get the version string
    auto verinfo = glGetString(GL_VERSION);
    // throw on error
    if (!verinfo)
      throw exception{"Unable to get OpenGL version string"};
    // otherwise, parse the version info
    // note: aliasing as char is allowed
    auto begin = reinterpret_cast<const char*>(verinfo);
    auto end = begin;
    // advance until first '.'
    while (*end && *end != '.')
      end++;
    // set major version
    major_ = static_cast<decltype(major_)>(std::stoi(std::string{begin, end}));
    // if we are at the end, technically this is an error, but we stop early.
    // in this case we just treat the minor version as zero
    if (!*end) {
      minor_ = 0u;
      return;
    }
    // otherwise, update end + begin, and continue
    begin = ++end;
    while (*end && *end != '.' && *end != ' ')
      end++;
    // set minor version
    minor_ = static_cast<decltype(minor_)>(std::stoi(std::string{begin, end}));
    // again, if at end, allow early stop
    if (!*end)
      return;
    // if end is at '.', then there is a build component we can extract
    if (*end == '.') {
      // update end + begin + continue to ' '
      begin = ++end;
      while (*end && *end != ' ')
        end++;
      // update build version info
#if PDMPMT_HAS_CXX20
      build_ = {begin, end};
#else
      build_ = {begin, static_cast<std::size_t>(end - begin)};
#endif  // !PDMPMT_HAS_CC20
    }
    // otherwise, proceed to end of string for vendor-specific info view
    begin = ++end;
    while (*end)
      end++;
#if PDMPMT_HAS_CXX20
    info_ = {begin, end};
#else
    info_ = {begin, static_cast<std::size_t>(end - begin)};
#endif  // !PDMPMT_HAS_CXX20
  }

  /**
   * Ctor.
   *
   * Construct from a major and minor version with no extra info.
   *
   * @param major OpenGL major version
   * @param minor OpenGL minor version
   */
  version(unsigned major, unsigned minor) noexcept
    : major_{major}, minor_{minor}
  {}

  /**
   * Return the OpenGL major version.
   */
  auto major() const noexcept { return major_; }

  /**
   * Return the OpenGL minor version.
   */
  auto minor() const noexcept { return minor_; }

  /**
   * Return a string view of the vendor-specific build string.
   *
   * This will point to the empty string if not available in the version info.
   */
  const auto& build() const noexcept { return build_; }

  /**
   * Return a string view of any additional vendor-specific info.
   *
   * This will point to the empty string if not available in the version info.
   */
  const auto& info() const noexcept { return info_; }

private:
  unsigned major_;
  unsigned minor_;
  std::string_view build_{""};
  std::string_view info_{""};
};

}  // namespace opengl
}  // namespace pdmpmt

#endif  // PDMPMT_OPENGL_HH_
