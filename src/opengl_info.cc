/**
 * @file opengl_info.cc
 * @author Derek Huang
 * @brief C++ program for querying OpenGL platform/device info
 * @copyright MIT License
 */

#ifdef _WIN32
// reduce Windows.h include size
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wingdi.h>
#endif  // _WIN32

// TODO: make part of features.h
#ifdef __has_include
#if __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#define PDMPMT_HAS_X11 1
#endif  // __has_include(<X11/Xlib.h>)
#endif  // __has_include

#ifndef PDMPMT_HAS_X11
#define PDMPMT_HAS_X11 0
#endif  // PDMPMT_HAS_X11

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <GL/gl.h>
// TODO: should have separate check for GLX
#if PDMPMT_HAS_X11
#include <GL/glx.h>
#endif  // PDMPMT_HAS_X11

#include "pdmpmt/opengl.hh"
#ifdef _WIN32
#include "pdmpmt/win32.hh"
#endif  // _WIN32

namespace {

const auto progname = std::filesystem::path{__FILE__}.string();

}  // namespace

int main()
{
#if defined(_WIN32)
  // register window class. we don't care about the icon
  WNDCLASSA wclass{};
  // default window callback (since we won't do anything)
  wclass.lpfnWndProc = DefWindowProcA;
  // note: nullptr hInstance seems to be allowed
  // use program name for the menu/class name
  wclass.lpszMenuName = progname.c_str();
  wclass.lpszClassName = progname.c_str();
  // register Window class
  if (!RegisterClassA(&wclass)) {
    std::cerr << "Error: Failed to register window class. " <<
      pdmpmt::win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // create window
  auto wnd = CreateWindowA(
    wclass.lpszClassName,
    wclass.lpszClassName,  // make window name the same for now
    0,                     // window style flags (make visible with WS_VISIBLE)
    CW_USEDEFAULT,         // defaulted x position
    CW_USEDEFAULT,         // defaulted y position
    CW_USEDEFAULT,         // defaulted width
    CW_USEDEFAULT,         // defaulted height
    nullptr,               // parent window (none for now)
    nullptr,               // menu/child window identifier
    wclass.hInstance,
    nullptr                // extra parameter for window creation
  );
  if (!wnd) {
    std::cerr << "Error: Unable to create dummy window. " <<
      pdmpmt::win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // get device context for window
  pdmpmt::win32::device_context dc{wnd};
  // set up pixel format descriptor
  PIXELFORMATDESCRIPTOR pfd{};
  pfd.nSize = sizeof pfd;
  pfd.nVersion = 1;
  // ask for OpenGL support for window and standard double-buffering
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  // 1 octet for each color channel
  pfd.cColorBits = 8;
  // 4 octets for the z-buffer
  pfd.cDepthBits = 32;
  pfd.iLayerType = PFD_MAIN_PLANE;
  // choose pixel format given the descriptr + device context
  auto px_fmt = ChoosePixelFormat(dc, &pfd);
  if (!px_fmt) {
    std::cerr << "Error: Couldn't choose device pixel format. " <<
      pdmpmt::win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // now set the pixel format for the device context
  if (SetPixelFormat(dc, px_fmt, &pfd) != TRUE) {
    std::cerr << "Error: Couldn't set device pixel format. " <<
      pdmpmt::win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // create OpenGL context, make it current, and initialize extensions
  pdmpmt::opengl::context glc{dc, true};
  pdmpmt::opengl::init_extensions();
  // print some OpenGL info
  std::cout <<
    "OpenGL version: " << glGetString(GL_VERSION) << "\n" <<
    "OpenGL vendor: " << glGetString(GL_VENDOR) << "\n" <<
    "OpenGL renderer: " << glGetString(GL_RENDERER) << "\n" <<
    // TODO: re-enable later if user supplies command-line flag. there can be
    // a large number of extensions for the device and it becomes very verbose
    "OpenGL extensions: " << pdmpmt::opengl::extensions << "\n" <<
    std::flush;
#elif PDMPMT_HAS_X11
  // open default X display (e.g. :0 on WSL1 from the DISPLAY env var)
  auto dsp = XOpenDisplay(nullptr);
  if (!dsp) {
    // default display is from the DISPLAY env var. if nullptr, then unknown
    auto dsp_name = []
    {
      auto name = std::getenv("DISPLAY");
      return (name) ? name : "(unknown)";
    }();
    std::cerr << "Error: Unable to open default display " << dsp_name <<
      std::endl;
    return EXIT_FAILURE;
  }
  // default screen number
  auto scn = DefaultScreen(dsp);
  // print display info
  std::cout <<
    "X display: " << DisplayString(dsp) << "\n" <<
    "X protocol: " << ProtocolVersion(dsp) << "." << ProtocolRevision(dsp) <<
      "\n" <<
    "X vendor: " << ServerVendor(dsp) << "\n" <<
    "X screen: " << scn << "\n" <<
    std::flush;
  // check if GLX is supported by the X display
  // TODO: what are the GLX error/event bases?
  if (!glXQueryExtension(dsp, nullptr, nullptr)) {
    std::cerr << "Error: X display " << DisplayString(dsp) <<
      " does not support GLX" << std::endl;
    return EXIT_FAILURE;
  }
  // GLX info
  std::cout <<
#if defined(GLX_VERSION_1_1)
    "GLX server vendor: " << glXQueryServerString(dsp, scn, GLX_VENDOR) <<
      "\n" <<
    "GLX server version: " << glXQueryServerString(dsp, scn, GLX_VERSION) <<
      "\n" <<
    "GLX server extensions: " << glXQueryServerString(dsp, scn, GLX_EXTENSIONS) <<
      "\n" <<
    "GLX client vendor: " << glXGetClientString(dsp, GLX_VENDOR) << "\n" <<
    "GLX client version: " << glXGetClientString(dsp, GLX_VERSION) << "\n" <<
    "GLX client extensions: " << glXGetClientString(dsp, GLX_EXTENSIONS) <<
      "\n" <<
#else
    "GLX client/server information excluded (GLX version < 1.1)" <<
#endif  // !defined(GLX_VERSION_1_1)
    std::flush;
  // choose pixel format. this is as close to the Windows format as posible
  int px_fmt[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, None};
  auto xvi = glXChooseVisual(dsp, scn, px_fmt);
  if (!xvi) {
    std::cerr << "Error: Unable to choose appropriate X visual" << std::endl;
    return EXIT_FAILURE;
  }
  // create OpenGL context
  // TODO: still need to choose pixel format, create OpenGL context, etc.
  // done, free visual info + close display
  XFree(xvi);
  // TODO: handle possible BadGC error
  XCloseDisplay(dsp);
#endif  // !defined(_WIN32)
  return EXIT_SUCCESS;
}
