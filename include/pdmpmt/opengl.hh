/**
 * @file opengl.hh
 * @author Derek Huang
 * @brief C++ header for OpenGL helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_OPENGL_HH_
#define PDMPMT_OPENGL_HH_

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wingdi.h>
#endif  // _WIN32

#include <cstddef>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

// note: on Windows the path is actually gl/GL.h
#include <GL/gl.h>
// include GLX if available
#if !defined(_WIN32) && defined(__has_include)
#if __has_include(<GL/glx.h>)
#include <GL/glx.h>
#define PDMPMT_HAS_GLX 1
#endif  // __has_include(<GL/glx.h>)
#endif  // !defined(_WIN32) && defined(__has_include)

#ifndef PDMPMT_HAS_GLX
#define PDMPMT_HAS_GLX 0
#endif  // PDMPMT_HAS_GLX

#include "pdmpmt/common.h"
#include "pdmpmt/features.h"
#include "pdmpmt/warnings.h"

#ifdef _WIN32
#include "pdmpmt/win32.hh"
#endif  // _WIN32

/**
 * Type alias name associated with the given OpenGL extension function.
 *
 * @note This is only meaningful after `PDMPMT_GLEXT_FUNC` is used for `name`.
 *
 * @param name OpenGL extension function name
 */
#define PDMPMT_GLEXT_FUNC_TYPE(name) \
  PDMPMT_CONCAT(PDMPMT_CONCAT(pdmpmt_opengl_, name), _type)

/**
 * Macro for defining an OpenGL extension function pointer.
 *
 * Since these extension functions can be context dependent each function
 * pointer is thread-local since contexts are bound to their creation threads.
 *
 * @param name OpenGL extension function name
 * @param ret Return type
 * @param ... Argument types
 */
#define PDMPMT_GLEXT_FUNC(name, ret, ...) \
  /* APIENTRY defined by OpenGL implementations */ \
  using PDMPMT_GLEXT_FUNC_TYPE(name) = ret (APIENTRY *)(__VA_ARGS__); \
  /* we don't want to have function pointers per-translation-unit */ \
  inline thread_local PDMPMT_GLEXT_FUNC_TYPE(name) name

// OpenGl extension functions. since these may be context dependent, we make
// the function pointers thread-local since contexts tied to individual threads
PDMPMT_GLEXT_FUNC(glGetStringi, const GLubyte*, GLenum name, GLuint index);

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
class exception : public std::runtime_error {
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

// currently only available for Windows
#ifdef _WIN32
/**
 * Class representing an OpenGL context.
 *
 * On scope exit the OpenGL context will be destroyed.
 */
class context {
public:
  /**
   * Ctor.
   *
   * Constructs from a Win32 window device context with set pixel format. Note
   * that the window device context must be alive and in scope for the lifetime
   * of the OpenGL context,e.g. `ReleaseDC` or `DeleteDC` not called.
   *
   * @param dc Win32 device context handle
   * @param current `true` to make the context the current OpenGL context
   */
  context(HDC dc, bool current = false)
    : dc_{dc}, glc_{wglCreateContext(dc)}, current_{current}
  {
    // failed
    if (!glc_)
      throw win32::exception{"OpenGL context creation failed"};
    // short-circuit if not making current
    if (current_ && !wglMakeCurrent(dc_, glc_))
      throw win32::exception{"Could not make OpenGL context current"};
  }

  /**
   * Deleted copy ctor.
   */
  context(const context&) = delete;

  /**
   * Move ctor.
   */
  context(context&& other) noexcept
  {
    from(std::move(other));
  }

  /**
   * Move assignment operator.
   */
  auto& operator=(context&& other) noexcept(noexcept(destroy()))
  {
    destroy();
    from(std::move(other));
    return *this;
  }

  /**
   * Dtor.
   */
  ~context() noexcept(noexcept(destroy()))
  {
    destroy();
  }

  /**
   * Return the Win32 device context associated with the OpenGL context.
   */
  HDC device_context() const noexcept { return dc_; }

  /**
   * Return the raw OpenGL context handle.
   */
  auto handle() const noexcept { return glc_; }

  /**
   * Indicate if the OpenGL context is the current context.
   *
   * The result of this function is only meaningful if checked on the same
   * thread that was used to create the OpenGL context.
   */
  bool current() const noexcept { return current_; }

  /**
   * Implicitly convert to the OpenGL context handle.
   *
   * This is useful for C function interop and to test for ownership.
   */
  operator HGLRC() const noexcept
  {
    return glc_;
  }

private:
  HDC dc_;
  HGLRC glc_;
  bool current_;

