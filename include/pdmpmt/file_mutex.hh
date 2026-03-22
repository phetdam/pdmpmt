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
#include <fileapi.h>
#include <synchapi.h>
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
   * the inotify descriptor for more file deletion events. As one migh expect,
   * using inotify is much more efficient than using a spinlock, as the latter
   * produces more volatile timings and consumes much more user + kernel CPU.
   *
   * However, on Windows, we use a `try_lock()` spin despite the availability
   * of directory change notification functions, as spinning provides better
   * performance compared to using `Find(First|Next)ChangeNotification[W]()` or
   * `ReadDirectoryChangesW()`. These directory change notification functions
   * result in noticeably slower performance when there is high contention. See
   * the corresponding document for more details and exact timing data.
   */
  void lock()
  {
    // first attempt to try_lock() directly
    if (try_lock())
      return;
    // otherwise run a loop to acquire the lock. on Windows the best-performing
    // implementation is a spinlock, while on Linux, inotify is the best choice
#if defined(_WIN32)
    while (!try_lock());
#else
    inotify_lock();
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

#if defined(_WIN32)
  /**
   * Perform a blocking lock for the mutex.
   *
   * This example implementation uses the Win32 directory change notification
   * functions `Find(First|Next)ChangeNotification()` and waits on provided
   * directory handle until a file name change event occurs, e.g. because a
   * file was renamed, created, or deleted, before calling `try_lock()` again.
   *
   * Although simpler to code up compared to using `ReadDirectoryChangesW()`
   * the directory change notification functions do not enable identification
   * of which file the received event affected.
   *
   * @note This function is unused and provided only for exposition.
   */
  void find_first_change_lock()
  {
    // get notification handle receiving directory file name change events
    auto cnh = FindFirstChangeNotificationW(
      path_.parent_path().c_str(),
      FALSE,                        // do not watch subtrees
      FILE_NOTIFY_CHANGE_FILE_NAME  // file rename, delete, creation
    );
    if (cnh == bad_handle)
      throw std::system_error{
        {static_cast<int>(GetLastError()), std::system_category()},
        "FindFirstChangeNotificationW() error"
      };
    // ensure we stop monitoring directory change notifications on scope exit
    scope_exit cnh_guard{[cnh] { FindCloseChangeNotification(cnh); }};
    // begin change notification handle monitoring loop
    while (true) {
      // block until we fail or get a notification
      switch (WaitForSingleObject(cnh, INFINITE)) {
      // received notification
      case WAIT_OBJECT_0:
        // attempt try_lock() again. if successful, we can return
        if (try_lock())
          return;
        // otherwise prepare handle for next wait until notification
        if (!FindNextChangeNotification(cnh))
          throw std::system_error{
            {static_cast<int>(GetLastError()), std::system_category()},
            "FindNextChangeNotification() error"
          };
        break;
      // error otherwise as WAIT_ABANDONED and WAIT_TIMEOUT not possible
      default:
        throw std::system_error{
          {static_cast<int>(GetLastError()), std::system_category()},
          "WaitForSingleObject() error"
        };
      }
    }
  }

  /**
   * Perform a blocking lock for the mutex.
   *
   * This example implementation uses blocking calls the Win32 function
   * `ReadDirectoryChangesW()` on a directory handle for file notification
   * events, similar to inotify in Linux, and calls `try_lock()` if the file
   * event corresponds to the deletion of the lockfile. `try_lock()` is also
   * called If there are too many change events for the system to enumerate or
   * record in the buffer as a fallback mechanism.
   *
   * This implementation is a bit more complex compared to using the
   * `Find(First|Next)ChangeNotification()` API but allows us to avoid calling
   * `try_lock()` for changes to unrelated files in the same directory.
   *
   * @note This function is unused and provided only for exposition.
   */
  void read_directory_changes_lock()
  {
    // open directory handle for read
    auto dh = CreateFileW(
      path_.parent_path().c_str(),
      GENERIC_READ,
      FILE_SHARE_READ,
      nullptr,                     // default security attributes
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS,  // required for opening a directory
      nullptr                      // hTemplateFile
    );
    if (dh == bad_handle)
      throw std::system_error{
        {static_cast<int>(GetLastError()), std::system_category()},
        "CreateFileW() error"
      };
    // ensure directory handle closed on scope exit
    scope_exit dh_guard{[dh] { CloseHandle(dh); }};
    // buffer for FILE_NOTIFY_INFORMATION variable-size struct
    // note: MAX_PATH is for the entire *path* but unlikely for a file name
    // without directory qualifiers to exceed 256 chars anyways
    alignas(DWORD) char buf[sizeof(FILE_NOTIFY_INFORMATION) + MAX_PATH];
    // bytes returned in buffer by ReadDirectoryChangesW()
    DWORD buf_size = 0u;
    // begin directory change monitoring loop. we always first try_lock() to
    // prevent blocking when there are no changes to the lockfile
    while (!try_lock()) {
      // block for directory changes
      if (
        !ReadDirectoryChangesW(
          dh,
          buf,
          sizeof buf,
          FALSE,                         // don't monitor subdirectories
          FILE_NOTIFY_CHANGE_FILE_NAME,  // rename, create, deletion
          &buf_size,
          nullptr,                       // no OVERLAPPED for async operation
          nullptr                        // no async completion handler
        )
      ) {
        switch (GetLastError()) {
        // system couldn't enumerate all changes so fall back on try_lock()
        case ERROR_NOTIFY_ENUM_DIR:
          continue;
        // other errors
        default:
          throw std::system_error{
            {static_cast<int>(GetLastError()), std::system_category()},
            "ReadDirectoryChangesW() error"
          };
        }
      }
      // if buf_size still zero, too many changes, so fall back to try_lock()
      if (!buf_size)
        continue;
      // loop through file notify structs
      auto fni = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buf);
      // when offset is zero, no more info
      while (!fni->NextEntryOffset) {
        // get name of affected file
        std::wstring_view fname{fni->FileName, fni->FileNameLength / sizeof(WCHAR)};
        // if lockfile deleted + try_lock() succeeds, we are done
        if (
          (fname == path_.filename().c_str()) &&
          (fni->Action == FILE_ACTION_REMOVED) &&
          try_lock()
        )
          return;
        // otherwise advance
        fni = reinterpret_cast<decltype(fni)>(buf + fni->NextEntryOffset);
      }
      // resume looping
    }
  }
#else
  /**
   * Perform a blocking lock for the mutex.
   *
   * This function suses inotify to efficiently check if the lockfile has been
   * deleted, blocking using `poll()` until the kernel indicates a file change
   * in the monitored directory. If the deleted file is the lockfile and
   * `try_lock()` succeeds, we can then acquire the lock.
   */
  void inotify_lock()
  {
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
  }
#endif  // !defined(_WIN32)
};

}  // namespace pdmpmt

#endif  // PDMPMT_FILE_MUTEX_HH_
