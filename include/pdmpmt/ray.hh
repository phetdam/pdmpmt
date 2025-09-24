/**
 * @file ray.hh
 * @author Derek Huang
 * @brief C++ header for Ray API helpers
 * @copyright MIT License
 */

#ifndef PDMPMT_RAY_HH_
#define PDMPMT_RAY_HH_

#include <csignal>

#include <ray/api.h>

namespace pdmpmt {

/**
 * Ray runtime context manager.
 *
 * This ensures that the Ray runtime is correctly shut down on program exit.
 *
 * @note Every C++ process using the Ray C++ API *must* call `ray::Init()` and
 *  `ray::Shutdown()`. This of course dashes any hope of using one process to
 *  start a Ray cluster, have separate processes connect to and submit tasks,
 *  and then have the first process eventually do the shutdown.
 */
class ray_runtime_context {
public:
  /**
   * Default ctor.
   *
   * Calls the equivalent of `ray::Init()`.
   *
   * @param stop_on_abort `true` to ensure Ray shutdown before `abort()`
   */
  ray_runtime_context(bool stop_on_abort = true)
  {
    ray::Init();
    // on error, Ray will rudely abort, so calling ray::Shutdown() is desirable
    set_abort_handler(stop_on_abort);
  }

  /**
   * Ctor.
   *
   * Delegates arguments to `Init(ray::RayConfig&)` overload.
   *
   * @param stop_on_abort `true` to ensure Ray shutdown before `abort()`
   */
  ray_runtime_context(ray::RayConfig& cfg, bool stop_on_abort = true)
  {
    ray::Init(cfg);
    // on error, Ray will rudely abort, so calling ray::Shutdown() is desirable
    set_abort_handler(stop_on_abort);
  }

  /**
   * Dtor.
   *
   * Ensures that the Ray runtime is cleaned up before exit.
   */
  ~ray_runtime_context()
  {
    // TODO: maybe put in a try-catch
    ray::Shutdown();
  }

  /**
   * Indicate if `ray::Shutdown()` is called if the program aborts.
   */
  bool stop_on_abort() const noexcept { return stop_on_abort_; }

private:
  bool stop_on_abort_;

  /**
   * Installs the necessary signal handler for `SIGABRT` for Ray shutdown.
   *
   * On successful installation of the `SIGABRT` handler, `stop_on_abort_` is
   * `true`, otherwise it is set to `false`.
   *
   * @param stop_on_abort `true` to ensure Ray shutdown before `abort()`
   */
  void set_abort_handler(bool stop_on_abort) noexcept
  {
    if (stop_on_abort) {
      auto prev = std::signal(SIGABRT, [](int) { ray::Shutdown(); });
      stop_on_abort_ = (SIG_ERR != prev);
    }
    else
      stop_on_abort_ = false;
  }
};

}  // namespace pdmpmt

#endif  // PDMPMT_RAY_HH_