  /**
   * Move-initialize the OPenGL context.
   *
   * On completion `other` will have zeroed-out members.
   */
  void from(context&& other) noexcept
  {
    dc_ = other.dc_;
    glc_ = other.glc_;
    current_ = other.current_;
    other.dc_ = nullptr;
    other.glc_ = nullptr;
    other.current_ = false;
  }

  /**
   * Destroy the OpenGL context if the handle if not `nullptr`.
   */
  void destroy() noexcept
  {
    // TODO: could error, but usually we don't care. not sure it's worth making
    // this dtor noexcept(false) or trying to detect where to print errors
    if (glc_)
      wglDeleteContext(glc_);
  }
};
#endif  // _WIN32

/**
 * Macro representing the OPenGL extension loading routine.
 *
 * This resolves to `wglGetProcAddress` on Windows and `glXGetProcAddress` for
 * platforms that have a GLX implementation. Otherwise, it is undefined.
 *
 * @param name String literal for the OpenGL extension function name
 */
#if defined(_WIN32)
#define PDMPMT_LOAD_GLEXT(name) \
  /* no cast; expects LPCSTR */ \
  wglGetProcAddress(name)
// GLX extension init
#elif PDMPMT_HAS_GLX
#define PDMPMT_LOAD_GLEXT(name) \
  /* needs reinterpret_cast */ \
  glXGetProcAddress(reinterpret_cast<const GLubyte*>(name))
#endif  // !defined(_WIN32)

/**
 * Initialize all OpenGL extension of interest.
 *
 * If extensions were loaded, returns `true`, and returns `false` otherwise.
 *
 * @todo Consider if we want so add ways to report loading errors.
 */
inline bool init_extensions()
{
// macro for correctly assigning each function pointer
#if defined(PDMPMT_LOAD_GLEXT)
#define PDMPMT_GLEXT_INIT_FUNC(name) \
  do { \
    auto fptr = PDMPMT_LOAD_GLEXT(PDMPMT_STRINGIFY(name)); \
    /* kludge */ \
    name = (PDMPMT_GLEXT_FUNC_TYPE(name)) fptr; \
  } \
  while (false)
// empty if there is no extension loading function
#else
#define PDMPMT_GLEXT_INIT_FUNC(name)
#endif  // !defined(PDMPMT_LOAD_GLEXT)
// suppres C4191 for MSVC
PDMPMT_MSVC_WARNING_PUSH()
PDMPMT_MSVC_WARNING_DISABLE(4191)
  // call macro for each name of interest
  PDMPMT_GLEXT_INIT_FUNC(glGetStringi);
PDMPMT_MSVC_WARNING_POP()
// not needed anymore
#undef PDMPMT_GLEXT_INIT_FUNC
// return false if no extensions were loaded
#if defined(PDMPMT_LOAD_GLEXT)
  return true;
#else
  return false;
#endif  // !defined(PDMPMT_LOAD_GLEXT)
}

/**
 * Proxy object type for retrieving the list of OpenGL extension names.
 *
 * This helps provide a unified interface for OpenGL before and after OpenGL
 * 3.1 where `glGetString(GL_EXTENSIONS)` was removed.
 */
struct extensions_proxy_type {};

/**
 * Global for referring to OpenGL extensions.
 */
constexpr extensions_proxy_type extensions;

/**
 * Stream each OpenGL extension name separated by spaces.
 *
 * If `glGetStringi` is not available on an OpenGL 3.0+ platform the number of
 * extensions and a message on `glGetStringi` not being available is printed.
 */
inline auto& operator<<(std::ostream& out, extensions_proxy_type)
{
// OpenGL 3.1+ way of using GL_EXTENSIONS
#if defined(GL_VERSION_3_0)
  // get extension count
  GLint n_ext;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n_ext);
// FIXME: glGetStringi is not in OpenGL 1.1 and cannot be assumed to statically
// exist. we have a couple choices to pick from:
//
// 1. include glext.h and compile with GL_GLEXT_PROTOTYPES (not desirable)
// 2. correctly query the glGetStringi function pointer
//
// for now, we choose 2., as init_extensions helps with the function pointers
  if (!glGetStringi)
    return out << n_ext << " (cannot list names without glGetStringi)";
  // iterate
  for (GLint i = 0; i < n_ext; i++) {
    if (i)
      out << " ";
    out << glGetStringi(GL_EXTENSIONS, i);
  }
// use old glGetString(NUM_EXTENSIONS)
#else
  out << glGetString(GL_EXTENSIONS);
#endif  // !defined(GL_NUM_EXTENSIONS)
  return out;
}

// TODO: add operator<< for std::vector

}  // namespace opengl
}  // namespace pdmpmt

#endif  // PDMPMT_OPENGL_HH_
