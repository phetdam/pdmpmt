/**
 * @file file_mutex.hh
 * @author Derek Huang
 * @brief C++ header for a file-based inter-thread and inter-process mutex
 * @copyright MIT License
 */

#ifndef PDMPMT_FILE_MUTEX_HH_
#define PDMPMT_FILE_MUTEX_HH_

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <fcntl.h>
#include <limits.h>       // for portable NAME_MAX on POSIX systems
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

#include "pdmpmt/scope_exit.hh"

namespace pdmpmt {

/**
 * File-based mutex for inter-thread and inter-process synchronization.
 *
 * This satifies the *Lockable* C++ named requirements and is suitable for
 * synchronizing both threads and processes, although using `std::mutex` is
 * likely more performant if only threads need synchronization.
 *
 * @note If the associated lockfile is accidentally left on the filesystem due
 *  to an abnormal program exit, future `try_lock()` calls will always return
 *  `false` and `lock()` calls will block forever.
 */
class file_mutex {
public:
  // file descriptor/handle type
#if defined(_WIN32)
  using handle_type = HANDLE;
#else
  using handle_type = int;
#endif  // !defined(_WIN32)

  // value for invalid handle
#if defined(_WIN32)
  // cannot be constexpr as INVALID_HANDLE_VALUE is not a valid address
  static inline const handle_type bad_handle = INVALID_HANDLE_VALUE;
#else
  static constexpr handle_type bad_handle = -1;
#endif  // !defined(_WIN32)

  /**
   * Ctor.
   *
   * @param path Path to file used to represent mutex lock
   */
  file_mutex(std::filesystem::path path) noexcept : path_{std::move(path)} {}

  /**
   * Deleted copy ctor.
   */
  file_mutex(const file_mutex&) = delete;

  /**
   * Lock the mutex, blocking if the mutex cannot be acquired.
   *
   * On Linux, inotify is used to implement the `try_lock()` then block logic,
   * monitoring the directory containing the lockfile for file deletion events.
   * If the received event corresponds to the lockfile being deleted, then we
   * `try_lock()` again, and if unsuccessful, block again to continue polling
   * the inotify descriptor for more file deletion events.
   *
   * Using inotify is much more efficient than using a spinlock, as the latter
   * produces more volatile timings and consumes much more user + kernel CPU.
   *
   * @todo Stop using spinning `try_lock()` for Windows and use the directory
   *  change notification functions to follow the inotify model.
   */
  void lock()
  {
    // TODO: use Find(First|Next)ChangeNotificationW() functions on Windows to
    // obtain directory change notifications like with Linux inotify
#if defined(_WIN32)
    while (!try_lock());
#else
    // first attempt to try_lock() directly
    if (try_lock())
      return;
    // blocked, so get fd to inotify instance
    auto ifd = inotify_init();
    if (ifd < 0)
      throw std::system_error{
        errno, std::system_category(), "inotify_init() error"
      };
    // ensure we close the inotify fd on scope exit. when the inotify fd is
    // closed, all associated watches will also be freed
    scope_exit ifd_guard{[ifd] { close(ifd); }};
    // watch the lockfile directory for deletion events
    auto wd = inotify_add_watch(ifd, path_.parent_path().c_str(), IN_DELETE);
    if (wd < 0)
      throw std::system_error{
        errno, std::system_category(), "inotify_add_watch() error"
      };
    // begin poll() loop blocking for read events on inotify fd
    pollfd pfd{ifd, POLLIN};
    while (true) {
      // block for result
      // note: should always return 1 except on error since we block
      if (poll(&pfd, 1, -1) < 0)
        throw std::system_error{errno, std::system_category(), "poll() error"};
      // if no POLLIN (inotify event to read), go back to blocking
      if (!(pfd.revents & POLLIN))
        continue;
      // buffer large enough for one variable-size inotify event + pointer
      alignas(inotify_event) char ebuf[sizeof(inotify_event) + NAME_MAX + 1U];
      auto iev = reinterpret_cast<const inotify_event*>(ebuf);
      // read() event
      if (read(ifd, ebuf, sizeof ebuf) < 0)
        throw std::system_error{errno, std::system_category(), "read() error"};
      // if lockfile deleted, we try_lock(), and break if successful
      // note: need to check wd since event queue can overflow (iev->wd -1)
      if (
        (iev->wd == wd) &&
        (iev->mask & IN_DELETE) &&
        (std::string_view{iev->name} == path_.filename().c_str()) && try_lock()
      )
        break;
    }
#endif  // !defined(_WIN32)
  }

  /**
   * Unlock the mutex.
   *
   * Any errors from closing the file descriptor/handle are ignored and the
   * associated lockfile is deleted from the filesystem.
   */
  void unlock() noexcept
  {
#if defined(_WIN32)
    CloseHandle(handle_);
    DeleteFileW(path_.c_str());
#else
    close(handle_);
    unlink(path_.c_str());
#endif  // !defined(_WIN32)
  }

  /**
   * Attempt to lock the mutex in a thread-safe manner.
   *
   * This attempts to create a new exclusive-access read-only file. Therefore,
   * on Linux `open()` is called with `O_EXCL` and on Windows, `CreateFileA()`
   * with `FILE_ATTRIBUTE_NORMAL` is called with `CREATE_NEW`.
   *
   * @returns `true` if locking succeeded, `false` otherwie
   */
  bool try_lock()
  {
#if defined(_WIN32)
    // get handle to file for read
    auto handle = CreateFileW(
      path_.c_str(),
      GENERIC_READ,
      0,                      // exclusive access
      nullptr,                // default security attributes
      CREATE_NEW,
      // note: could use FILE_FLAG_DELETE_ON_CLOSE to ensure that the file is
      // deleted whenever all its handles are closed, which is desirable if
      // the program crashed while holding the mutex. this would be behavior
      // that is different from the POSIX implementation however
      FILE_ATTRIBUTE_NORMAL,
      nullptr                 // hTemplateFile
    );
    // if valid, update handle_ and indicate success
    if (handle != bad_handle) {
      handle_ = handle;
      return true;
    }
    // othewise, deal with errors
    switch (GetLastError()) {
    // if file exists or access denied (not done deleting) then lock failed
    case ERROR_FILE_EXISTS:
    case ERROR_ACCESS_DENIED:
      return false;
    // other errors
    default:
      // note: cast to avoid narrowing error from DWORD to int
      // note: using error_code overload causes MSVC to emit C4868
      throw std::system_error{
        static_cast<int>(GetLastError()),
        std::system_category(),
        "error creating lockfile with CreateFileA()"
      };
    }
#else
    // get handle to read-only file
    auto handle = open(
      path_.c_str(),
      O_CREAT | O_EXCL,
      S_IRUSR | S_IRGRP | S_IROTH
    );
    // if valid, update handle_ and indicate success
    if (handle != bad_handle) {
      handle_ = handle;
      return true;
    }
    // otherwise, handle errors
    switch (errno) {
    // file already exists so can't lock
    case EEXIST:
      return false;
    // other errors
    default:
      throw std::system_error{
        {errno, std::system_category()},
        "error creating lockfile with open()"
      };
    }
#endif  // !defined(_WIN32)
  }

private:
  std::filesystem::path path_;      // lockfile path
  handle_type handle_{bad_handle};  // lockfile handle
};

}  // namespace pdmpmt

#endif  // PDMPMT_FILE_MUTEX_HH_
