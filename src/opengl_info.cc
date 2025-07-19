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
#include <errhandlingapi.h>
#include <wingdi.h>
#include <WinUser.h>
#endif  // _WIN32

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <utility>

#include <GL/gl.h>

namespace {

#ifdef _WIN32
namespace win32 {

/**
 * Retrieve the error message for the given Win32 system error code.
 *
 * This error code would be returned by `GetLastError()` for example.
 *
 * @param err Win32 system error code
 */
auto strerror(DWORD err)
{
  return std::system_category().message(static_cast<int>(err));
}

/**
 * Retrieve the error message for the `GetLastError()` error code.
 *
 * This retrieves the error message for the last thread-local error recorded.
 */
auto strerror()
{
  return strerror(GetLastError());
}

/**
 * Create or acquire a new Win32 device context.
 *
 * The created handle will be appropriately released or deleted depending on
 * whether it was retrieved or created anew.
 */
class device_context {
public:
  /**
   * Tag type for the ctor overload for the entire screen's device context.
   */
  struct screen_tag {};
  static constexpr screen_tag screen;

  /**
   * Default ctor.
   *
   * The device context handle is `nullptr` and is unusable.
   */
  constexpr device_context() noexcept = default;

  /**
   * Ctor.
   *
   * Retrieve the device context for the entire screen.
   */
  explicit device_context(screen_tag) : device_context{nullptr} {}

  /**
   * Ctor.
   *
   * Retrieve a device context handle from the given window handle. If the
   * given handle is `nullptr` then the entire screen is used.
   *
   * @param hwnd Windows handle to retrieve device context for
   */
  explicit device_context(HWND hwnd)
    : handle_{GetDC(hwnd)}, window_{hwnd}, owned_{false}
  {
    // GetDC failed
    if (!handle_)
      throw std::system_error{
        // cast to avoid disallowed narrowing
        static_cast<int>(GetLastError()),
        std::system_category(),
        "Unable to get device context for HWND"
      };
  }

  /**
   * Ctor.
   *
   * Create a new device context for a specific output device. See
   * https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdca.
   *
   * @note The windows handle will be `nullptr`, which is technically valid as
   *  it represents the entire screen, but in this case
   *
   * @param driver
   * @param device
   * @param params
   */
  device_context(const char* driver, const char* device, const DEVMODEA* params)
    : handle_{CreateDCA(driver, device, nullptr, params)}, owned_{true}
  {
    // CreateDCA failed
    if (!handle_)
      throw std::system_error{
        // cast to avoid disallowed narrowing
        static_cast<int>(GetLastError()),
        std::system_category(),
        "Failed to create device context with driver " + std::string{driver} +
        " and device " + device
      };
  }

  /**
   * Deleted copy ctor.
   */
  device_context(const device_context&) = delete;

  /**
   * Move ctor.
   */
  device_context(device_context&& other) noexcept
  {
    from(std::move(other));
  }

  /**
   * Dtor.
   */
  ~device_context()
  {
    destroy();
  }

  /**
   * Move assignment operator.
   */
  auto& operator=(device_context&& other) noexcept
  {
    destroy();
    from(std::move(other));
    return *this;
  }

  /**
   * Return the raw device context handle.
   *
   * This handle can only be used by one thread at a time.
   */
  HDC handle() const noexcept { return handle_; }

  /**
   * Return the window handle associated with the device context.
   *
   * The window handle is `nullptr` if the device context is that of the entire
   * screen or if the device context was created with `CreateDCA`. To check for
   * ownership the `owned()` member should be called instead.
   */
  HWND window() const noexcept { return window_; }

  /**
   * Indicate if the handle is owned.
   *
   * An owned handle is one created with `CreateDCA`.
   */
  bool owned() const noexcept { return owned_; }

  /**
   * Return the raw device context handle.
   *
   * This is useful for Win32 interop or for checking handle validity.
   */
  operator HDC() const noexcept
  {
    return handle_;
  }

private:
  HDC handle_{};
  HWND window_{};
  bool owned_{};

  /**
   * Initialize the device context from another instance.
   *
   * On completion the other instance will have `nullptr` device context and
   * window handles. The ownership flag will be set to `false`.
   */
  void from(device_context&& other) noexcept
  {
    handle_ = other.handle_;
    window_ = other.window_;
    owned_ = other.owned_;
    // zero out
    other.handle_ = nullptr;
    other.window_ = nullptr;
    other.owned_ = false;
  }

  /**
   * Destroy the device context.
   *
   * Either `ReleaseDC` or `DeleteDC` is called if the handle is not `mullptr`.
   */
  void destroy() noexcept
  {
    // if no handle, skip
    if (!handle_)
      return;
    // otherwise, delete/release based on ownership
    if (owned_)
      // TODO: don't ignore return value
      DeleteDC(handle_);
    else
      ReleaseDC(window_, handle_);
  }
};

}  // namespace win32
#endif  // _WIN32

const auto progname = std::filesystem::path{__FILE__}.string();

}  // namespace

int main()
{
#ifdef _WIN32
  // register window class. we don't care about the icon
  WNDCLASSA wclass{};
  // default window callback (since we won't do anything)
  wclass.lpfnWndProc = DefWindowProcA;
  // TODO: do i need to set hInstance?
  // use program name for the menu/class name
  wclass.lpszMenuName = progname.c_str();
  wclass.lpszClassName = progname.c_str();
  // register Window class
  if (!RegisterClassA(&wclass)) {
    std::cerr << "Error: Failed to register window class. " <<
      win32::strerror() << std::endl;
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
      win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // get device context for window
  win32::device_context dc{wnd};
  // set up pixel format descriptor
  PIXELFORMATDESCRIPTOR pfd{};
  pfd.nSize = sizeof pfd;
  pfd.nVersion = 1;
  // ask for OpenGL support
  pfd.dwFlags = /*PFD_DRAW_TO_WINDOW |*/ PFD_SUPPORT_OPENGL;
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
      win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // now set the pixel format for the device context
  if (SetPixelFormat(dc, px_fmt, &pfd) != TRUE) {
    std::cerr << "Error: Couldn't set device pixel format. " <<
      win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // create OpenGL context
  auto glctx = wglCreateContext(dc);
  if (!glctx) {
    std::cerr << "Error: OpenGL context creation failed. " <<
      win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // make OpenGL context current
  if (wglMakeCurrent(dc, glctx) != TRUE) {
    std::cerr << "Error: Could not make OpenGL context current. " <<
      win32::strerror() << std::endl;
    return EXIT_FAILURE;
  }
  // get OPenGL version
  auto gl_version = glGetString(GL_VERSION);
  std::cout << "OpenGL version: " << gl_version << std::endl;
  // TODO: print more OpenGL info
  // done, delete OpenGL context and release device context
  if (wglDeleteContext(glctx) != TRUE) {
    std::cerr << "Error: Failed to delete OpenGL context. " <<
      win32::strerror() << std::endl;
    return EXIT_SUCCESS;
  }
#else
  std::cout << "not implemented" << std::endl;
#endif  // _WIN32
  return EXIT_SUCCESS;
}
