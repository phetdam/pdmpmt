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
#include <unistd.h>
#endif  // !defined(_WIN32)

#include <filesystem>
#include <system_error>
#include <utility>

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
   * Lock the mutex.
   *
   * This simply spins on `try_lock()` for simplicity.
   *
   * @todo For better performance this should be implemented by monitoring file
   *  system events using OS interfaces, e.g. inotify on Linux or the
   *  `Find(First|Next)ChangeNotificationW()` functions on Windows.
   */
  void lock()
  {
    while (!try_lock());
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
