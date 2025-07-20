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

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <GL/gl.h>

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
#else
  std::cout << "not implemented" << std::endl;
#endif  // !defined(_WIN32)
  return EXIT_SUCCESS;
}
