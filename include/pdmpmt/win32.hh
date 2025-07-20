/**
 * @file win32.hh
 * @author Derek Huang
 * @brief C++ header for Win32 helpers
 * @copyright MIT License
 *
 * @note Some functions may require linking against extra Windows libraries.
 */

#ifndef PDMPMT_WIN32_HH_
#define PDMPMT_WIN32_HH_

// reduce Windows.h include size and exclude outdated headers like Windows
// Sockets 1. there is generally little reason to not define this macro
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <ios>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace pdmpmt {
namespace win32 {

/**
 * Retrieve the error message for the given Win32 system error code.
 *
 * This error code would be returned by `GetLastError()` for example.
 *
 * @note The returned message may be terminated with a newline.
 *
 * @param err Win32 system error code
 */
inline auto strerror(DWORD err)
{
  // pointer to buffer allocated by FormatMessageA
  HLOCAL buf;
  // allocate buffer for corresponding error message and return its length. the
  // returned value does *not* include the added null terminator. adapted from
  // https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code
  auto len = FormatMessageA(
    // allocate buffer, indicate system message source, don't format
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    // ignored message source since FORMAT_MESSAGE_FROM_SYSTEM is the source
    nullptr,
    // message is the error code
    err,
    // use default lannguage
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    // pass in pointer message buffer for LocalAlloc (we must cast)
    reinterpret_cast<LPSTR>(&buf),
    // no minimum allocation size
    0,
    // va_list of values for any %n format specifiers
    nullptr
  );
  // if zero, failed, so just return generic message
  // note: the GetLastError value might also be an HRESULT
  if (!len) {
    std::stringstream ss;
    ss << "FormatMessageA error " << GetLastError() <<
      " for " << err << " (" << std::hex << err << ")";
    return ss.str();
  }
  // otherwise, copy to string, clean up, and return
  std::string msg{static_cast<const char*>(buf), len};
  // if error freeing append make note of this
  if (LocalFree(buf)) {
    std::stringstream ss;
    ss << " LocalFree error " << GetLastError();
    msg += ss.str();
  }
  // done
  return msg;
}

/**
 * Retrieve the error message for the `GetLastError()` error code.
 *
 * This retrieves the error message for the last thread-local error recorded.
 */
inline auto strerror()
{
  return strerror(GetLastError());
}

/**
 * Exception to represent a Win32 system exception.
 */
class exception : public std::runtime_error {
public:
  /**
   * Default ctor.
   *
   * This constructs an exception with the error message corresponding to the
   * value of `GetLastError()` without any additional information.
   */
  exception() : exception{GetLastError()} {}

  /**
   * Ctor.
   *
   * This constructs an exception with the error message corresponding to the
   * value of `GetLastError()` with a user-defined message.
   *
   * @param msg User-defined message to precede the Win32 error message
   */
  exception(const std::string& msg) : exception{GetLastError(), msg} {}

  /**
   * Ctor,
   *
   * Constructs from just the @Win32 error code.
   *
   * @param err Win32 system error code
   */
  exception(DWORD err)
    : std::runtime_error{"Win32 error: " + strerror(err)}, err_{err}
  {}

  /**
   * Ctor.
   *
   * Constructs from a Win32 error code and a user-defined message.
   *
   * @param err Win32 syste error code
   * @param msg User-defined message to precede the Win32 error message
   */
  exception(DWORD err, const std::string& msg)
    : std::runtime_error{"Win32 error: " + msg + ". " + strerror(err)},
      err_{err}
  {}

  /**
   * Return the Windows system error code.
   */
  auto err() const noexcept { return err_; }

private:
  DWORD err_;
};

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
      throw exception{"Unable to get device context for HWND"};
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
      throw exception{
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
}  // namespace pdmpmt

#endif  // PDMPMT_WIN32_HH_
